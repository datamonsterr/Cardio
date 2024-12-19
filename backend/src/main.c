#include <stdio.h>
#include <string.h>
#include "mpack.h"
#include "generic_map.h"
#include "server.h"

int main(void)
{
    int listenfd = create_listen_socket("127.0.0.1", "5500", 10);

    while (1)
    {
        int client_fd = accept_connection(listenfd);
        char buffer[MAXLINE];
        Map *m = map_create(string_compare, free, free);
        int n = recv_login_request(client_fd, buffer, MAXLINE, m);
        if (n < 0)
        {
            perror("recv_login_request");
            close(client_fd);
            continue;
        }

        MapEntry *entry = map_search(m, "user");
        if (entry == NULL)
        {
            fprintf(stderr, "user not found\n");
            close(client_fd);
            continue;
        }

        char *user = (char *)entry->value;
        printf("user: %s\n", user);

        entry = map_search(m, "pass");
        if (entry == NULL)
        {
            fprintf(stderr, "pass not found\n");
            close(client_fd);
            continue;
        }

        char *pass = (char *)entry->value;
        close(client_fd);
    }
}