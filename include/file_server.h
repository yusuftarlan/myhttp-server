#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "base.h"

int sendStaticFile(int client_fd, char *path);

#endif