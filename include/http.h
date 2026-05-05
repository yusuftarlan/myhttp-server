
#ifndef HTTP_H
#define HTTP_H
#define _GNU_SOURCE
#include "base.h" //dasda

#include <sys/socket.h>

#define MAX_HEADER_SIZE 8192 // 8 KB
#define MAX_BODY_SIZE 65536  // 64 KB
#define MAX_REQ_SIZE 53248
#define RECV_CHUNK_SIZE 1024 // Her recv'de en fazla 1 KB oku

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
    const char *MIMEtype;
    char headers[300];
    long fileSize;
} HttpResponse;

extern const char *httpOKStatus;
extern char *httpNotFoundResponse;
extern const char *httpServerErrorStatus;

void initHttpNotFoundHeaders();
void initHttpServerErrorHeaders();
void initHttpBadRequestHeaders();
int initHttpNotFoundResponse();
int initHttpBadRequestResponse();
void sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse);
int prepHttpResponseHeader(HttpResponse *httpResponse);
int parse_http_request_header(char *buffer, HttpRequest *httpRequest);
int parse_http_request_body(char *buffer, HttpRequest *httpRequest, char *endOfHeader, size_t body_size);

int send_404Http_Response(int client_fd);
int send_BadRequest_Response(int client_fd);
int sendFileContent(int client_fd, FILE *file);

#endif