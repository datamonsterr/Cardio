#include "main.h"

struct addrinfo *server_addr_init(const char *ipaddr, const char *port)
{
    struct addrinfo hints, *res;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_family = AF_UNSPEC;     // Support both IPv4 IPv6
    int status;
    if ((status = getaddrinfo(ipaddr, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "server_addr_init getaddrinfo error: %d", status);
        exit(1);
    }
    return res;
}

void server_perror(const char *msg)
{
    fprintf(stderr, "server.c error: %s\n", msg);
    exit(1);
}

int create_listen_socket(const char *ipaddr, const char *port, int backlog)
{
    struct addrinfo *res = server_addr_init(ipaddr, port);

    int listenfd = socket(res->ai_family, res->ai_socktype, 0);
    if (listenfd < 0)
    {
        server_perror("socket");
    }

    if (bind(listenfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        server_perror("bind");
    }

    if (listen(listenfd, backlog) < 0)
    {
        server_perror("listen");
    }
    freeaddrinfo(res);

    return listenfd;
}

int accept_connection(int listenfd)
{
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_fd = accept(listenfd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd < 0)
    {
        server_perror("accept");
    }
    return client_fd;
}

void close_connection(int fd)
{
    if (close(fd) < 0)
    {
        server_perror("close");
    }
}

int recv_login_request(int fd, char *msg, int buffer_size, Map *data)
{
    int bytes_received = recv(fd, msg, buffer_size, 0);
    if (bytes_received < 0)
    {
        server_perror("recv");
        return -1;
    }

    char content[MAXLINE];

    memcpy(content, msg + 3, bytes_received - 3);

    data = decode_msgpack(msg, bytes_received);

    if (data == NULL)
    {
        return -1;
    }

    return 1;
}

int send_login_response(int fd, const char *response, int len, Map data) // Return the number of bytes sent
{
    return 0;
}