#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "base.h"
#include "file_helper.h"

/* File transmission buffer size for chunked delivery */
#define BUFFERSIZE 4096

/* Static file serving function */
int sendStaticFile(int client_fd, char *path);

#endif