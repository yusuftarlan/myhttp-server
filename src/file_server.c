
#include "file_server.h"


int safe_path(const char *path)
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

int sendStaticFile(int client_fd, char *path)
{
    char *fileRoot = "./www";
    printf("path:%s\n", path);

    if (!safe_path(path))
    {
        send404HttpResponse(client_fd);
        return -1;
    }

    size_t filePathSize = strlen(fileRoot) + strlen(path) + 1;
    char *filePath = (char *)malloc(filePathSize);

    // 1. DÜZELTME: Malloc çökme koruması
    if (filePath == NULL)
    {
        printf("Kritik Hata: filePath icin bellek ayrilamadi\n");
        // İdeal olanı burada send500ServerError() gibi bir fonksiyon çağırmaktır
        return -1; // İşleme devam etme, çık!
    }

    int control = snprintf(filePath, filePathSize, "%s%s", fileRoot, path);
    printf("filePATH:%s\n", filePath);

    if (control < 0 || control >= filePathSize)
    {
        printf("Birlestirme basarisiz veya kesildi\n");
        free(filePath); // Çıkmadan önce belleği temizle
        return -1;
    }

    FILE *file = fopen(filePath, "rb");

    if (file == NULL)
    {
        send404HttpResponse(client_fd);
        // 2. DÜZELTME: ASIL SIZINTIYI ÇÖZEN SATIR
        free(filePath);
        return 0;
    }

    HttpResponse httpResponse;
    httpResponse.MIMEtype = setMIMEtype(filePath);
    printf("MIMETYPE: %s\n", httpResponse.MIMEtype);

    prepfileSize(file, &httpResponse);
    prepHttpResponseHeader(&httpResponse);
    sendHttpResponseHeader(client_fd, &httpResponse);
    sendFileContent(client_fd, file);

    fclose(file);
    free(filePath); // Başarılı senaryodaki normal temizlik

    // 3. DÜZELTME: Fonksiyon int döndürüyor, başarılı bitiş için bir değer dönmeli
    return 1;
}
