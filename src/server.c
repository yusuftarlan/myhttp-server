#include "server.h"
#include "http.h"

/* Global flag to control server main loop */
int server_running = 1;

/* Thread synchronization primitives for queue management */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_t threads[THREAD_COUNT];

/* Thread-safe queue for managing client file descriptors */
int queue[QUEUE_SIZE];
int front = 0;
int rear = 0;
int count = 0;

/**
 * handle_signal - Signal handler for graceful server shutdown
 * @sig: Signal number (SIGINT or SIGTERM)
 *
 * Description: Handles termination signals by setting server_running flag to 0
 *              and waking all waiting threads so they can exit gracefully.
 */
void handle_signal(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        LOG_INFO("\nClosing signal is recived. The last request is required...\n");
        server_running = 0;
        pthread_cond_broadcast(&queue_cond);
    }
}

/**
 * _safe_read_client - Safely read data from client socket with error handling
 * @client_fd: Client file descriptor
 * @tempBuffer: Buffer to store received data
 * @total_bytes: Current number of bytes in buffer
 * @remaining_space: Available space in buffer
 *
 * Return: Bytes read on success, 0 if client closed connection,
 *         -1 on read error, -2 on interrupted call, -3 on timeout
 */
ssize_t _safe_read_client(int client_fd, char *tempBuffer, size_t total_bytes, size_t remaining_space)
{
    ssize_t bytes_received = recv(client_fd, (tempBuffer + total_bytes), remaining_space, 0);

    if (bytes_received < 0)
    {
        /* Check for timeout condition */
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return -3;
        }
        /* Interrupted system call - retry needed */
        else if (errno == EINTR)
        {
            return -2;
        }

        /* Fatal read error */
        return -1;
    }
    /* Client closed the connection */
    else if (bytes_received == 0)
    {
        return 0;
    }
    else
    {
        return bytes_received;
    }
}
/**
 * _read_header - Read and parse HTTP request header from client
 * @client_fd: Client file descriptor
 * @tempBuffer: Buffer to store received data
 * @total_bytes: Pointer to track total bytes read
 * @_toutErrorCode: Pointer to store timeout error status
 *
 * Return: Pointer to end of header (\r\n\r\n) or NULL on error
 */
char *_read_header(int client_fd, char *tempBuffer, size_t *total_bytes, int *_toutErrorCode)
{
    ssize_t bytes_received = 0;
    char *endOfHeader;

    do
    {
        size_t remaining_space = MAX_REQ_SIZE - *total_bytes - 1;

        /* Check if buffer is full - prevent overflow */
        if (remaining_space <= 0)
        {
            LOG_ERROR("Request buffer is full!\n");
            break;
        }

        bytes_received = _safe_read_client(client_fd, tempBuffer, *total_bytes, remaining_space);

        /* Handle timeout - send 408 Request Timeout */
        if (bytes_received == -3)
        {
            *_toutErrorCode = 1;
            send_HttpError_Response(client_fd, 408);
            return NULL;
        }
        /* Interrupted call - retry */
        else if (bytes_received == -2)
        {
            continue;
        }
        /* Fatal read error - send 500 Internal Server Error */
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

            /* Search for end of HTTP header (\r\n\r\n) */
            endOfHeader = strstr(tempBuffer, "\r\n\r\n");

            if (endOfHeader != NULL)
            {
                return endOfHeader;
            }
        }

    } while (bytes_received > 0 && *total_bytes < MAX_HEADER_SIZE);

    return NULL;
}

/**
 * _handle_client - Process HTTP request from client
 * @client_fd: Client file descriptor
 *
 * Description: Reads HTTP header and body, parses request, and routes to appropriate handler.
 *              Handles both GET and POST requests with proper error handling.
 *
 * Return: NULL on completion
 */
static void *_handle_client(int client_fd)
{
    int _toutErrorCode = 0;
    char tempBuffer[MAX_REQ_SIZE + 1] = {0};
    size_t total_bytes = 0;

    char *endOfHeader = NULL;
    char *startBody = NULL;
    size_t header_size;

    /* Read HTTP header from client */
    endOfHeader = _read_header(client_fd, tempBuffer, &total_bytes, &_toutErrorCode);

    /* Check if header was successfully read */
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
            send_HttpError_Response(client_fd, 400);
            return NULL;
        }
    }

    /* Calculate header size */
    header_size = endOfHeader - tempBuffer;

    /* Security check: Validate header size doesn't exceed limit */
    if (header_size > MAX_REQ_SIZE)
    {
        LOG_ERROR("Response header exceeds limit size\n");
        send_HttpError_Response(client_fd, 400);
        return NULL;
    }

    /* Extract header into separate buffer */
    char headerBuffer[header_size + 1];
    strncpy(headerBuffer, tempBuffer, header_size);
    headerBuffer[header_size] = '\0';

    HttpRequest httpRequest = {0};

    /* Parse HTTP request header */
    if (parse_http_request_header(headerBuffer, &httpRequest) != 0)
    {
        send_HttpError_Response(client_fd, 400);
        free(httpRequest.body);
        return NULL;
    }

    /* Calculate body size already received in initial read */
    size_t current_body_size = total_bytes - (header_size + 4);
    if (current_body_size > 0)
    {
        startBody = endOfHeader + 4;
    }

    /* Read remaining POST body if incomplete */
    if (strcmp(httpRequest.method, "POST") == 0 && httpRequest.content_length > current_body_size)
    {
        ssize_t bytes_received = 0;
        size_t remaining_space;

        do
        {
            remaining_space = MAX_REQ_SIZE - total_bytes - 1;
            bytes_received = _safe_read_client(client_fd, tempBuffer, total_bytes, remaining_space);

            /* Handle timeout during body read */
            if (bytes_received == -3)
            {
                _toutErrorCode = 1;
                send_HttpError_Response(client_fd, 408);
                return NULL;
            }
            /* Interrupted call - retry */
            else if (bytes_received == -2)
            {
                continue;
            }
            /* Fatal read error */
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

    /* Verify body was fully received */
    if (current_body_size < httpRequest.content_length)
    {
        send_HttpError_Response(client_fd, 400);
        free(httpRequest.body);
        return NULL;
    }

    if (current_body_size > 0)
    {
        startBody = endOfHeader + 4;
    }

    /* Parse POST body if present */
    if (strcmp(httpRequest.method, "POST") == 0)
    {
        if (parse_http_request_body(&httpRequest, startBody, httpRequest.content_length) != 0)
        {
            send_HttpError_Response(client_fd, 400);
            free(httpRequest.body);
            return NULL;
        }
    }

    /* Route request to appropriate handler */
    route_request(client_fd, &httpRequest);
    free(httpRequest.body);

    return NULL;
}

/**
 * queue_push - Add client file descriptor to the queue
 * @client_fd: Client file descriptor to enqueue
 *
 * Description: Thread-safe operation that adds a client connection to the work queue.
 *              If queue is full, client connection is closed.
 */
void queue_push(int client_fd)
{
    pthread_mutex_lock(&queue_mutex);

    /* Queue is full - reject new connection */
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

/**
 * queue_pop - Remove and return client file descriptor from queue
 *
 * Description: Blocking operation that waits for a client in the queue.
 *              Respects server_running flag for graceful shutdown.
 *
 * Return: Client file descriptor or -1 if server shutting down with empty queue
 */
int queue_pop()
{
    pthread_mutex_lock(&queue_mutex);

    /* Wait for client in queue (also checks server_running for graceful shutdown) */
    while (count == 0 && server_running == 1)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    /* Exit if server shutdown signal received and no more clients */
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

/**
 * set_recv_timeout - Set receive timeout for socket
 * @client_fd: Client file descriptor
 * @second: Timeout in seconds
 *
 * Description: Configures SO_RCVTIMEO socket option to prevent indefinite blocking.
 */
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

/**
 * worker_thread - Worker thread function for processing client requests
 *
 * Description: Continuously polls the queue for clients and processes their requests.
 *              Exits gracefully when server_running is set to 0 and queue is empty.
 *
 * Return: NULL on thread completion
 */
void *worker_thread()
{
    while (1)
    {
        /* Get next client from queue (blocks if queue is empty) */
        int client_fd = queue_pop();

        /* Exit thread if server shutdown signal received */
        if (client_fd == -1)
        {
            break;
        }

        _handle_client(client_fd);
        close(client_fd);
    }

    return NULL;
}

/**
 * initHeaders - Initialize all HTTP response headers and bodies
 *
 * Description: Sets up template HTTP response messages for various error codes.
 *              Must be called before server starts accepting connections.
 */
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

/**
 * closingRoutine - Clean shutdown of server resources
 * @server_fd: Server socket file descriptor
 *
 * Description: Gracefully shuts down all threads, destroys synchronization primitives,
 *              frees allocated memory for HTTP response templates, and closes server socket.
 */
void closingRoutine(int server_fd)
{
    /* Wait for all worker threads to complete */
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            LOG_ERROR("Thread join: %d\n", i);
        }
    }

    /* Destroy thread synchronization primitives */
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    /* Free allocated HTTP response buffers */
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

/**
 * server_start - Initialize and run HTTP server
 * @PORT: Port number to listen on
 *
 * Description: Sets up server socket, creates worker threads, and enters main accept loop.
 *              Handles client connections and queues them for processing.
 *              Listens for SIGINT/SIGTERM for graceful shutdown.
 *
 * Return: 0 on success, 1 on initialization failure
 */
int server_start(int PORT)
{
    /* Register signal handlers for graceful shutdown */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* Initialize server address structure */
    struct sockaddr_in server_addr;
    int server_fd;

    /* Create server socket (IPv4, TCP) */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* Allow socket reuse to avoid TIME_WAIT state */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        return 1;
    }

    /* Bind socket to port */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Listen for incoming connections */
    if (listen(server_fd, BACKLOG) < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Initialize HTTP response templates */
    initHeaders();

    /* Create worker threads */
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    LOG_INFO("Server is starting: %d\n", PORT);

    /* Main accept loop - accept connections until shutdown signal */
    while (server_running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        /* Accept incoming client connection */
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);

        if (client_fd < 0)
        {
            continue;
        }

        /* Set timeout for socket operations (5 seconds) */
        set_recv_timeout(client_fd, 5);

        /* Enqueue client for processing */
        queue_push(client_fd);
    }

    /* Clean up and exit */
    closingRoutine(server_fd);
    LOG_INFO("Server closed \n");

    return 0;
}