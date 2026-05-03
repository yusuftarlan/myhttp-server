#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "base.h"

#include "file_helper.h"
#define BUFFERSIZE 4096

int sendStaticFile(int client_fd, char *path);

#endif