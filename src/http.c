
#include "http.h"

char *httppOKStatus = "HTTP/1.1 200 OK\r\n";
char *httpNotFoundStatus = "HTTP/1.1 404 Not Found\r\n";
char *httpServerErrorStatus = "HTTP/1.1 500 Internal Server Error\r\n";
char *httpBadRequestStatus = "HTTP/1.1 400 Bad Request\r\n";
char *httpTimeoutStatus = "HTTP/1.1 408 Request Timeout\r\n";
char *httpNotAllowedStatus = "HTTP/1.1 405 Method Not Allowed\r\n";

char *httpNotFoundBody = "<h1>404 Not Found</h1>";
char *httpServerErrorBody = "<h1>500 Internal Server Error</h1>";
char *httpBadRequestBody = "<h1> 400 Bad Request</h1>\r\n";
char *httpTimeoutBody = "408 Request Timeout\r\n";
char *httpNotAllowedBody = "405 Method Not Allowed";

char httpNotFoundHeaders[STATIC_RES_HEADER_SIZE];
char httpServerErrorHeaders[STATIC_RES_HEADER_SIZE];
char httpBadRequestHeaders[STATIC_RES_HEADER_SIZE];
char httpTimeoutHeaders[STATIC_RES_HEADER_SIZE];
char httpNotAllowedHeaders[STATIC_RES_HEADER_SIZE];
char *httpNotFoundResponse;
char *httpServerErrorResponse;
char *httpBadRequestResponse;
char *httpTimeoutResponse;
char *httpNotAllowedResponse;

// Not Found Response Headers tanımlanır.
void initHttpNotFoundHeaders()
{
    snprintf(httpNotFoundHeaders, sizeof(httpNotFoundHeaders),
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpNotFoundBody));

    return;
}

void initHttpHeader(char *httpHeaders, char *httpBody)
{
    snprintf(httpHeaders, STATIC_RES_HEADER_SIZE,
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpBody));
    return;
}
// Server Error Response Headers tanımlanır.
void initHttpServerErrorHeaders()
{
    initHttpHeader(httpServerErrorHeaders, httpServerErrorBody);

    return;
}

// Bad Request Response Headers tanımlanır.
void initHttpBadRequestHeaders()
{
    initHttpHeader(httpBadRequestHeaders, httpBadRequestBody);

    return;
}

// Timeout Headers tanımlanır.
void initHttpTimeoutHeaders()
{
    initHttpHeader(httpTimeoutHeaders, httpTimeoutBody);

    return;
}

// Method Not Allowed Headers tanımlanır.
void initHttpNotAllowedHeaders()
{
    initHttpHeader(httpNotAllowedHeaders, httpNotAllowedBody);

    return;
}

// Not Found Response son hali tanımlanır.
int _initHttpResponse(char *status, char *headers, char *body, char **response)
{
    size_t responseSize = strlen(status) +
                          strlen(headers) +
                          strlen(body) + 1;

    *response = calloc(responseSize, sizeof(char));

    if (*response == NULL)
    {

        return -1;
    }

    int n = snprintf(*response, responseSize,
                     "%s%s%s", status, headers, body);

    if (n <= 0 || (size_t)n >= responseSize)
    {

        free(*response);
        return -1;
    }

    return 0;
}
int initHttpNotFoundResponse()
{
    return _initHttpResponse(httpNotFoundStatus, httpNotFoundHeaders, httpNotFoundBody, &httpNotFoundResponse);
}

int initServerErrorResponse()
{
    return _initHttpResponse(httpServerErrorStatus, httpServerErrorHeaders, httpServerErrorBody, &httpServerErrorResponse);
}

// Bad Request Response son hali tanımlanır.
int initHttpBadRequestResponse()
{
    return _initHttpResponse(httpBadRequestStatus, httpBadRequestHeaders, httpBadRequestBody, &httpBadRequestResponse);
}

// Timeout Response son hali tanımlanır.
int initHttpTimeoutResponse()
{
    return _initHttpResponse(httpTimeoutStatus, httpTimeoutHeaders, httpTimeoutBody, &httpTimeoutResponse);
}

int initHttpNotAllowedResponse()
{
    return _initHttpResponse(httpNotAllowedStatus, httpNotAllowedHeaders, httpNotAllowedBody, &httpNotAllowedResponse);
}

// Request Content Length sayısını döndürür.
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

// Http Request Header bilgileri parçalanır.
int parse_http_request_header(char *buffer, HttpRequest *httpRequest)
{
    char *current_pos = buffer;
    char *next_line = strstr(current_pos, "\r\n");
    
    if (next_line == NULL)
    {
        next_line = strchr(current_pos, '\n');

        if (next_line != NULL)
        {
            LOG_INFO("\\n is found in header\n");
        }
        else
        {
            LOG_ERROR("Status line not found\n");
            return -1;
        }
    }

    size_t status_line_length = next_line - current_pos;

    char tempStatusLine[status_line_length + 1];
    strncpy(tempStatusLine, current_pos, status_line_length);
    tempStatusLine[status_line_length] = '\0';

    int parsed_count = sscanf(tempStatusLine, "%15s %255s %15s", httpRequest->method,
                              httpRequest->path, httpRequest->version);

    if (parsed_count < 3)
    {
        return -1;
    }

    httpRequest->content_length = _find_content_length(buffer);

    if (strcmp(httpRequest->method, "GET") == 0 && httpRequest->content_length > 0)
    {
        LOG_ERROR("Body cannot have body\n");
        return -1;
    }

    return 0;
}

// Http Request Body parçalanır.
int parse_http_request_body(HttpRequest *httpRequest, char *startBody, size_t body_size)
{

    httpRequest->body = calloc(body_size + 1, 1);
    if (httpRequest->body == NULL)
    {
        LOG_ERROR("request.body  calloc\n");
        return -1;
    }

    strncpy(httpRequest->body, startBody, body_size);
    return 0;
}

// Response Header dinamik olarak hazırlanır.
int prepHttpResponseHeader(HttpResponse *httpResponse)
{
    strcpy(httpResponse->statusLine, httppOKStatus);
    size_t size = sizeof(httpResponse->headers);
    int n = snprintf(httpResponse->headers, size,
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "Cache-Control: no-store\r\n"
                     "\r\n",
                     httpResponse->MIMEtype,
                     httpResponse->fileSize);

    if (n < 0 || n >= (int)size)
    {
        LOG_ERROR("Creating response header didnt work.\n");
        return -1;
    }

    return 0;
}

// Api hatalarının Response Headeri dinamik olarak hazırlanır.
int prepApiErrorHeader(HttpResponse *httpResponse)
{
    strcpy(httpResponse->statusLine, httpBadRequestStatus);
    size_t size = sizeof(httpResponse->headers);
    int n = snprintf(httpResponse->headers, size,
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "Cache-Control: no-store\r\n"
                     "\r\n",
                     httpResponse->MIMEtype,
                     httpResponse->fileSize);

    if (n < 0 || n >= (int)size)
    {
        LOG_ERROR("Creating response header didnt work.\n");
        return -1;
    }

    return 0;
}

int _sendAll(int client_fd, const char *buffer, size_t bytesRead)
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
        {
            LOG_ERROR("send() function: -1.\n");
            return -1;
        }

        bytesSent += n;
    }

    return 0;
}

int send_404Http_Response(int client_fd)
{
    return _sendAll(client_fd, httpNotFoundResponse, strlen(httpNotFoundResponse));
}

int send_Server_Error_Response(int client_fd)
{
    return _sendAll(client_fd, httpServerErrorResponse, strlen(httpServerErrorResponse));
}

int send_BadRequest_Response(int client_fd)
{
    return _sendAll(client_fd, httpBadRequestResponse, strlen(httpBadRequestResponse));
}

int send_Timeout_Response(int client_fd)
{
    return _sendAll(client_fd, httpTimeoutResponse, strlen(httpTimeoutResponse));
}

int send_NotAllowed_Response(int client_fd)
{
    return _sendAll(client_fd, httpNotAllowedResponse, strlen(httpNotAllowedResponse));
}

int send_HttpError_Response(int client_fd, int statusCode)
{
    switch (statusCode)
    {
    case 404:
        send_404Http_Response(client_fd);
        break;
    case 500:
        send_Server_Error_Response(client_fd);
        break;
    case 400:
        send_BadRequest_Response(client_fd);
        break;
    case 408:
        send_Timeout_Response(client_fd);
        break;
    case 405:
        send_NotAllowed_Response(client_fd);
        break;
    default:
        return -1;
        break;
    }
    return 0;
}

int sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse)
{
    size_t headerSize = strlen(httpResponse->statusLine) + strlen(httpResponse->headers) + 1;
    char *header = calloc(headerSize, 1);

    if (header == NULL)
    {
        LOG_ERROR("Dynamic Response header calloc\n");
        send_Server_Error_Response(client_fd);
        return -1;
    }

    snprintf(header, headerSize, "%s%s", httpResponse->statusLine, httpResponse->headers);
    _sendAll(client_fd, header, headerSize - 1);
    free(header);
    return 0;
}

int sendFileContent(int client_fd, FILE *file)
{
    char BUFFER[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(BUFFER, 1, BUFFER_SIZE, file)) > 0)
    {
        _sendAll(client_fd, BUFFER, bytesRead);
    }
    return 0;
}

int sendApiContent(int client_fd, char *json_string)
{
    size_t size = strlen(json_string);

    _sendAll(client_fd, json_string, size);

    return 0;
}
