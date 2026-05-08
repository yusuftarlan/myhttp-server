#include "file_helper.h"

/**
 * setMIMEtype - Determine MIME type based on file extension
 * @filePath: Full file path to analyze
 *
 * Description: Examines file extension and returns appropriate MIME type string.
 *              Uses case-insensitive comparison for common extensions.
 *              Returns "application/octet-stream" for unknown types as safe fallback.
 *
 * Return: MIME type string (static, do not free)
 */
const char *setMIMEtype(const char *filePath)
{
    /* Validate input - NULL path safety check */
    if (filePath == NULL)
    {
        return "application/octet-stream";
    }

    /* Extract file extension from path */
    const char *ext = strrchr(filePath, '.');

    /* Check if extension exists - CRITICAL for preventing NULL dereference */
    if (ext == NULL)
    {
        /* No extension found - use default binary type */
        return "application/octet-stream";
    }

    /* HTML files - text/html with UTF-8 charset */
    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        return "text/html; charset=utf-8";

    /* CSS stylesheets */
    if (strcasecmp(ext, ".css") == 0)
        return "text/css";

    /* JavaScript code files */
    if (strcasecmp(ext, ".js") == 0)
        return "application/javascript";

    /* PNG images (case-insensitive) */
    if (strcasecmp(ext, ".png") == 0)
        return "image/png";

    /* JPEG images (case-insensitive) */
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
        return "image/jpeg";

    /* MP4 video files */
    if (strcmp(ext, ".mp4") == 0)
        return "video/mp4";

    /* MP3 audio files */
    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";

    /* JSON data files */
    if (strcasecmp(ext, ".json") == 0)
        return "application/json";

    /* Unknown extension - return safe default binary type */
    return "application/octet-stream";
}

/**
 * prepfileSize - Determine file size and populate response structure
 * @file: Open file pointer to measure
 * @httpResponse: Pointer to HttpResponse structure to populate
 *
 * Description: Seeks to end of file, retrieves size in bytes, and rewinds pointer.
 *              Stores file size in httpResponse for Content-Length header.
 *
 * Return: 0 on success, -1 if file size cannot be determined
 */
int prepfileSize(FILE *file, HttpResponse *httpResponse)
{
    /* Seek to end of file to get size */
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);

    /* Rewind to beginning for reading content */
    rewind(file);

    /* Verify file size is valid */
    if (fileSize >= 0)
    {
        httpResponse->fileSize = fileSize;
        return 0;
    }

    return -1;
}
