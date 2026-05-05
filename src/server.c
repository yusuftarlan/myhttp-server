#include "server.h"
#include "http.h"

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

char *_read_header(int client_fd, char *tempBuffer, size_t *total_bytes)
{

    ssize_t bytes_received = 0;

    char *endOfHeader;
    do
    {
        size_t remaining_space = MAX_REQ_SIZE - *total_bytes - 1;

        // Eğer buffer dolduysa daha fazla okuma yapma
        if (remaining_space <= 0)
        {
            fprintf(stderr, "Hata: Buffer tamamen doldu!\n");
            break;
            ;
        }
        printf("%zu..%zu..\n", *total_bytes, remaining_space);
        bytes_received = recv(client_fd, (tempBuffer + *total_bytes), remaining_space, 0);

        // Hata var
        if (bytes_received < 0)
        {
            // Timeout oldu
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // send_request_timeout(client_fd);
                close(client_fd);
                return NULL;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else
            {
                // Gerçek recv hatası
                perror("recv");
                close(client_fd);
                return NULL;
            }
        }
        else if (bytes_received == 0)
        {
            // Client bağlantıyı kapattı
            close(client_fd);
            return NULL;
        }
        else
        {
            printf("BUFFER YAZILDI!\n");
            printf("%s\n", tempBuffer);
            *total_bytes = *total_bytes + bytes_received;
            tempBuffer[*total_bytes] = '\0';

            endOfHeader = strstr(tempBuffer, "\r\n\r\n");

            if (endOfHeader != NULL)
            {
                printf("endofheaderbulundu\n");
                return endOfHeader;
            }
        }

    } while (bytes_received > 0 && *total_bytes < MAX_HEADER_SIZE);
}
static void *_handle_client(int client_fd)
{
    char tempBuffer[MAX_REQ_SIZE + 1] = {0};
    size_t total_bytes = 0;
    char *endOfHeader = NULL;
    char *startBody = NULL;
    size_t header_size;

    // HEADER SONUNA KADAR BULUNMAYA ÇALIŞILIR.
    // endOfRequest parametresine artık gerek yok, kaldırdık.
    endOfHeader = _read_header(client_fd, tempBuffer, &total_bytes);

    if (endOfHeader != NULL)
    {
        header_size = endOfHeader - tempBuffer;

        startBody = endOfHeader + 4; // \r\n\r\n atlanarak body'nin başlangıcı bulunur
        printf("startbodyAtandi: %zu\n", (startBody - endOfHeader));
    }
    else
    {
        fprintf(stderr, "Hata: Header sonu (\\r\\n\\r\\n) bulunamadi veya baglanti koptu.\n");
        close(client_fd);
        return NULL;
    }

    // GÜVENLİK: Header boyutunun sınırı aşmasını engelleyen kontrolü aktif etmelisiniz!
    if (header_size > MAX_REQ_SIZE)
    {
        // send_BadRequest_Response(client_fd); // Fonksiyonunuzu aktif edin
        close(client_fd);
        return NULL;
    }

    // Sadece header'ı tutacak buffer
    char headerBuffer[header_size + 1];

    // DÜZELTME 1: total_bytes KADAR DEĞİL, header_size KADAR KOPYALA
    strncpy(headerBuffer, tempBuffer, header_size);
    headerBuffer[header_size] = '\0'; // strncpy sonuna \0 koymayabilir, biz garantiye alıyoruz

    HttpRequest httpRequest;
    if (parse_http_request_header(headerBuffer, &httpRequest) != 0)
    {
        // send_BadRequest_Response(client_fd);
        close(client_fd);
        return NULL;
    }

    // DÜZELTME 2: Şu ana kadar recv ile okunmuş olan mevcut body miktarını hesapla
    size_t current_body_size = total_bytes - (header_size + 4);

    // Body'yi ayıkla (Önceki Valgrind hatalarını çözdüğümüz fonksiyonunuz)
    parse_http_request_body(tempBuffer, &httpRequest, startBody, httpRequest.content_length);

    if (strcmp(httpRequest.method, "POST") == 0)
    {
        // DÜZELTME 3: Eğer beklenen boyut, elimizdeki boyuttan BÜYÜKSE okumaya devam et
        if (httpRequest.content_length > current_body_size)
        {
            // EKSİK BODY'İ TAMAMLAMA MANTIĞI BURAYA GELECEK
            // Geri kalan (httpRequest.content_length - current_body_size) kadar byte'ı
            // recv() ile bir while döngüsü içinde okuyup body'ye eklemelisiniz.
        }
    }

    route_request(client_fd, &httpRequest);

    close(client_fd);
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

void set_recv_timeout(int client_fd, int second)
{
    struct timeval tv;
    tv.tv_sec = second;
    tv.tv_usec = 0;

    setsockopt(client_fd,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &tv,
               sizeof(tv));
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
    initHttpBadRequestHeaders();
    initHttpNotFoundResponse();
    initHttpBadRequestResponse();
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

        set_recv_timeout(client_fd, 5);

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