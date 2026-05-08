
#include "file_server.h"

/**
 * _safe_path - Validate file path for security threats
 * @path: File path to validate
 *
 * Description: Prevents directory traversal and other path-based attacks.
 *              Checks for null/empty paths, URL encoding, backslashes, and parent directory references.
 *
 * Return: 1 if path is safe, 0 if path contains security risks
 */
int _safe_path(const char *path)
{
    /* Check for NULL or empty path */
    if (path == NULL || path[0] == '\0')
        return 0;

    /* Reject URL-encoded characters (e.g., %2e for dot) */
    if (strchr(path, '%'))
        return 0;

    /* Reject Windows path separators */
    if (strchr(path, '\\'))
        return 0;

    /* Prevent directory traversal (../) attacks */
    if (strstr(path, ".."))
        return 0;

    return 1;
}

/**
 * sendStaticFile - Locate and send static file to client
 * @client_fd: Client socket file descriptor
 * @path: Requested file path (relative to www root)
 *
 * Description: Serves static files from www/ directory with security checks.
 *              Validates path, determines MIME type, prepares response headers,
 *              and transmits file content to client.
 *              Uses cleanup label for proper resource deallocation on all paths.
 *
 * Return: 0 on success, -1 on security violation or error, 0 on file not found (404 sent)
 */
int sendStaticFile(int client_fd, char *path)
{
    char *fileRoot = "./www";
    LOG_INFO("Requested file: %s\n", path);

    /* Security check: validate path to prevent directory traversal attacks */
    if (!_safe_path(path))
    {
        LOG_ERROR("Path is not safe %s\n", path);
        send_HttpError_Response(client_fd, 404);
        return -1;
    }

    /* Construct full file path */
    size_t filePathSize = strlen(fileRoot) + strlen(path) + 1;
    char *filePath = (char *)calloc(filePathSize, 1);

    if (filePath == NULL)
    {
        LOG_ERROR("Memory allocation failed for file path\n");
        send_HttpError_Response(client_fd, 500);
        return -1;
    }

    /* Format complete file path */
    int control = snprintf(filePath, filePathSize, "%s%s", fileRoot, path);

    if (control < 0 || (size_t)control >= filePathSize)
    {
        LOG_ERROR("Path formatting failed\n");
        send_HttpError_Response(client_fd, 500);
        free(filePath);
        return -1;
    }

    /* Attempt to open file */
    FILE *file = fopen(filePath, "rb");

    if (file == NULL)
    {
        LOG_ERROR("File not found: %s\n", filePath);
        send_HttpError_Response(client_fd, 404);
        free(filePath);
        return 0;
    }

    /* Initialize HTTP response structure */
    HttpResponse httpResponse;
    httpResponse.MIMEtype = setMIMEtype(filePath);

    int status = 1;

    /* Prepare file size information */
    if (prepfileSize(file, &httpResponse) != 0)
    {
        LOG_ERROR("Failed to determine file size\n");
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    /* Format HTTP response headers */
    if (prepHttpResponseHeader(&httpResponse) != 0)
    {
        LOG_ERROR("Failed to prepare response header\n");
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    /* Send HTTP response headers to client */
    if (sendHttpResponseHeader(client_fd, &httpResponse) != 0)
    {
        LOG_ERROR("Failed to send response header\n");
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    /* Send file content to client */
    if (sendFileContent(client_fd, file) != 0)
    {
        LOG_ERROR("Failed to send file content\n");
        send_HttpError_Response(client_fd, 500);
        goto cleanup;
    }

    /* Success - mark status before cleanup */
    status = 0;

    /* Cleanup label: releases resources regardless of success/failure path */
cleanup:
    free(filePath);
    fclose(file);

    return status;
}
