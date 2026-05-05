#include "router.h"

void route_request(int client_fd, HttpRequest *httpRequest)
{
    printf("route_request hosgeldin\n");
    printf("metot:%s\nPATH:%s\nbody:%s\n", httpRequest->method, httpRequest->path, httpRequest->body);

    if (strcmp(httpRequest->method, "GET") == 0)
    {
        sendStaticFile(client_fd, httpRequest->path);
    }
    else if (strcmp(httpRequest->method, "POST") == 0)
    {
        char *is_api = strstr(httpRequest->path, "/api/");
        if (is_api == httpRequest->path) // /api/ ilk bastami
        {
            handleApi(client_fd, httpRequest);
            return;
        }
        else
        {
            send_BadRequest_Response(client_fd);
            return;
        }
    }
    else
    {
        send_BadRequest_Response(client_fd);
        return;
    }
}