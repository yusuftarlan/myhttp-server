
#include "http.h"

/* HTTP status lines for various response codes */
char *httppOKStatus = "HTTP/1.1 200 OK\r\n";
char *httpNotFoundStatus = "HTTP/1.1 404 Not Found\r\n";
char *httpServerErrorStatus = "HTTP/1.1 500 Internal Server Error\r\n";
char *httpBadRequestStatus = "HTTP/1.1 400 Bad Request\r\n";
char *httpTimeoutStatus = "HTTP/1.1 408 Request Timeout\r\n";
char *httpNotAllowedStatus = "HTTP/1.1 405 Method Not Allowed\r\n";

/* HTTP response body messages */
char *httpNotFoundBody = "<h1>404 Not Found</h1>";
char *httpServerErrorBody = "<h1>500 Internal Server Error</h1>";
char *httpBadRequestBody = "<h1> 400 Bad Request</h1>\r\n";
char *httpTimeoutBody = "408 Request Timeout\r\n";
char *httpNotAllowedBody = "405 Method Not Allowed";

/* HTTP response header buffers */
char httpNotFoundHeaders[STATIC_RES_HEADER_SIZE];
char httpServerErrorHeaders[STATIC_RES_HEADER_SIZE];
char httpBadRequestHeaders[STATIC_RES_HEADER_SIZE];
char httpTimeoutHeaders[STATIC_RES_HEADER_SIZE];
char httpNotAllowedHeaders[STATIC_RES_HEADER_SIZE];

/* Complete HTTP response buffers (status + headers + body) */
char *httpNotFoundResponse;
char *httpServerErrorResponse;
char *httpBadRequestResponse;
char *httpTimeoutResponse;
char *httpNotAllowedResponse;

/**
 * initHttpNotFoundHeaders - Initialize HTTP 404 Not Found response headers
 *
 * Description: Populates httpNotFoundHeaders buffer with Content-Type,
 *              Content-Length, and Connection headers for 404 responses.
 */
void initHttpNotFoundHeaders()
{
    snprintf(httpNotFoundHeaders, sizeof(httpNotFoundHeaders),
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpNotFoundBody));
}

/**
 * initHttpHeader - Generic HTTP header initialization helper
 * @httpHeaders: Buffer to store formatted headers
 * @httpBody: Response body to calculate Content-Length from
 *
 * Description: Formats HTTP headers with Content-Type, Content-Length,
 *              and Connection directives.
 */
void initHttpHeader(char *httpHeaders, char *httpBody)
{
    snprintf(httpHeaders, STATIC_RES_HEADER_SIZE,
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             strlen(httpBody));
}

/**
 * initHttpServerErrorHeaders - Initialize HTTP 500 Internal Server Error headers
 */
void initHttpServerErrorHeaders()
{
    initHttpHeader(httpServerErrorHeaders, httpServerErrorBody);
}

/**
 * initHttpBadRequestHeaders - Initialize HTTP 400 Bad Request headers
 */
void initHttpBadRequestHeaders()
{
    initHttpHeader(httpBadRequestHeaders, httpBadRequestBody);
}

/**
 * initHttpTimeoutHeaders - Initialize HTTP 408 Request Timeout headers
 */
void initHttpTimeoutHeaders()
{
    initHttpHeader(httpTimeoutHeaders, httpTimeoutBody);
}

/**
 * initHttpNotAllowedHeaders - Initialize HTTP 405 Method Not Allowed headers
 */
void initHttpNotAllowedHeaders()
{
    initHttpHeader(httpNotAllowedHeaders, httpNotAllowedBody);
}

/**
 * _initHttpResponse - Generic HTTP response initialization helper
 * @status: HTTP status line (e.g., "HTTP/1.1 404 Not Found\r\n")
 * @headers: Formatted HTTP headers
 * @body: Response body HTML content
 * @response: Pointer to store allocated response buffer
 *
 * Description: Allocates memory and combines status line, headers, and body
 *              into a complete HTTP response ready to send.
 *
 * Return: 0 on success, -1 on memory allocation or formatting error
 */
int _initHttpResponse(char *status, char *headers, char *body, char **response)
{
    /* Calculate total response size */
    size_t responseSize = strlen(status) +
                          strlen(headers) +
                          strlen(body) + 1;

    /* Allocate memory for complete response */
    *response = calloc(responseSize, sizeof(char));

    if (*response == NULL)
    {
        return -1;
    }

    /* Format complete response */
    int n = snprintf(*response, responseSize,
                     "%s%s%s", status, headers, body);

    /* Verify formatting success */
    if (n <= 0 || (size_t)n >= responseSize)
    {
        free(*response);
        return -1;
    }

    return 0;
}

/**
 * initHttpNotFoundResponse - Initialize complete 404 Not Found response
 */
int initHttpNotFoundResponse()
{
    return _initHttpResponse(httpNotFoundStatus, httpNotFoundHeaders, httpNotFoundBody, &httpNotFoundResponse);
}

/**
 * initServerErrorResponse - Initialize complete 500 Internal Server Error response
 */
int initServerErrorResponse()
{
    return _initHttpResponse(httpServerErrorStatus, httpServerErrorHeaders, httpServerErrorBody, &httpServerErrorResponse);
}

/**
 * initHttpBadRequestResponse - Initialize complete 400 Bad Request response
 */
int initHttpBadRequestResponse()
{
    return _initHttpResponse(httpBadRequestStatus, httpBadRequestHeaders, httpBadRequestBody, &httpBadRequestResponse);
}

/**
 * initHttpTimeoutResponse - Initialize complete 408 Request Timeout response
 */
int initHttpTimeoutResponse()
{
    return _initHttpResponse(httpTimeoutStatus, httpTimeoutHeaders, httpTimeoutBody, &httpTimeoutResponse);
}

/**
 * initHttpNotAllowedResponse - Initialize complete 405 Method Not Allowed response
 */
int initHttpNotAllowedResponse()
{
    return _initHttpResponse(httpNotAllowedStatus, httpNotAllowedHeaders, httpNotAllowedBody, &httpNotAllowedResponse);
}

/**
 * _find_content_length - Extract Content-Length from HTTP header
 * @buffer: HTTP header buffer
 *
 * Description: Searches for Content-Length header field and parses its numeric value.
 *
 * Return: Content-Length value or 0 if not found
 */
unsigned long long _find_content_length(char *buffer)
{
    char *endptr;

    /* Search for Content-Length header (case-insensitive) */
    char *contentLengthPtr = strcasestr(buffer, "Content-Length:");
    if (contentLengthPtr != NULL)
    {
        /* Find colon separator */
        char *colonPtr = strchr(contentLengthPtr, ':');
        if (colonPtr != NULL)
        {
            /* Parse numeric value after colon */
            return strtoull(colonPtr + 1, &endptr, 10);
        }
    }

    return 0;
}

/**
 * parse_http_request_header - Parse HTTP request header line and fields
 * @buffer: HTTP header buffer
 * @httpRequest: Pointer to HttpRequest structure to populate
 *
 * Description: Extracts method, path, version from request line.
 *              Also retrieves Content-Length and validates GET requests.
 *
 * Return: 0 on success, -1 on parse error or validation failure
 */
int parse_http_request_header(char *buffer, HttpRequest *httpRequest)
{
    char *current_pos = buffer;

    /* Find first line terminator (CRLF preferred) */
    char *next_line = strstr(current_pos, "\r\n");

    if (next_line == NULL)
    {
        next_line = strchr(current_pos, '\n');

        if (next_line != NULL)
        {
            LOG_INFO("LF line ending found in header\n");
        }
        else
        {
            LOG_ERROR("Status line not found\n");
            return -1;
        }
    }

    /* Extract status line (METHOD PATH VERSION) */
    size_t status_line_length = next_line - current_pos;

    char tempStatusLine[status_line_length + 1];
    strncpy(tempStatusLine, current_pos, status_line_length);
    tempStatusLine[status_line_length] = '\0';

    /* Parse status line components */
    int parsed_count = sscanf(tempStatusLine, "%15s %255s %15s", httpRequest->method,
                              httpRequest->path, httpRequest->version);

    if (parsed_count < 3)
    {
        return -1;
    }

    /* Extract Content-Length header */
    httpRequest->content_length = _find_content_length(buffer);

    /* Validate: GET requests must not have body */
    if (strcmp(httpRequest->method, "GET") == 0 && httpRequest->content_length > 0)
    {
        LOG_ERROR("GET request cannot have body\n");
        return -1;
    }

    return 0;
}

/**
 * parse_http_request_body - Extract and store HTTP request body
 * @httpRequest: Pointer to HttpRequest structure to populate
 * @startBody: Pointer to start of body in buffer
 * @body_size: Number of bytes in body
 *
 * Description: Allocates memory and copies request body for further processing.
 *
 * Return: 0 on success, -1 on memory allocation failure
 */
int parse_http_request_body(HttpRequest *httpRequest, char *startBody, size_t body_size)
{
    /* Allocate buffer for request body */
    httpRequest->body = calloc(body_size + 1, 1);
    if (httpRequest->body == NULL)
    {
        LOG_ERROR("Failed to allocate request body buffer\n");
        return -1;
    }

    /* Copy body data */
    strncpy(httpRequest->body, startBody, body_size);
    return 0;
}

/**
 * prepHttpResponseHeader - Format successful HTTP response header
 * @httpResponse: Pointer to HttpResponse structure
 *
 * Description: Populates status line and headers with 200 OK and content metadata.
 *              Sets MIME type and file size for content delivery.
 *
 * Return: 0 on success, -1 on formatting error
 */
int prepHttpResponseHeader(HttpResponse *httpResponse)
{
    strcpy(httpResponse->statusLine, httppOKStatus);
    size_t size = sizeof(httpResponse->headers);

    /* Format response headers with content metadata */
    int n = snprintf(httpResponse->headers, size,
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "Cache-Control: no-store\r\n"
                     "\r\n",
                     httpResponse->MIMEtype,
                     httpResponse->fileSize);

    /* Verify formatting success */
    if (n < 0 || n >= (int)size)
    {
        LOG_ERROR("Failed to format response header\n");
        return -1;
    }

    return 0;
}

/**
 * prepApiErrorHeader - Format HTTP error response header for API
 * @httpResponse: Pointer to HttpResponse structure
 *
 * Description: Populates status line with 400 Bad Request and error metadata.
 *
 * Return: 0 on success, -1 on formatting error
 */
int prepApiErrorHeader(HttpResponse *httpResponse)
{
    strcpy(httpResponse->statusLine, httpBadRequestStatus);
    size_t size = sizeof(httpResponse->headers);

    /* Format error response headers */
    int n = snprintf(httpResponse->headers, size,
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "Cache-Control: no-store\r\n"
                     "\r\n",
                     httpResponse->MIMEtype,
                     httpResponse->fileSize);

    /* Verify formatting success */
    if (n < 0 || n >= (int)size)
    {
        LOG_ERROR("Failed to format error response header\n");
        return -1;
    }

    return 0;
}

/**
 * _sendAll - Send complete buffer to socket (handles partial sends)
 * @client_fd: Client socket file descriptor
 * @buffer: Data buffer to send
 * @bytesRead: Total bytes to send
 *
 * Description: Guarantees all bytes are sent by handling partial writes.
 *              Continues sending until all data is transmitted or error occurs.
 *
 * Return: 0 on success, -1 on send error
 */
int _sendAll(int client_fd, const char *buffer, size_t bytesRead)
{
    size_t bytesSent = 0;
    size_t bytesRemaining;
    ssize_t n;
    const char *bufferStartPoint;

    /* Send all bytes, handling partial sends */
    while (bytesSent < bytesRead)
    {
        bufferStartPoint = buffer + bytesSent;
        bytesRemaining = bytesRead - bytesSent;
        n = send(client_fd, bufferStartPoint, bytesRemaining, 0);

        /* Handle send error */
        if (n <= 0)
        {
            LOG_ERROR("send() failed\n");
            return -1;
        }

        bytesSent += n;
    }

    return 0;
}

/**
 * send_404Http_Response - Send HTTP 404 Not Found response
 */
int send_404Http_Response(int client_fd)
{
    return _sendAll(client_fd, httpNotFoundResponse, strlen(httpNotFoundResponse));
}

/**
 * send_Server_Error_Response - Send HTTP 500 Internal Server Error response
 */
int send_Server_Error_Response(int client_fd)
{
    return _sendAll(client_fd, httpServerErrorResponse, strlen(httpServerErrorResponse));
}

/**
 * send_BadRequest_Response - Send HTTP 400 Bad Request response
 */
int send_BadRequest_Response(int client_fd)
{
    return _sendAll(client_fd, httpBadRequestResponse, strlen(httpBadRequestResponse));
}

/**
 * send_Timeout_Response - Send HTTP 408 Request Timeout response
 */
int send_Timeout_Response(int client_fd)
{
    return _sendAll(client_fd, httpTimeoutResponse, strlen(httpTimeoutResponse));
}

/**
 * send_NotAllowed_Response - Send HTTP 405 Method Not Allowed response
 */
int send_NotAllowed_Response(int client_fd)
{
    return _sendAll(client_fd, httpNotAllowedResponse, strlen(httpNotAllowedResponse));
}

/**
 * send_HttpError_Response - Send appropriate error response based on status code
 * @client_fd: Client socket file descriptor
 * @statusCode: HTTP status code (404, 500, 400, 408, 405)
 *
 * Description: Routes to specific error handler based on status code.
 *
 * Return: 0 on success, -1 on invalid status code
 */
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

/**
 * sendHttpResponseHeader - Send HTTP response status line and headers
 * @client_fd: Client socket file descriptor
 * @httpResponse: Pointer to HttpResponse with formatted headers
 *
 * Description: Combines status line and headers, then transmits to client.
 *
 * Return: 0 on success, -1 on memory or send error
 */
int sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse)
{
    size_t headerSize = strlen(httpResponse->statusLine) + strlen(httpResponse->headers) + 1;

    /* Allocate temporary buffer for combined headers */
    char *header = calloc(headerSize, 1);
    if (header == NULL)
    {
        LOG_ERROR("Failed to allocate response header buffer\n");
        send_Server_Error_Response(client_fd);
        return -1;
    }

    /* Format and send headers */
    snprintf(header, headerSize, "%s%s", httpResponse->statusLine, httpResponse->headers);
    _sendAll(client_fd, header, headerSize - 1);
    free(header);

    return 0;
}

/**
 * sendFileContent - Send complete file content to client
 * @client_fd: Client socket file descriptor
 * @file: File pointer to read from
 *
 * Description: Reads file in chunks and sends all data to client.
 *              Handles partial sends transparently.
 *
 * Return: 0 on success
 */
int sendFileContent(int client_fd, FILE *file)
{
    char BUFFER[BUFFER_SIZE];
    size_t bytesRead;

    /* Read and transmit file in chunks */
    while ((bytesRead = fread(BUFFER, 1, BUFFER_SIZE, file)) > 0)
    {
        _sendAll(client_fd, BUFFER, bytesRead);
    }

    return 0;
}

/**
 * sendApiContent - Send JSON API response to client
 * @client_fd: Client socket file descriptor
 * @json_string: JSON response string
 *
 * Description: Transmits JSON formatted API response.
 *
 * Return: 0 on success
 */
int sendApiContent(int client_fd, char *json_string)
{
    size_t size = strlen(json_string);

    _sendAll(client_fd, json_string, size);

    return 0;
}
