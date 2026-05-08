#ifndef ROUTER_H
#define ROUTER_H

#include "base.h"
#include "http.h"
#include "api_server.h"
#include "file_server.h"

/* HTTP request dispatcher - routes GET to static files, POST to API */
int route_request(int client_fd, HttpRequest *httpRequest);

#endif