
#include "file_server.h"

int sendStaticFile(int client_fd, char *path)
{
    char *fileRoot = "./www";
    size_t filePathSize = strlen(fileRoot) + strlen(path) + 1;
    char *filePath = (char *)malloc(filePathSize);

    if (filePath == NULL)
    {
        printf("filePath olusturulamadi\n");
    }

    int control = snprintf(filePath, filePathSize, "%s%s", fileRoot, path);
    printf("filePATH:%s\n", filePath);

    if (control < 0 || control >= filePathSize)
    {
        printf("Birlestirme basarisiz veya kesildi\n");
    }

    printf("%s\n", filePath);

    if (access(filePath, F_OK) == 0)
    {
        printf("Dosya var\n");
    }
    else
    {
        printf("Dosya yok\n");
    }
}