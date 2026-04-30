#include "server.h"

#define BACKLOG 10
#define BUFFER_SIZE 4096

void *handle_client(void *arg)
{
    int client_fd = *((int *)arg);
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
    close(client_fd);
    return NULL;
}

int server_start(int PORT)
{
    int server_fd;                  // Sunucu için bir file descriptor
    struct sockaddr_in server_addr; ////Sunucu ayarlarını tutan struct şablonu

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

    while (1)
    {
        struct sockaddr_in client_addr; // Alıcı bilgilerini tutan struct şablonu
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int)); // Alıcı (client) için file descriptor

        if (client_fd == NULL)
        {
            perror("malloc failed");
            continue;
        }

        *client_fd = accept(server_fd,
                            (struct sockaddr *)&client_addr,
                            &client_addr_len);

        if (*client_fd < 0)
        {
            perror("accept failed");
            free(client_fd);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_fd) != 0)
        {
            perror("pthread_create failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}