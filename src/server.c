#include "server.h"
#include "http.h"
#include <signal.h>

int server_running = 1;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_t threads[THREAD_COUNT];
int queue[QUEUE_SIZE];
int front = 0;
int rear = 0;
int count = 0;

void handle_signal(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        printf("\nKapanış sinyali alındı, sunucu durduruluyor...\n");
        server_running = 0;
        pthread_cond_broadcast(&queue_cond);
    }
}

static void *_handle_client(int client_fd)
{
    char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));

    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (bytes_received > 0)
    {
        HttpRequest httpRequest;

        if (parse_http_request(buffer, &httpRequest) == 0)
        {
            route_request(client_fd, &httpRequest);
        }
    }

    free(buffer);

    return NULL;
}

void queue_push(int client_fd)
{

    pthread_mutex_lock(&queue_mutex);

    if (count == QUEUE_SIZE)
    {
        pthread_mutex_unlock(&queue_mutex);
        close(client_fd);
        return;
    }

    queue[rear] = client_fd;
    rear = (rear + 1) % QUEUE_SIZE;
    count++;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int queue_pop()
{
    pthread_mutex_lock(&queue_mutex);

    // DİKKAT: Artık sadece count'u değil, server_running'i de kontrol ediyoruz.
    while (count == 0 && server_running == 1)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Uykudan uyandık. Neden uyandık?
    // Eğer sunucu kapanma sinyali verdiyse VE işlenecek iş kalmadıysa çık!
    if (server_running == 0 && count == 0)
    {
        pthread_mutex_unlock(&queue_mutex);
        return -1; // Thread'e "çıkış yap" mesajı olarak -1 gönderiyoruz
    }

    int client_fd = queue[front];
    front = (front + 1) % QUEUE_SIZE;
    count--;

    pthread_mutex_unlock(&queue_mutex);
    return client_fd;
}

void *worker_thread()
{
    while (1)
    {
        int client_fd = queue_pop();

        if (client_fd == -1)
        {
            break; // Döngüden çık ve thread'i temizce sonlandır.
        }

        _handle_client(client_fd);
        close(client_fd);
    }

    return NULL;
}

void initHeaders()
{
    initHttpNotFoundHeaders();
    initHttpServerErrorHeaders();
    initHttpNotFoundResponse();
}

void closingRoutine(int server_fd)
{
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        // pthread_join, ilgili thread tamamen kapanana kadar main'i bekletir.
        // Kapandığında o thread'in stack belleği işletim sistemine temizce iade edilir.
        if (pthread_join(threads[i], NULL) != 0)
        {
            perror("Thread join hatası");
        }
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    extern char *httpNotFoundResponse;
    if (httpNotFoundResponse != NULL)
    {
        free(httpNotFoundResponse);
    }

    close(server_fd);
}

int server_start(int PORT)
{
    perror("error test");

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Sunucu için bir file descriptor
    struct sockaddr_in server_addr; ////Sunucu ayarlarını tutan struct şablonu
    int server_fd;
    // Server socketi oluşturma AF_INET: IPv4 SOCK_STREAM: akış tabanlı, 0: protokol: varsayılan protokolü seç
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Her türlü IP adresini kabul et
    server_addr.sin_port = htons(PORT);       // Şu PORT'u dinle

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        return 1;
    }
    // Socket'i server_fd'ye bağlama
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // BACKLOG kişilik isteklik kuyruk oluştur.
    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    initHeaders();

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    while (server_running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);

        if (client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        queue_push(client_fd);
    }

    closingRoutine(server_fd);

    return 0;
}