#ifndef SERVER_H
#define SERVER_H

#include "base.h"
#include "http.h"
#include "router.h"

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#define BACKLOG 36
#define THREAD_COUNT 8
#define QUEUE_SIZE 32

extern int queue[QUEUE_SIZE];
extern int front;
extern int rear;
extern int count;
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_cond;

int server_start(int port);

#endif