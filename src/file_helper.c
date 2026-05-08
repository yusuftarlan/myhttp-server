#include "file_helper.h"

const char *setMIMEtype(const char *filePath)
{
    // Dosya yolu NULL gelirse diye güvenlik kontrolü
    if (filePath == NULL)
    {
        return "application/octet-stream";
    }

    const char *ext = strrchr(filePath, '.');

    // EĞER NOKTA YOKSA (ext == NULL) KONTROLÜ - KRİTİK!
    if (ext == NULL)
    {
        // Uzantı yoksa varsayılan binary tipini dön
        return "application/octet-stream";
    }

    // strcasecmp kullanarak büyük/küçük harf duyarsız kontrol yap (Örn: .PNG ve .png aynıdır)
    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0)
        return "text/html; charset=utf-8";

    if (strcasecmp(ext, ".css") == 0)
        return "text/css";

    if (strcasecmp(ext, ".js") == 0)
        return "application/javascript";

    if (strcasecmp(ext, ".png") == 0)
        return "image/png";

    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
        return "image/jpeg";

    if (strcmp(ext, ".mp4") == 0)
        return "video/mp4";

    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
        
    if (strcasecmp(ext, ".json") == 0)
        return "application/json";

    // Bilinmeyen bir uzantıysa varsayılan tipi dön
    return "application/octet-stream";
}
int prepfileSize(FILE *file, HttpResponse *httpResponse)
{

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    if (fileSize >= 0)
    {
        httpResponse->fileSize = fileSize;
        return 0;
    }
    return -1;
}
