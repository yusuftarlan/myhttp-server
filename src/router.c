#include "router.h"

// GET, POST, API ayrımı yapılan nokta
int route_request(int client_fd, HttpRequest *httpRequest)
{

    if (strcmp(httpRequest->method, "GET") == 0)
    {
        if (strcmp(httpRequest->path, "/") == 0)
        {

            return sendStaticFile(client_fd, "/index.html");
        }

        return sendStaticFile(client_fd, httpRequest->path);
    }
    else if (strcmp(httpRequest->method, "POST") == 0)
    {
        char *is_api = strstr(httpRequest->path, "/api/");

        if (is_api == NULL)
        {
            LOG_ERROR("Bad path: %s\n", httpRequest->path);
            send_HttpError_Response(client_fd, 400); // bad request

            return -1;
        }

        if (is_api == httpRequest->path) // /api/ ilk bastami
        {
            return handleApi(client_fd, httpRequest);
        }
        else
        {
            send_HttpError_Response(client_fd, 400); // bad request
            return -1;
        }
    }
    else
    {
        send_HttpError_Response(client_fd, 400); // bad request
        return -1;
    }
}