
#ifndef HTTP_H
#define HTTP_H
#include "base.h" //dasda
#include <sys/socket.h>
typedef struct
{
    char method[16];
    char path[256];
    char version[16];
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
int initHttpNotFoundResponse();
void sendHttpResponseHeader(int client_fd, HttpResponse *httpResponse);
int prepHttpResponseHeader(HttpResponse *httpResponse);
int parse_http_request(const char *buffer, HttpRequest *request);
int send404HttpResponse(int client_fd);
int sendFileContent(int client_fd, FILE *file);

#endif