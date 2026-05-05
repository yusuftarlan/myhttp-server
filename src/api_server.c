#include "api_server.h"

int _api_hello(int client_fd, HttpRequest *httpRequest)
{
    printf("postbody:%s\n", httpRequest->body);
    cJSON *root = cJSON_Parse(httpRequest->body);

    if (root == NULL)
    {
        printf("JSON hatasi: %s\n", cJSON_GetErrorPtr());
    }
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");

    if (cJSON_IsString(name) && (name->valuestring != NULL))
    {
        printf("Kullanici: %s\n", name->valuestring);
    }
}

int handleApi(int client_fd, HttpRequest *httpRequest)
{
    if (strcmp(httpRequest->path, "/api/hello") == 0)
    {
        _api_hello(client_fd, httpRequest);
    }
}
