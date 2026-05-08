
#ifndef HTTP_H
#define HTTP_H

#define _GNU_SOURCE

#include "base.h"
#include <sys/socket.h>

/* HTTP protocol configuration constants */
#define MAX_HEADER_SIZE 8192      /* Maximum HTTP header size in bytes */
#define MAX_REQ_SIZE 53248        /* Maximum total request size */
#define RECV_CHUNK_SIZE 1024      /* Single socket read chunk size */
#define STATIC_RES_HEADER_SIZE 512 /* Response header buffer size */

/* Parsed HTTP request structure containing method, path, and body data */
typedef struct
{
    char method[16];              /* HTTP method: GET, POST etc. */
    char path[256];               /* Request URL/file path */
    char version[16];             /* HTTP version: HTTP/1.1 */
    unsigned long long content_length; /* Size of request body */
    char *header;                 /* Complete header section */
    char *body;                   /* Request body data for POST requests */
} HttpRequest;

/* HTTP response structure with status, headers, and content metadata */
typedef struct
{
    char statusLine[32];          /* HTTP status line (e.g., HTTP/1.1 200 OK) */
    char headers[300];            /* Formatted response headers */
    const char *MIMEtype;         /* Content MIME type */
    long fileSize;                /* Size of response body */
} HttpResponse;

extern char *httpNotFoundResponse;
extern char *httpServerErrorResponse;
extern char *httpBadRequestResponse;
extern char *httpTimeoutResponse;
extern char *httpNotAllowedResponse;

void initHttpNotFoundHeaders();
void initHttpServerErrorHeaders();
void initHttpBadRequestHeaders();
void initHttpTimeoutHeaders();
void initHttpNotAllowedHeaders();

int initHttpNotFoundResponse();
int initServerErrorResponse();
int initHttpBadRequestResponse();
int initHttpTimeoutResponse();
int initHttpNotAllowedResponse();

int parse_http_request_header(char *buffer, HttpRequest *httpRequest);
int parse_http_request_body(HttpRequest *httpRequest, char *startBody, size_t body_size);

int prepHttpResponseHeader(HttpResponse *httpResponse);
int prepApiErrorHeader(HttpResponse *httpResponse);

int sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse);

int send_HttpError_Response(int client_fd, int statusCode);
int sendFileContent(int client_fd, FILE *file);
int sendApiContent(int client_fd, char *json_string);
#endif