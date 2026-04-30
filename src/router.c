#include "router.h"

int route_request(int client_fd, HttpRequest *httpRequest)
{
    if (strcmp(httpRequest->method, "GET") == 0)
    {
        sendStaticFile(client_fd, httpRequest->path);
    }
    else if (strcmp("POST", httpRequest->method) == 0)
    {
    }
}