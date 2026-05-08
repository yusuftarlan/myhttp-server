
#ifndef HTTP_H
#define HTTP_H
#define _GNU_SOURCE
#include "base.h" //dasda

#include <sys/socket.h>

#define MAX_HEADER_SIZE 8192 // 8 KB
#define MAX_REQ_SIZE 53248
#define RECV_CHUNK_SIZE 1024 // Her recv'de en fazla 1 KB oku
#define STATIC_RES_HEADER_SIZE 512
typedef struct
{
    char method[16];  // "GET", "POST", "DELETE" vs. için fazlasıyla yeterli
    char path[256];   // İstek yapılan URL/dosya yolu
    char version[16]; // "HTTP/1.1"
    unsigned long long content_length;
    char *header;
    char *body;
} HttpRequest;

typedef struct
{
    char statusLine[32];
    char headers[300];
    const char *MIMEtype;
    long fileSize;

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