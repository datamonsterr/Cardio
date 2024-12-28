#pragma once
#include "main.h"
#define MAXEVENTS 100

void *get_in_addr(struct sockaddr *sa);
int get_listener_socket(const char *ipaddr, const char *port, int backlog);
int set_nonblocking(int sockfd);
int accept_connection(int listenfd);
void close_connection(int fd);
void server_perror(const char *msg);