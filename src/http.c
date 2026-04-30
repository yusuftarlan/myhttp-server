
#include "http.h"

int parse_http_request(const char *buffer, HttpRequest *request)
{
    if (buffer == NULL || request == NULL)
    {
        return -1;
    }

    memset(request, 0, sizeof(HttpRequest));

    int result = sscanf(
        buffer,
        "%15s %255s %15s",
        request->method,
        request->path,
        request->version);

    if (result != 3)
    {
        return -1;
    }

    return 0;
}

int sa()
{
    printf("sa"); //sdasdadsa
}