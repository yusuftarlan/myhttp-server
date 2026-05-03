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

    // İsteğe bağlı debug çıktısı
    // printf("ext deger: %s\n", ext);

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

    if (strcasecmp(ext, ".json") == 0)
        return "application/json";

    // Bilinmeyen bir uzantıysa varsayılan tipi dön
    return "application/octet-stream";
}
int prepfileSize(FILE *file, HttpResponse *httpResponse)
{
    printf("prepfonksiyonuna girildi");

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    httpResponse->fileSize = fileSize;
    return 0;
}
