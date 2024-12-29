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
            if (events[i].data.fd == listener)
            {
                int client_fd = accept_connection(listener);

                if (client_fd == -1)
                {
                    continue;
                }

                set_nonblocking(client_fd);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
                {
                    perror("epoll_ctl");
                    return 1;
                }
            }
            else
            {
                char buf[MAXLINE];
                memset(buf, 0, MAXLINE);

                int nbytes = recv(events[i].data.fd, buf, MAXLINE, 0);

                if (nbytes <= 0)
                {
                    close(events[i].data.fd);
                    continue;
                }

                Header *header = decode_header(buf);

                if (header == NULL)
                {
                    close(events[i].data.fd);
                    continue;
                }

                switch (header->packet_type)
                {
                case 100: // Login
                    handle_login_request(events[i].data.fd, buf, nbytes);
                    break;

                default:
                    break;
                }

                free(header);
            }
        }
    }
}