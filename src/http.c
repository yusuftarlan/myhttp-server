
#include "http.h"

const char *htppOKStatus = "HTTP/1.1 200 OK\r\n";
const char *httpNotFoundStatus = "HTTP/1.1 404 Not Found\r\n";
const char *httpServerErrorStatus = "HTTP/1.1 500 Internal Server Error\r\n";
const char *httpNotFoundBody = "<h1>404 Not Found</h1>";
const char *httpServerErrorBody = "<h1>500 Internal Server Error</h1>";

char httpNotFoundHeaders[512];
char httpServerErrorHeaders[512];

char *httpNotFoundResponse;

void initHttpNotFoundHeaders()
{
    snprintf(httpNotFoundHeaders, sizeof(httpNotFoundHeaders),
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpNotFoundBody));
}

void initHttpServerErrorHeaders()
{
    snprintf(httpServerErrorHeaders, sizeof(httpServerErrorHeaders),
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpServerErrorBody));
}

int initHttpNotFoundResponse()
{
    size_t responseSize = strlen(httpNotFoundStatus) +
                          strlen(httpNotFoundHeaders) +
                          strlen(httpNotFoundBody) + 1;

    httpNotFoundResponse = calloc(responseSize, sizeof(char));

    int n = snprintf(httpNotFoundResponse, responseSize,
                     "%s%s%s", httpNotFoundStatus, httpNotFoundHeaders, httpNotFoundBody);

    if (n <= 0 || (size_t)n >= responseSize)
    {
        printf("Response olusturulamadi veya kesildi\n");
        free(httpNotFoundResponse);
        return -1;
    }
    return 0;
}

int parse_http_request(const char *buffer, HttpRequest *request)
{
    if (buffer == NULL || request == NULL)
    {
        return -1;
    }

    memset(request, 0, sizeof(HttpRequest));

    int result = sscanf(
        buffer,
        "%15s %255s %15s",
        request->method,
        request->path,
        request->version);

    if (result != 3)
    {
        return -1;
    }

    return 0;
}

int prepHttpResponseHeader(HttpResponse *httpResponse)
{
    strcpy(httpResponse->statusLine, htppOKStatus);
    return snprintf(httpResponse->headers, sizeof(httpResponse->headers),
                    "Content-Type: %s\r\n"
                    "Content-Length: %ld\r\n"
                    "Connection: close\r\n"
                    "Cache-Control: public, max-age=31536000\r\n"
                    "\r\n",
                    httpResponse->MIMEtype,
                    httpResponse->fileSize);
}

int sendAll(int client_fd, const char *buffer, size_t bytesRead)
{
    size_t bytesSent = 0;
    size_t bytesRemaining;
    ssize_t n;
    const char *bufferStartPoint;

    while (bytesSent < bytesRead)
    {
        bufferStartPoint = buffer + bytesSent;
        bytesRemaining = bytesRead - bytesSent;
        n = send(client_fd, bufferStartPoint, bytesRemaining, 0);

        if (n <= 0)
            return -1;

        bytesSent += n;
    }

    return 0;
}

int send404HttpResponse(int client_fd)
{
    return sendAll(client_fd, httpNotFoundResponse, strlen(httpNotFoundResponse));
}

void sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse)
{
    size_t headerSize = strlen(httpResponse->statusLine) + strlen(httpResponse->headers) + 1;
    char *header = calloc(headerSize, 1);
    printf("HeaderSize:%zu", headerSize);
    snprintf(header, headerSize, "%s%s", httpResponse->statusLine, httpResponse->headers);

    printf("header:%s ve size:%zu\n", header, strlen(header));
    sendAll(client_fd, header, headerSize - 1);
    free(header);
}

int sendFileContent(int client_fd, FILE *file)
{
    char BUFFER[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(BUFFER, 1, BUFFER_SIZE, file)) > 0)
    {
        sendAll(client_fd, BUFFER, bytesRead);
    }
    return bytesRead;
}
