#pragma once
#include "main.h"
#define MAXEVENTS 100

void *get_in_addr(struct sockaddr *sa);
int get_listener_socket(const char *ipaddr, const char *port, int backlog);
int set_nonblocking(int sockfd);
int accept_connection(int listenfd);
void close_connection(int fd);
void server_perror(const char *msg);
int sendall(int socketfd, char *buf, int *len); // Send all data in buffer

// Hadnler
int handle_login_request(int client_fd, char *data, size_t data_len);