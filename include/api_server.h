#ifndef API_SERVER_H
#define API_SERVER_H
#include "base.h"
#include "http.h"
#include "cJSON.h"
int handleApi(int client_fd, HttpRequest *httpRequest);
void send_api_success(int client_fd, cJSON *data);
void send_api_error(int client_fd, const char *status_str, const char *error_message);

#endif