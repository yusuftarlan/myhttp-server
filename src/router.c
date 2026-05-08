#include "router.h"

/**
 * route_request - Route HTTP request to appropriate handler
 * @client_fd: Client socket file descriptor
 * @httpRequest: Pointer to parsed HTTP request structure
 *
 * Description: Routes incoming requests based on HTTP method (GET or POST).
 *              GET requests serve static files from www/ directory.
 *              POST requests are validated for API path format and delegated to handleApi.
 *
 * Return: 0 on success, -1 on error (invalid method or malformed request)
 */
int route_request(int client_fd, HttpRequest *httpRequest)
{
    /* Handle GET requests - serve static files */
    if (strcmp(httpRequest->method, "GET") == 0)
    {
        /* Root path requests default to index.html */
        if (strcmp(httpRequest->path, "/") == 0)
        {
            return sendStaticFile(client_fd, "/index.html");
        }

        /* Serve requested static file */
        return sendStaticFile(client_fd, httpRequest->path);
    }
    /* Handle POST requests - API endpoints only */
    else if (strcmp(httpRequest->method, "POST") == 0)
    {
        /* Check if request path contains /api/ prefix */
        char *is_api = strstr(httpRequest->path, "/api/");

        /* Invalid path - /api/ prefix required for POST requests */
        if (is_api == NULL)
        {
            LOG_ERROR("Bad path: %s\n", httpRequest->path);
            send_HttpError_Response(client_fd, 400);
            return -1;
        }

        /* Ensure /api/ is at the beginning of the path */
        if (is_api == httpRequest->path)
        {
            return handleApi(client_fd, httpRequest);
        }
        else
        {
            send_HttpError_Response(client_fd, 400);
            return -1;
        }
    }
    /* Invalid HTTP method */
    else
    {
        send_HttpError_Response(client_fd, 400);
        return -1;
    }
}