
#include "http.h"

const char *htppOKStatus = "HTTP/1.1 200 OK\r\n";
const char *httpNotFoundStatus = "HTTP/1.1 404 Not Found\r\n";
const char *httpServerErrorStatus = "HTTP/1.1 500 Internal Server Error\r\n";
const char *httpBadRequestStatus = "HTTP/1.1 400 Bad Request\r\n";

const char *httpNotFoundBody = "<h1>404 Not Found</h1>";
const char *httpServerErrorBody = "<h1>500 Internal Server Error</h1>";
const char *httpBadRequestBody = "<h1> 400 Bad Request</h1>\r\n";

char httpNotFoundHeaders[512];
char httpServerErrorHeaders[512];
char httpBadRequestHeaders[512];

char *httpNotFoundResponse;
char *httpBadRequestResponse;
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

void initHttpBadRequestHeaders()
{
    snprintf(httpBadRequestHeaders, sizeof(httpBadRequestHeaders),
             "Content-Type: text/plain\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             strlen(httpBadRequestBody),
             httpBadRequestBody);
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

int initHttpBadRequestResponse()
{
    size_t responseSize = strlen(httpBadRequestStatus) +
                          strlen(httpBadRequestHeaders) +
                          strlen(httpBadRequestBody) + 1;

    httpBadRequestResponse = calloc(responseSize, sizeof(char));

    int n = snprintf(httpBadRequestResponse, responseSize,
                     "%s%s%s", httpBadRequestStatus, httpBadRequestHeaders, httpBadRequestBody);

    if (n <= 0 || (size_t)n >= responseSize)
    {
        printf("Response olusturulamadi veya kesildi\n");
        free(httpNotFoundResponse);
        return -1;
    }
    return 0;
}

unsigned long long _find_content_length(char *buffer)
{
    char *endptr;
    char *contentLengthPtr = strcasestr(buffer, "Content-Length:");
    if (contentLengthPtr != NULL)
    {
        char *colonPtr = strchr(contentLengthPtr, ':');
        if (colonPtr != NULL)
        {

            return strtoull(colonPtr + 1, &endptr, 10);
        }
    }
    return 0;
}

int parse_http_request_header(char *buffer, HttpRequest *httpRequest)
{
    char *current_pos = buffer;
    printf("\nCR:%s\n", buffer);
    char *next_line = strstr(current_pos, "\r\n");
    if (next_line == NULL)
    {
        next_line = strchr(current_pos, '\n');

        if (next_line != NULL)
        {
            printf("Satir sonu sadece \\n olarak bulundu!\n");
        }
    }

    char *endptr; // Content Length sayısını okuma işlemi için
    size_t status_line_length = next_line - current_pos;

    char tempStatusLine[status_line_length + 1];
    strncpy(tempStatusLine, current_pos, status_line_length);
    tempStatusLine[status_line_length] = '\0';
    printf("tempSL:%s\n", tempStatusLine);
    int parsed_count = sscanf(tempStatusLine, "%15s %255s %15s", httpRequest->method,
                              httpRequest->path, httpRequest->version);

    httpRequest->content_length = _find_content_length(buffer);

    if (strcmp(httpRequest->method, "GET") == 0)
    {
        if (httpRequest->content_length > 0)
        {
            printf("Get Body bulundu!\n");
            return -1;
        }
        printf("return0 donulecek\n");
        return 0;
    }
    else if (strcmp(httpRequest->method, "POST") == 0)
    {
        if (httpRequest->content_length > 0)
        {
            return 0;
        }
    }
}

int parse_http_request_body(char *buffer, HttpRequest *httpRequest, char *startBody, size_t body_size)
{
    printf("Gelen body_size: %zu\n", body_size);
    printf("sratBody:%s\n", startBody);
    httpRequest->body = calloc(body_size + 1, 1);
    printf("parreqbody2\n");

    strncpy(httpRequest->body, startBody, body_size);
    printf("parreqbody3\n");

    printf("httpRequest->body: %s\n", httpRequest->body);
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

int send_404Http_Response(int client_fd)
{
    return sendAll(client_fd, httpNotFoundResponse, strlen(httpNotFoundResponse));
}

int send_BadRequest_Response(int client_fd)
{
    return sendAll(client_fd, httpBadRequestResponse, strlen(httpBadRequestResponse));
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
