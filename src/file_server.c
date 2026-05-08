
#include "file_server.h"

// Kötü niyetli path kontrolü
int _safe_path(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return 0;

    if (strchr(path, '%'))
        return 0;

    if (strchr(path, '\\'))
        return 0;
    if (strstr(path, ".."))
        return 0;

    return 1;
}

// Statik file'ı bulur ve göndermeye çalışır.
int sendStaticFile(int client_fd, char *path)
{
    char *fileRoot = "./www";
    LOG_INFO("Requested file: %s\n", path);

    if (!_safe_path(path))
    {
        LOG_ERROR("Path is not safe %s\n", path);
        send_HttpError_Response(client_fd, 404); // not found
        return -1;
    }

    size_t filePathSize = strlen(fileRoot) + strlen(path) + 1;
    char *filePath = (char *)calloc(filePathSize, 1);

    if (filePath == NULL)
    {
        LOG_ERROR("filePath calloc %s\n", path);
        send_HttpError_Response(client_fd, 500);
        return -1;
    }

    int control = snprintf(filePath, filePathSize, "%s%s", fileRoot, path);

    if (control < 0 || (size_t)control >= filePathSize)
    {
        LOG_ERROR("check filePath snprintf \n");
        send_HttpError_Response(client_fd, 500);
        free(filePath);
        return -1;
    }

    FILE *file = fopen(filePath, "rb");

    if (file == NULL)
    {
        LOG_ERROR("File cannot be found: %s\n", filePath);
        send_HttpError_Response(client_fd, 404);
        free(filePath);
        return 0;
    }

    HttpResponse httpResponse;
    httpResponse.MIMEtype = setMIMEtype(filePath);

    int status = 1;
    if (prepfileSize(file, &httpResponse) != 0)
    {
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    if (prepHttpResponseHeader(&httpResponse) != 0)
    {
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    if (sendHttpResponseHeader(client_fd, &httpResponse) != 0)
    {
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    if (sendFileContent(client_fd, file) != 0)
    {
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    status = 0;

cleanup:
    free(filePath);
    fclose(file);

    return status;
}
