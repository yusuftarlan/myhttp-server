#include "server.h"
#include "http.h"

int server_running = 1;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Kuyruğu trade safe yapmak için
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;    // Thread durum yönetimi için
pthread_t threads[THREAD_COUNT];                         // Threadleri yönetmek için dizi

int queue[QUEUE_SIZE]; // Client_fd 'leri tutan trade safe dizi
int front = 0;
int rear = 0;
int count = 0;

void handle_signal(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        LOG_INFO("\nClosing signal is recived. The last request is required...\n");
        server_running = 0;
        pthread_cond_broadcast(&queue_cond);
    }
}

ssize_t _safe_read_client(int client_fd, char *tempBuffer, size_t total_bytes, size_t remaining_space)
{
    ssize_t bytes_received = recv(client_fd, (tempBuffer + total_bytes), remaining_space, 0);

    if (bytes_received < 0)
    {
        // Timeout oldu
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return -3; // send_request_timeout(client_fd);
        }
        else if (errno == EINTR)
        {
            return -2; // Tekrar denenebilir.
        }

        return -1; // recv() Okuma hatası
    }
    else if (bytes_received == 0)
    {
        // Client bağlantı kapattı
        return 0;
    }
    else
    {
        return bytes_received;
    }
}
char *_read_header(int client_fd, char *tempBuffer, size_t *total_bytes, int *_toutErrorCode)
{

    ssize_t bytes_received = 0;
    char *endOfHeader;

    do
    {
        size_t remaining_space = MAX_REQ_SIZE - *total_bytes - 1;

        // Eğer buffer dolduysa daha fazla okuma yapma
        if (remaining_space <= 0)
        {
            LOG_ERROR("Request buffer is full!\n");

            break;
        }

        bytes_received = _safe_read_client(client_fd, tempBuffer, *total_bytes, remaining_space);
        if (bytes_received == -3)
        {
            *_toutErrorCode = 1;
            send_HttpError_Response(client_fd, 408);
            return NULL;
        }
        else if (bytes_received == -2)
        {
            continue;
        }
        else if (bytes_received == -1)
        {
            *_toutErrorCode = 1;
            send_HttpError_Response(client_fd, 500);
            return NULL;
        }
        else
        {
            *total_bytes = *total_bytes + bytes_received;
            tempBuffer[*total_bytes] = '\0';

            endOfHeader = strstr(tempBuffer, "\r\n\r\n");

            if (endOfHeader != NULL)
            {
                return endOfHeader;
            }
        }

    } while (bytes_received > 0 && *total_bytes < MAX_HEADER_SIZE);

    return NULL;
}
static void *_handle_client(int client_fd)
{
    int _toutErrorCode = 0;
    char tempBuffer[MAX_REQ_SIZE + 1] = {0};
    size_t total_bytes = 0;

    char *endOfHeader = NULL;
    char *startBody = NULL;
    size_t header_size;

    // HEADER SONUNA KADAR BULUNMAYA ÇALIŞILIR.
    endOfHeader = _read_header(client_fd, tempBuffer, &total_bytes, &_toutErrorCode);

    if (endOfHeader == NULL)
    {
        if (_toutErrorCode == 1)
        {
            LOG_ERROR("Recv() timeout is reached\n");
            return NULL;
        }
        else
        {
            LOG_ERROR("Response header not found\n");
            send_HttpError_Response(client_fd, 400); // bad request

            return NULL;
        }
    }

    header_size = endOfHeader - tempBuffer;

    // GÜVENLİK: Header boyutunun sınırı aşmasını engelleyen kontrolü aktif etmelisiniz!
    if (header_size > MAX_REQ_SIZE)
    {
        LOG_ERROR("Response header exceeds limit size\n");
        send_HttpError_Response(client_fd, 400);
        return NULL;
    }

    // Sadece header'ı tutacak buffer
    char headerBuffer[header_size + 1];
    strncpy(headerBuffer, tempBuffer, header_size);
    headerBuffer[header_size] = '\0';

    HttpRequest httpRequest = {0};

    if (parse_http_request_header(headerBuffer, &httpRequest) != 0)
    {
        send_HttpError_Response(client_fd, 400);
        free(httpRequest.body);
        return NULL;
    }

    // DÜZELTME 2: Şu ana kadar recv ile okunmuş olan mevcut body miktarını hesapla
    size_t current_body_size = total_bytes - (header_size + 4);
    if (current_body_size > 0)
    {
        startBody = endOfHeader + 4; // \r\n\r\n atlanarak body'nin başlangıcı bulunur
    }

    // Eğer POST body eksik kalmışsa
    if (strcmp(httpRequest.method, "POST") == 0 && httpRequest.content_length > current_body_size)
    {
        ssize_t bytes_received = 0;
        size_t remaining_space;
        do
        {
            remaining_space = MAX_REQ_SIZE - total_bytes - 1;
            bytes_received = _safe_read_client(client_fd, tempBuffer, total_bytes, remaining_space);
            if (bytes_received == -3)
            {
                _toutErrorCode = 1;
                send_HttpError_Response(client_fd, 408);
                return NULL;
            }
            else if (bytes_received == -2)
            {
                continue;
            }
            else if (bytes_received == -1)
            {
                _toutErrorCode = 1;
                send_HttpError_Response(client_fd, 500);
                return NULL;
            }
            else
            {
                current_body_size = current_body_size + bytes_received;
                total_bytes = total_bytes + bytes_received;
                tempBuffer[total_bytes] = '\0';
            }
        } while (current_body_size < httpRequest.content_length && total_bytes < MAX_REQ_SIZE);
    }

    if (current_body_size < httpRequest.content_length)
    {
        send_HttpError_Response(client_fd, 400);
        free(httpRequest.body);
        return NULL;
    }

    if (current_body_size > 0)
    {
        startBody = endOfHeader + 4; // \r\n\r\n atlanarak body'nin başlangıcı bulunur
    }

    // Eğer post ise body'yi ayıkla
    if (strcmp(httpRequest.method, "POST") == 0)
    {
        if (parse_http_request_body(&httpRequest, startBody, httpRequest.content_length) != 0)
        {
            send_HttpError_Response(client_fd, 400);
            free(httpRequest.body);
            return NULL;
        }
    }

    route_request(client_fd, &httpRequest);
    free(httpRequest.body);
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

    // Eğer sunucu kapanma sinyali verdiyse VE işlenecek iş kalmadıysa çık!
    if (server_running == 0 && count == 0)
    {
        pthread_mutex_unlock(&queue_mutex);
        return -1;
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
        int client_fd = queue_pop(); // Kuyruktan iş çekmeye çalışır.

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
    initHttpTimeoutHeaders();
    initHttpNotAllowedHeaders();

    initHttpNotFoundResponse();
    initHttpBadRequestResponse();
    initServerErrorResponse();
    initHttpTimeoutResponse();
    initHttpNotAllowedResponse();
}

void closingRoutine(int server_fd)
{
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        // pthread_join, ilgili thread tamamen kapanana kadar main'i bekletir.
        // Kapandığında o thread'in stack belleği işletim sistemine temizce iade edilir.
        if (pthread_join(threads[i], NULL) != 0)
        {
            LOG_ERROR("Thread join: %d\n", i);
        }
    }
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    if (httpNotFoundResponse != NULL)
    {
        free(httpNotFoundResponse);
    }
    if (httpBadRequestResponse != NULL)
    {
        free(httpBadRequestResponse);
    }
    if (httpServerErrorResponse)
    {
        free(httpServerErrorResponse);
    }
    if (httpTimeoutResponse)
    {
        free(httpTimeoutResponse);
    }

    close(server_fd);
}

int server_start(int PORT)
{

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct sockaddr_in server_addr; ////Sunucu ayarlarını tutan struct şablonu
    int server_fd;                  // Sunucu için bir file descriptor
    // Server socketi oluşturma AF_INET: IPv4 SOCK_STREAM: akış tabanlı, 0: protokol: varsayılan protokolü seç
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Her türlü IP adresini kabul et
    server_addr.sin_port = htons(PORT);       // Şu PORT'u dinle

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {

        return 1;
    }
    // Socket'i server_fd'ye bağlama
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {

        exit(EXIT_FAILURE);
    }

    // BACKLOG kişilik isteklik kuyruk oluştur.
    if (listen(server_fd, BACKLOG) < 0)
    {

        exit(EXIT_FAILURE);
    }

    initHeaders();

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    LOG_INFO("Server is starting: %d\n", PORT);
    while (server_running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);
        if (client_fd < 0)
        {

            continue;
        }

        set_recv_timeout(client_fd, 5);

        queue_push(client_fd);
    }

    closingRoutine(server_fd);
    LOG_INFO("Server closed \n");
    return 0;
}