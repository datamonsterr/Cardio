#include "main.h"

int main(void)
{
    int listener = get_listener_socket("127.0.0.1", "8080", 100);

    if (listener == -1)
    {
        return 1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listener;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event) == -1)
    {
        perror("epoll_ctl");
        return 1;
    }

    struct epoll_event *events = calloc(MAXEVENTS, sizeof(event));

    if (events == NULL)
    {
        perror("calloc");
        return 1;
    }

    for (;;)
    {
        int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);

        for (int i = 0; i < n; i++)
        {
            int sockfd = events[i].data.fd;
            conn_data_t *conn_data = events[i].data.ptr;
            if (sockfd == listener)
            {
                int client_fd = accept_connection(listener);

                if (client_fd == -1)
                {
                    continue;
                }

                set_nonblocking(client_fd);

                if (add_connection_to_epoll(epoll_fd, client_fd) == -1)
                {
                    fprintf(stderr, "main: Cannot add client to epoll");
                    return 1;
                };
            }
            else
            {
                char buf[MAXLINE];
                memset(buf, 0, MAXLINE);

                int nbytes = recv(sockfd, buf, MAXLINE, 0);

                if (nbytes <= 0)
                {
                    close_connection(epoll_fd, sockfd);
                    continue;
                }

                Header *header = decode_header(buf);

                if (header == NULL)
                {
                    close_connection(epoll_fd, sockfd);
                    continue;
                }

                switch (header->packet_type)
                {
                case 100: // Login
                    printf("Login request from client %d\n", sockfd);
                    handle_login_request(conn_data, sockfd, buf, nbytes);
                    break;

                case 200: // Signup
                    printf("Signup request from client %d\n", sockfd);
                    // handle_signup_request(events[i].data.fd, buf, nbytes);
                    break;

                default:
                    break;
                }

                free(header);
            }
        }
    }
}