#include "main.h"

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(const char *host, const char *port, int backlog)
{
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(host, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "get_listener_socket: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL)
    {
        return -1;
    }

    // Listen
    if (listen(listener, backlog) == -1)
    {
        return -1;
    }

    return listener;
}

int set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

int accept_connection(int listenfd)
{
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_fd = accept(listenfd, (struct sockaddr *)&client_addr, &addr_size);

    // log the new connection address using get_in_addr
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof client_ip);
    fprintf(stdout, "New connection from %s\n", client_ip);

    if (client_fd < 0)
    {
        fprintf(stderr, "accept_connection: Cannot accept connection\n");
        return -1;
    }
    return client_fd;
}

int sendall(int socketfd, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while (total < *len)
    {
        n = send(socketfd, buf + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

conn_data_t *init_connection_data(int client_fd)
{
    conn_data_t *conn_data = malloc(sizeof(conn_data_t));
    if (!conn_data)
    {
        fprintf(stderr, "init_connection_data: Cannot allocate memory for connection data\n");
        close(client_fd);
        return NULL;
    }

    conn_data->fd = client_fd;
    memset(conn_data->username, 0, 32);
    conn_data->user_id = 0;
    conn_data->balance = 0;
    conn_data->table_id = 0;
    conn_data->buffer_len = 0;
    conn_data->is_active = false;

    return conn_data;
}

int add_connection_to_epoll(int epoll_fd, int client_fd)
{
    conn_data_t *conn_data = init_connection_data(client_fd);

    if (!conn_data)
    {
        fprintf(stderr, "add_connection_to_epoll: Cannot initialize connection data\n");
        return -1;
    }

    // Set up epoll_event
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered input events
    event.data.ptr = conn_data;       // Associate custom data

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
    {
        fprintf(stderr, "add_connection_to_epoll: Cannot add client to epoll\n");
        free(conn_data);
        close(client_fd);
        return -1;
    }

    fprintf(stdout, "Added client %d to epoll\n", client_fd);

    return 0;
}

int update_conn_data(int epoll_fd, int client_fd, conn_data_t *conn_data)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = conn_data;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1)
    {
        fprintf(stderr, "update_conn_data: Cannot update client data\n");
        return -1;
    }

    return 0;
}

int close_connection(int epoll_fd, conn_data_t *conn_data)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_data->fd, NULL) == -1)
    {
        fprintf(stderr, "close_connection: Cannot remove client from epoll\n");
        return -1;
    }

    fprintf(stdout, "Closed connection from client %d\n", conn_data->fd);

    free(conn_data);
    close(conn_data->fd);
    return 0;
}