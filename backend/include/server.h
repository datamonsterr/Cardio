#pragma once
#include "main.h"
#define MAXEVENTS 100

void *get_in_addr(struct sockaddr *sa);
int get_listener_socket(const char *ipaddr, const char *port, int backlog);
int set_nonblocking(int sockfd);
int accept_connection(int listenfd);
int sendall(int socketfd, char *buf, int *len); // Send all data in buffer
                                                //
// Connection management
// Custom data structure to associate with each connection
typedef struct
{
    char buffer[1024];       // Buffer for partial reads/writes
    int fd;                  // File descriptor for the connection
    char username[32];       // Player's username
    unsigned int user_id;    // Player's ID
    float balance;           // Chips the player has
    unsigned short table_id; // Game table ID (0 if not at a table)
    size_t buffer_len;       // Length of valid data in the buffer
    bool is_active;          // Player's activity status
} conn_data_t;
// Initialize connection data with default values
conn_data_t *init_connection_data(int client_fd);
// Add connection to epoll
int add_connection_to_epoll(int epoll_fd, int client_fd);
// Close connection
int close_connection(int epoll_fd, conn_data_t *conn_data);
// Update connection data
int update_conn_data(int epoll_fd, int client_fd, conn_data_t *conn_data);

// Hadnler
int handle_login_request(conn_data_t *conn_data, int client_fd, char *data, size_t data_len);