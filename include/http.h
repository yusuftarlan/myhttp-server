
#ifndef HTTP_H
#define HTTP_H
#include "base.h" //dasda
typedef struct
{
    char method[16];
    char path[256];
    char version[16];
} HttpRequest;

typedef struct
{
    char response[16];
    char path[256];
    char version[16];
} HttpResponse;

int parse_http_request(const char *buffer, HttpRequest *request);

#endif