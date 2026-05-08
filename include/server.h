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

/* Server configuration constants */
#define BACKLOG 36       /* Connection queue size for listen() */
#define THREAD_COUNT 8   /* Number of worker threads */
#define QUEUE_SIZE 32    /* Client work queue capacity */

/* Thread-safe client queue state variables */
extern int queue[QUEUE_SIZE];
extern int front;                    /* Queue head pointer */
extern int rear;                     /* Queue tail pointer */
extern int count;                    /* Number of items in queue */
extern pthread_mutex_t queue_mutex;  /* Queue access lock */
extern pthread_cond_t queue_cond;    /* Queue notification signal */

/* Server initialization and main loop */
int server_start(int port);

#endif