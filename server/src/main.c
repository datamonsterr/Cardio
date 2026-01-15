#include "main.h"
#include <stdlib.h>
#include <time.h>

int main(void)
{
    // Seed random number generator for deck shuffling
    srand((unsigned int)time(NULL));
    
    int listener = get_listener_socket("0.0.0.0", "8080", 100);

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

    if (set_nonblocking(listener) == -1)
    {
        logger(MAIN_LOG, "Error", "Cannot set nonblocking");
        return 1;
    }

    struct epoll_event* events = calloc(MAXEVENTS, sizeof(event));

    if (events == NULL)
    {
        perror("calloc");
        return 1;
    }

    TableList* table_list = init_table_list(1000);

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

                if (set_nonblocking(client_fd) == -1)
                {
                    logger(MAIN_LOG, "Error", "Cannot set nonblocking");
                    return 1;
                }

                if (add_connection_to_epoll(epoll_fd, client_fd) == -1)
                {
                    logger(MAIN_LOG, "Error", "Cannot add connection to epoll");
                    return 1;
                }
            }
            else
            {
                conn_data_t* conn_data = events[i].data.ptr;
                if (!conn_data || conn_data->fd <= 0)
                {
                    logger(MAIN_LOG, "Error", "Invalid connection data");
                    continue;
                }

                char buf[MAXLINE];
                memset(buf, 0, MAXLINE);

                int nbytes = recv(conn_data->fd, buf, MAXLINE, 0);

                if (nbytes < 0)
                {
                    if (nbytes < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // No data available; try again later
                            continue;
                        }
                        logger(MAIN_LOG, "Error", "Cannot receive data");
                        close_connection(epoll_fd, conn_data);
                        continue;
                    }
                    logger(MAIN_LOG, "Error", "Cannot receive data");
                    close_connection(epoll_fd, conn_data);
                    continue;
                }
                else if (nbytes == 0)
                {
                    logger(MAIN_LOG, "Info", "Client disconnected");
                    if (conn_data->table_id > 0)
                    {
                        leave_table(conn_data, table_list);
                    }
                    close_connection(epoll_fd, conn_data);
                    continue;
                }

                // Handle handshake first (4 bytes: length=2, protocol_version=2)
                if (nbytes == 4 && conn_data->user_id == 0)
                {
                    uint16_t handshake_len = ntohs(*(uint16_t*)&buf[0]);
                    uint16_t protocol_ver = ntohs(*(uint16_t*)&buf[2]);
                    
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Handshake from fd=%d: len=%d, ver=%d", 
                             conn_data->fd, handshake_len, protocol_ver);
                    logger(MAIN_LOG, "Info", log_msg);
                    
                    // Handshake response: [length:2, code:1]
                    char response[3];
                    uint16_t resp_len = htons(1);  // 1 byte follows
                    memcpy(response, &resp_len, 2);
                    
                    if (protocol_ver == 0x0001 && handshake_len == 2)
                    {
                        response[2] = 0x00;  // HANDSHAKE_OK
                        logger(MAIN_LOG, "Info", "Handshake OK");
                    }
                    else
                    {
                        response[2] = 0x01;  // PROTOCOL_NOT_SUPPORTED
                        logger(MAIN_LOG, "Warn", "Handshake failed - unsupported protocol");
                    }
                    
                    send(conn_data->fd, response, 3, 0);
                    continue;
                }
                
                Header* header = decode_header(buf);

                if (header == NULL)
                {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Cannot decode header, received %d bytes", nbytes);
                    logger(MAIN_LOG, "Error", log_msg);
                    printf("Unknown request, received %d bytes\n", nbytes);
                    close_connection(epoll_fd, conn_data);
                    continue;
                }

                switch (header->packet_type)
                {
                case PACKET_PING:
                    // Respond to PING with PONG
                    {
                        char log_msg[128];
                        snprintf(log_msg, sizeof(log_msg), "PING received from fd=%d, sending PONG", conn_data->fd);
                        logger(MAIN_LOG, "Debug", log_msg);
                        
                        RawBytes* pong_packet = encode_packet(PROTOCOL_V1, PACKET_PONG, NULL, 0);
                        send(conn_data->fd, pong_packet->data, pong_packet->len, 0);
                        free(pong_packet->data);
                        free(pong_packet);
                    }
                    break;

                case PACKET_LOGIN:
                    logger(MAIN_LOG, "Info", "Login request from client");
                    handle_login_request(conn_data, buf, nbytes);
                    break;

                case PACKET_SIGNUP:
                    logger(MAIN_LOG, "Info", "Signup request from client");
                    handle_signup_request(conn_data, buf, nbytes);
                    break;

                case PACKET_CREATE_TABLE:
                    logger(MAIN_LOG, "Info", "Create table request from client");
                    handle_create_table_request(conn_data, buf, nbytes, table_list);
                    break;

                case PACKET_TABLES:
                    logger(MAIN_LOG, "Info", "Get all tables request from client");
                    handle_get_all_tables_request(conn_data, buf, nbytes, table_list);
                    break;

                case PACKET_JOIN_TABLE:
                    logger(MAIN_LOG, "Info", "Join table request from client");
                    handle_join_table_request(conn_data, buf, nbytes, table_list);
                    break;

                case PACKET_SCOREBOARD:
                    logger(MAIN_LOG, "Info", "Get scoreboard request from client");
                    handle_get_scoreboard(conn_data, buf, nbytes);
                    break;

                case PACKET_FRIENDLIST:
                    logger(MAIN_LOG, "Info", "Get friendlist request from client");
                    handle_get_friendlist(conn_data, buf, nbytes);
                    break;
                
                case PACKET_ADD_FRIEND:
                    logger(MAIN_LOG, "Info", "Add friend request from client");
                    handle_add_friend_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_INVITE_FRIEND:
                    logger(MAIN_LOG, "Info", "Invite friend request from client");
                    handle_invite_friend_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_ACCEPT_INVITE:
                    logger(MAIN_LOG, "Info", "Accept invite request from client");
                    handle_accept_invite_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_REJECT_INVITE:
                    logger(MAIN_LOG, "Info", "Reject invite request from client");
                    handle_reject_invite_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_GET_INVITES:
                    logger(MAIN_LOG, "Info", "Get invites request from client");
                    handle_get_invites_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_GET_FRIEND_LIST:
                    logger(MAIN_LOG, "Info", "Get friend list request from client");
                    handle_get_friend_list_request(conn_data, buf, nbytes);
                    break;
                
                case PACKET_LEAVE_TABLE:
                    logger(MAIN_LOG, "Info", "Leave table request from client");
                    handle_leave_table_request(conn_data, buf, nbytes, table_list);
                    break;
                
                case PACKET_ACTION_REQUEST:
                    logger(MAIN_LOG, "Info", "Action request from client");
                    handle_action_request(conn_data, buf, nbytes, table_list);
                    break;

                default:
                    handle_unknown_request(conn_data, buf, nbytes);
                    fprintf(stderr, "Header: %d\n", header->packet_type);
                    break;
                }

                free(header);
                memset(buf, 0, MAXLINE);
            }
        }
    }
}