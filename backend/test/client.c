#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

// int main(int argc, char *argv[])
// {
//     if (argc != 3)
//     {
//         fprintf(stderr, "usage: %s [host] [port]\n", argv[0]);
//         exit(1);
//     }

//     int servfd;
//     struct addrinfo hints, *res;

//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM;

//     if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0)
//     {
//         perror("getaddrinfo");
//         exit(1);
//     }

//     servfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//     if (servfd == -1)
//     {
//         perror("socket");
//         exit(1);
//     }

//     if (connect(servfd, res->ai_addr, res->ai_addrlen) == -1)
//     {
//         perror("connect");
//         exit(1);
//     }

//     char input[MAXLINE];
//     char recv_buffer[MAXLINE];

//     while (1)
//     {
//         printf("Enter a message: ");
//         if (fgets(input, MAXLINE, stdin) == NULL)
//         {
//             printf("Error reading input. Exiting...\n");
//             break;
//         }

//         size_t len = strlen(input);
//         if (len > 0 && input[len - 1] == '\n')
//         {
//             input[len - 1] = '\0';
//         }

//         if (send(servfd, input, strlen(input), 0) == -1)
//         {
//             perror("send");
//             break;
//         }

//         memset(recv_buffer, 0, sizeof(recv_buffer));

//         ssize_t recv_len = recv(servfd, recv_buffer, sizeof(recv_buffer) - 1, 0);
//         if (recv_len == -1)
//         {
//             perror("recv");
//             break;
//         }
//         recv_buffer[recv_len] = '\0';

//         printf("Received: %s\n", recv_buffer);
//     }

//     freeaddrinfo(res);
//     close(servfd);

//     return 0;
// }