#include "test.h"

int create_connection_socket(const char *ipaddr, const char *port)
{
    struct addrinfo hints, *res;
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_family = AF_UNSPEC;     // Support both IPv4 IPv6
    int status;
    if ((status = getaddrinfo(ipaddr, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "create_connection_socket getaddrinfo error: %d", status);
        exit(1);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("connect");
        exit(1);
    }

    freeaddrinfo(res);
    return sockfd;
}

uint16_t create_message(char *buffer)
{
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, "admin");
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "admin");
    mpack_finish_map(&writer);
    uint16_t len = mpack_writer_buffer_used(&writer);
    mpack_writer_destroy(&writer);
    memcpy(buffer, writer.buffer, len);
    return len;
}
