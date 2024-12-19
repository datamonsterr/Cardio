#pragma once
#include "main.h"
#include <arpa/inet.h>
#include <netdb.h>

int create_listen_socket(const char *ipaddr, const char *port, int backlog);
int accept_connection(int listenfd);
int recv_login_request(int fd, char *msg, int buffer_size, Map *data);

Map *decode_msgpack(const char *data, size_t len);