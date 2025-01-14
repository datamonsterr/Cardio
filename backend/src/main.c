#include "main.h"

// int main(void)
// {
//     int listener = get_listener_socket("0.0.0.0", "8080", 100);

//     if (listener == -1)
//     {
//         return 1;
//     }

//     int epoll_fd = epoll_create1(0);

//     if (epoll_fd == -1)
//     {
//         perror("epoll_create1");
//         return 1;
//     }

//     struct epoll_event event;
//     event.events = EPOLLIN;
//     event.data.fd = listener;

//     if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event) == -1)
//     {
//         perror("epoll_ctl");
//         return 1;
//     }

//     if (set_nonblocking(listener) == -1)
//     {
//         logger(MAIN_LOG, "Error", "Cannot set nonblocking");
//         return 1;
//     }

//     struct epoll_event *events = calloc(MAXEVENTS, sizeof(event));

//     if (events == NULL)
//     {
//         perror("calloc");
//         return 1;
//     }

//     TableList *table_list = init_table_list(1000);

//     for (;;)
//     {
//         int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);

//         for (int i = 0; i < n; i++)
//         {
//             if (events[i].data.fd == listener)
//             {
//                 int client_fd = accept_connection(listener);

//                 if (client_fd == -1)
//                 {
//                     continue;
//                 }

//                 if (set_nonblocking(client_fd) == -1)
//                 {
//                     logger(MAIN_LOG, "Error", "Cannot set nonblocking");
//                     return 1;
//                 }

//                 if (add_connection_to_epoll(epoll_fd, client_fd) == -1)
//                 {
//                     logger(MAIN_LOG, "Error", "Cannot add connection to epoll");
//                     return 1;
//                 }
//             }
//             else
//             {
//                 conn_data_t *conn_data = events[i].data.ptr;
//                 if (!conn_data || conn_data->fd <= 0)
//                 {
//                     logger(MAIN_LOG, "Error", "Invalid connection data");
//                     continue;
//                 }

//                 char buf[MAXLINE];
//                 memset(buf, 0, MAXLINE);

//                 int nbytes = recv(conn_data->fd, buf, MAXLINE, 0);

//                 if (nbytes < 0)
//                 {
//                     if (nbytes < 0)
//                     {
//                         if (errno == EAGAIN || errno == EWOULDBLOCK)
//                         {
//                             // No data available; try again later
//                             continue;
//                         }
//                         logger(MAIN_LOG, "Error", "Cannot receive data");
//                         close_connection(epoll_fd, conn_data);
//                         continue;
//                     }
//                     logger(MAIN_LOG, "Error", "Cannot receive data");
//                     close_connection(epoll_fd, conn_data);
//                     continue;
//                 }
//                 else if (nbytes == 0)
//                 {
//                     logger(MAIN_LOG, "Info", "Client disconnected");
//                     if (conn_data->table_id > 0)
//                     {
//                         leave_table(conn_data, table_list);
//                     }
//                     close_connection(epoll_fd, conn_data);
//                     continue;
//                 }
//                 Header *header = decode_header(buf);

//                 if (header == NULL)
//                 {
//                     logger(MAIN_LOG, "Error", "Cannot decode header");
//                     close_connection(epoll_fd, conn_data);
//                     continue;
//                 }

//                 switch (header->packet_type)
//                 {
//                 case PACKET_LOGIN:
//                     logger(MAIN_LOG, "Info", "Login request from client");
//                     handle_login_request(conn_data, buf, nbytes);
//                     break;

//                 case PACKET_SIGNUP:
//                     logger(MAIN_LOG, "Info", "Signup request from client");
//                     handle_signup_request(conn_data, buf, nbytes);
//                     break;

//                 case PACKET_CREATE_TABLE:
//                     logger(MAIN_LOG, "Info", "Create table request from client");
//                     handle_create_table_request(conn_data, buf, nbytes, table_list);
//                     break;

//                 case PACKET_TABLES:
//                     logger(MAIN_LOG, "Info", "Get all tables request from client");
//                     handle_get_all_tables_request(conn_data, buf, nbytes, table_list);
//                     break;

//                 case PACKET_JOIN_TABLE: // Join table
//                     logger(MAIN_LOG, "Info", "Join table request from client");
//                     handle_join_table_request(conn_data, buf, nbytes, table_list);
//                     break;

//                 default:
//                     fprintf(stderr, "Header: %d\n", header->packet_type);
//                     handle_unknown_request(conn_data, buf, nbytes);
//                     break;
//                 }

//                 free(header);
//                 memset(buf, 0, MAXLINE);
//             }
//         }
//     }
// }

int main(void) {

        
}