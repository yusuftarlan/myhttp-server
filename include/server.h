#ifndef SERVER_H
#define SERVER_H

#include "base.h"
#include "router.h"

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int server_start(int port);

#endif