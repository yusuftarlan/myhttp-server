#ifndef API_SERVER_H
#define API_SERVER_H
#include "base.h"
#include "http.h"
#include "cJSON.h"
int handleApi(int client_fd, HttpRequest *httpRequest);
#endif