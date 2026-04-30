#ifndef ROUTER_H
#define ROUTER_H
#include "base.h"
#include "http.h"
#include "file_server.h"

int route_request(int client_fd, HttpRequest *httpRequest);

#endif