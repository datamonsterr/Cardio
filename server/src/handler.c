#include "main.h"

void handle_login_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Login request from fd=%d, data_len=%zu", conn_data->fd, data_len);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    PGconn* conn = PQconnectdb(dbconninfo);
    Packet* packet = decode_packet(data, data_len);
    if (packet->header->packet_type != 100)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet type", 1);
    }

    if (packet->header->packet_len != data_len)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet length", 1);
    }

    LoginRequest* login_request = decode_login_request(packet->data);
    snprintf(log_msg, sizeof(log_msg), "Attempting login for user='%s'", login_request->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    int user_id = dbLogin(conn, login_request->username, login_request->password);
    snprintf(log_msg, sizeof(log_msg), "dbLogin returned user_id=%d for user='%s'", user_id, login_request->username);
    logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
    
    if (user_id > 0)
    {
        struct dbUser user_info = dbGetUserInfo(conn, user_id);
        snprintf(log_msg, sizeof(log_msg), "dbGetUserInfo returned: user_id=%d username='%s' balance=%d", 
                 user_info.user_id, user_info.username, user_info.balance);
        logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
        
        RawBytes* raw_bytes = encode_login_success_response(&user_info);
        RawBytes* response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));

        strncpy(conn_data->username, user_info.username, 32);
        conn_data->user_id = user_id;
        conn_data->is_active = true;
        conn_data->balance = user_info.balance;

        PQfinish(conn);

        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free(login_request);
        
        snprintf(log_msg, sizeof(log_msg), "Login SUCCESS: user='%s' (id=%d) fd=%d balance=%d", 
                 user_info.username, user_id, conn_data->fd, user_info.balance);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
        return;
    }

    RawBytes* raw_bytes = encode_response(R_LOGIN_NOT_OK);
    RawBytes* response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));
    
    snprintf(log_msg, sizeof(log_msg), "Login FAILED: user='%s' fd=%d (user_id=%d)", 
             login_request->username, conn_data->fd, user_id);
    logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);

    PQfinish(conn);

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    free(login_request);
}

void handle_signup_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Signup request from fd=%d", conn_data->fd);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    PGconn* conn = PQconnectdb(dbconninfo);
    Packet* packet = decode_packet(data, data_len);
    if (packet->header->packet_type != 200)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet type", 1);
    }

    if (packet->header->packet_len != data_len)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet length", 1);
    }

    SignupRequest* signup_request = decode_signup_request(packet->data);
    struct dbUser* user = malloc(sizeof(struct dbUser));
    strncpy(user->username, signup_request->username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    strncpy(user->password, signup_request->password, sizeof(user->password) - 1);
    user->password[sizeof(user->password) - 1] = '\0';
    strncpy(user->phone, signup_request->phone, sizeof(user->phone) - 1);
    user->phone[sizeof(user->phone) - 1] = '\0';
    strncpy(user->email, signup_request->email, sizeof(user->email) - 1);
    user->email[sizeof(user->email) - 1] = '\0';
    strncpy(user->fullname, signup_request->fullname, sizeof(user->fullname) - 1);
    user->fullname[sizeof(user->fullname) - 1] = '\0';
    strncpy(user->country, signup_request->country, sizeof(user->country) - 1);
    user->country[sizeof(user->country) - 1] = '\0';
    strncpy(user->gender, signup_request->gender, sizeof(user->gender) - 1);
    user->gender[sizeof(user->gender) - 1] = '\0';
    strncpy(user->dob, signup_request->dob, sizeof(user->dob) - 1);
    user->dob[sizeof(user->dob) - 1] = '\0';
    
    snprintf(log_msg, sizeof(log_msg), "Attempting signup for user='%s' email='%s' pass_len=%zu", 
             signup_request->username, signup_request->email, strlen(user->password));
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    int res = dbSignup(conn, user);

    if (res == DB_OK)
    {
        RawBytes* raw_bytes = encode_response(R_SIGNUP_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, 200, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));

        PQfinish(conn);

        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free(signup_request);
        
        snprintf(log_msg, sizeof(log_msg), "Signup SUCCESS: user='%s' fd=%d", user->username, conn_data->fd);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
        return;
    }

    RawBytes* raw_bytes = encode_response(R_SIGNUP_NOT_OK);
    RawBytes* response = encode_packet(PROTOCOL_V1, 200, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));
    
    snprintf(log_msg, sizeof(log_msg), "Signup FAILED: user='%s' fd=%d error_code=%d", 
             signup_request->username, conn_data->fd, res);
    logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);

    PQfinish(conn);

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    free(signup_request);
}

void handle_create_table_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Create table request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    Packet* packet = decode_packet(data, data_len);
    int is_valid = 1;

    if (packet->header->packet_type != 300)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet type", 1);
        is_valid = 0;
    }

    if (packet->header->packet_len != data_len)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet length", 1);
        is_valid = 0;
    }

    if (conn_data->user_id == 0)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        is_valid = 0;
    }

    if (conn_data->table_id != 0)
    {
        snprintf(log_msg, sizeof(log_msg), "User already at table (table_id=%d)", conn_data->table_id);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
        is_valid = 0;
    }

    if (is_valid == 0)
    {
        RawBytes* raw_bytes = encode_response(R_CREATE_TABLE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, 300, raw_bytes->data, raw_bytes->len);
        if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
        {
            logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
        }
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        return;
    }

    CreateTableRequest* create_table_request = decode_create_table_request(packet->data);
    snprintf(log_msg, sizeof(log_msg), "Creating table '%s' max_player=%d min_bet=%d", 
             create_table_request->table_name, create_table_request->max_player, create_table_request->min_bet);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    Table* table;
    int res = add_table(table_list, create_table_request->table_name, create_table_request->max_player,
                        create_table_request->min_bet);
    int is_join_ok = join_table(conn_data, table_list, res);
    RawBytes* raw_bytes = malloc(sizeof(RawBytes));
    RawBytes* response = malloc(sizeof(RawBytes));
    if (res > 0 && is_join_ok >= 0)
    {
        raw_bytes = encode_create_table_response(R_CREATE_TABLE_OK, res);
        response = encode_packet(PROTOCOL_V1, 300, raw_bytes->data, raw_bytes->len);
        snprintf(log_msg, sizeof(log_msg), "Table created SUCCESS: id=%d name='%s' creator='%s'", 
                 res, create_table_request->table_name, conn_data->username);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    }
    else
    {
        raw_bytes = encode_response(R_CREATE_TABLE_NOT_OK);
        response = encode_packet(PROTOCOL_V1, 300, raw_bytes->data, raw_bytes->len);
        snprintf(log_msg, sizeof(log_msg), "Table creation FAILED: res=%d is_join=%d", res, is_join_ok);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }

    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
    }

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    free(create_table_request);
}

void handle_get_all_tables_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
    Packet* packet = decode_packet(data, data_len);

    if (packet->header->packet_type != PACKET_TABLES)
    {
        logger(MAIN_LOG, "Error", "Handle get all tables: invalid packet type");
    }

    if (packet->header->packet_len != data_len)
    {
        logger(MAIN_LOG, "Error", "Handle get all tables: invalid packet length");
    }

    RawBytes* raw_bytes = encode_full_tables_response(table_list);
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_TABLES, raw_bytes->data, raw_bytes->len);
    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
    {
        logger(MAIN_LOG, "Error", "Handle get all tables: Cannot send response");
    }

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
}
void handle_join_table_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Join table request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    Packet* packet = decode_packet(data, data_len);

    int is_valid = 1;

    if (packet->header->packet_type != PACKET_JOIN_TABLE)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet type", 1);
        is_valid = 0;
    }

    if (packet->header->packet_len != data_len)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet length", 1);
        is_valid = 0;
    }

    if (conn_data->user_id == 0)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        is_valid = 0;
    }

    // Parse the requested table_id early so we can check if user is rejoining same table
    int requested_table_id = 0;
    if (is_valid) {
        requested_table_id = decode_join_table_request(packet->data);
    }

    // Check if user is already at a table
    if (conn_data->table_id != 0)
    {
        snprintf(log_msg, sizeof(log_msg), "User already at table (table_id=%d), requested table_id=%d", 
                 conn_data->table_id, requested_table_id);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
        
        // If user is trying to rejoin the same table, send them the current game state
        if (conn_data->table_id == requested_table_id) {
            snprintf(log_msg, sizeof(log_msg), "User '%s' rejoining same table %d, sending game state", 
                     conn_data->username, conn_data->table_id);
            logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
            
            // Find the table
            Table* table = NULL;
            for (size_t i = 0; i < table_list->size; i++) {
                if (table_list->tables[i].id == conn_data->table_id) {
                    table = &table_list->tables[i];
                    break;
                }
            }
            
            if (table && table->game_state) {
                // Send current game state
                RawBytes* game_state_data = encode_game_state(table->game_state, conn_data->user_id);
                if (game_state_data) {
                    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, game_state_data->data, game_state_data->len);
                    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
                    {
                        logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
                    }
                    free(response->data);
                    free(response);
                    free(game_state_data->data);
                    free(game_state_data);
                    free_packet(packet);
                    return;
                }
            }
        }
        
        // User is trying to join a different table while already at one
        logger_ex(MAIN_LOG, "ERROR", __func__, "User trying to join different table", 1);
        is_valid = 0;
    }

    if (is_valid == 0)
    {
        RawBytes* raw_bytes = encode_response(R_JOIN_TABLE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);
        if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
        {
            logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
        }
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        return;
    }

    int table_id = requested_table_id;
    snprintf(log_msg, sizeof(log_msg), "User '%s' attempting to join table_id=%d", 
             conn_data->username, table_id);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    int res = join_table(conn_data, table_list, table_id);
    printf("res = %d\n", res);
    RawBytes* raw_bytes = malloc(sizeof(RawBytes));
    RawBytes* response = malloc(sizeof(RawBytes));
    if (res >= 0)
    {
        snprintf(log_msg, sizeof(log_msg), "Join table SUCCESS: user='%s' table_id=%d seat=%d", 
                 conn_data->username, table_id, conn_data->seat);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
        
        Table* table = &table_list->tables[res];
        GameState* gs = table->game_state;
        
        snprintf(log_msg, sizeof(log_msg), "After join: table_id=%d num_players=%d hand_in_progress=%d", 
                 table->id, gs->num_players, gs->hand_in_progress);
        logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
        
        // Check if game should start (need at least 2 players, count WAITING/ACTIVE/ALL_IN)
        bool game_just_started = false;
        if (!gs->hand_in_progress && gs->num_players >= 2) {
            // Start a new hand
            snprintf(log_msg, sizeof(log_msg), "Starting hand %d at table %d after player join (num_players=%d)", 
                     gs->hand_id + 1, table->id, gs->num_players);
            logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
            
            int start_result = game_start_hand(gs);
            if (start_result == 0) {
                table->game_started = true;
                table->active_seat = gs->active_seat;
                game_just_started = true;
                
                // Debug: Log player states after game start
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (gs->players[i].state != PLAYER_STATE_EMPTY) {
                        snprintf(log_msg, sizeof(log_msg), 
                                "Player seat=%d id=%d name=%s state=%d money=%d is_dealer=%d is_sb=%d is_bb=%d", 
                                i, gs->players[i].player_id, gs->players[i].name, 
                                gs->players[i].state, gs->players[i].money,
                                gs->players[i].is_dealer, gs->players[i].is_small_blind, 
                                gs->players[i].is_big_blind);
                        logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
                    }
                }
                
                snprintf(log_msg, sizeof(log_msg), 
                        "Game started: hand_id=%d dealer_seat=%d active_seat=%d betting_round=%d", 
                        gs->hand_id, gs->dealer_seat, gs->active_seat, gs->betting_round);
                logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
            } else if (start_result == -3) {
                snprintf(log_msg, sizeof(log_msg), "Failed to start game: No big blind found");
                logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
            } else if (start_result == -4) {
                snprintf(log_msg, sizeof(log_msg), "Failed to start game: No active player after big blind");
                logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
            } else {
                snprintf(log_msg, sizeof(log_msg), "Failed to start game: error=%d", start_result);
                logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
            }
        }
        
        // Encode the current game state for the joining player
        RawBytes* game_state_data = encode_game_state(table->game_state, conn_data->user_id);
        
        snprintf(log_msg, sizeof(log_msg), 
                "encode_game_state returned: %p (size=%zu) for user_id=%d, gs->active_seat=%d", 
                (void*)game_state_data, game_state_data ? game_state_data->len : 0,
                conn_data->user_id, gs->active_seat);
        logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
        
        if (game_state_data) {
            snprintf(log_msg, sizeof(log_msg), "Game state data: len=%zu first_bytes=%02x %02x %02x %02x", 
                     game_state_data->len,
                     (unsigned char)game_state_data->data[0],
                     (unsigned char)game_state_data->data[1],
                     (unsigned char)game_state_data->data[2],
                     (unsigned char)game_state_data->data[3]);
            logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
            raw_bytes = game_state_data;
        } else {
            logger_ex(MAIN_LOG, "WARN", __func__, "encode_game_state returned NULL, sending simple OK", 1);
            raw_bytes = encode_response(R_JOIN_TABLE_OK);
        }
        
        // Send join response to the joining player first
        response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);
        printf("response len = %d\n", response->len);
        if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
        {
            logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
        }
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        
        // If game just started, broadcast to OTHER players (not the one who just joined)
        if (game_just_started) {
            snprintf(log_msg, sizeof(log_msg), "Broadcasting game start to other players at table %d", table->id);
            logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
            
            for (int i = 0; i < table->current_player; i++) {
                conn_data_t* other_conn = table->connections[i];
                
                // Skip the player who just joined (they already got the state in join response)
                if (!other_conn || other_conn->fd == conn_data->fd) {
                    continue;
                }
                
                // Encode and send game state to this player
                RawBytes* other_game_state = encode_game_state(gs, other_conn->user_id);
                if (other_game_state) {
                    RawBytes* broadcast_packet = encode_packet(PROTOCOL_V1, PACKET_UPDATE_GAMESTATE,
                                                              other_game_state->data, other_game_state->len);
                    if (broadcast_packet) {
                        int send_len = broadcast_packet->len;
                        int send_result = sendall(other_conn->fd, broadcast_packet->data, &send_len);
                        
                        if (send_result == -1) {
                            snprintf(log_msg, sizeof(log_msg), 
                                    "Failed to send game state to user='%s' fd=%d", 
                                    other_conn->username, other_conn->fd);
                            logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
                        } else {
                            snprintf(log_msg, sizeof(log_msg), 
                                    "Sent game state (%d bytes) to user='%s' fd=%d", 
                                    send_len, other_conn->username, other_conn->fd);
                            logger_ex(MAIN_LOG, "DEBUG", __func__, log_msg, 1);
                        }
                        
                        free(broadcast_packet->data);
                        free(broadcast_packet);
                    }
                    free(other_game_state->data);
                    free(other_game_state);
                }
            }
        }
        
        return;
    }
    else if (res == -2)
    {
        snprintf(log_msg, sizeof(log_msg), "Join table FAILED (FULL): user='%s' table_id=%d", 
                 conn_data->username, table_id);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
        raw_bytes = encode_response(R_JOIN_TABLE_FULL);
    }
    else
    {
        snprintf(log_msg, sizeof(log_msg), "Join table FAILED (ERROR): user='%s' table_id=%d res=%d", 
                 conn_data->username, table_id, res);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
        raw_bytes = encode_response(R_JOIN_TABLE_NOT_OK);
    }

    response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);

    printf("response len = %d\n", response->len);
    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
    }
    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    return;
}

void handle_unknown_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    fprintf(stderr, "Unknown request, received %ld bytes\n Send: ", data_len);
    for (int i = 0; i < data_len; i++)
    {
        fprintf(stderr, "%d ", data[i]);
    }
}

void handle_get_scoreboard(conn_data_t* conn_data, char* data, size_t data_len)
{
    PGconn* conn = PQconnectdb(dbconninfo);
    Packet* packet = decode_packet(data, data_len);

    if (packet->header->packet_type != PACKET_SCOREBOARD)
    {
        logger(MAIN_LOG, "Error", "Handle get scoreboard: invalid packet type");
    }

    dbScoreboard* scoreboard = dbGetScoreBoard(conn);

    RawBytes* raw_bytes = encode_scoreboard_response(scoreboard);
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_SCOREBOARD, raw_bytes->data, raw_bytes->len);
    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
    {
        logger(MAIN_LOG, "Error", "Handle get scoreboard: Cannot send response");
    }

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    PQfinish(conn);
}

void handle_get_friendlist(conn_data_t* conn_data, char* data, size_t data_len)
{
    PGconn* conn = PQconnectdb(dbconninfo);
    Packet* packet = decode_packet(data, data_len);

    if (packet->header->packet_type != PACKET_FRIENDLIST)
    {
        logger(MAIN_LOG, "Error", "Handle get friendlist: invalid packet type");
    }

    if (packet->header->packet_len != data_len)
    {
        logger(MAIN_LOG, "Error", "Handle get friendlist: invalid packet length");
    }

    if (conn_data->user_id == 0)
    {
        logger(MAIN_LOG, "Error", "Handle get friendlist: User not logged in");
    }

    FriendList* friendlist = dbGetFriendList(conn, conn_data->user_id);

    RawBytes* raw_bytes = encode_friendlist_response(friendlist);
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_FRIENDLIST, raw_bytes->data, raw_bytes->len);
    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
    {
        logger(MAIN_LOG, "Error", "Handle get friendlist: Cannot send response");
    }

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
    PQfinish(conn);
}

void handle_leave_table_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Leave table request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_LEAVE_TABLE) {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }
    
    // Validate user is at a table
    if (conn_data->table_id == 0) {
        RawBytes* raw_bytes = encode_response(R_LEAVE_TABLE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_LEAVE_TABLE, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*)&response->len);
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        logger_ex(MAIN_LOG, "WARN", __func__, "User not at a table", 1);
        return;
    }
    
    int old_table_id = conn_data->table_id;
    int result = leave_table(conn_data, table_list);
    
    RawBytes* raw_bytes;
    if (result == 0) {
        raw_bytes = encode_response(R_LEAVE_TABLE_OK);
        snprintf(log_msg, sizeof(log_msg), "Leave table SUCCESS: user='%s' left table_id=%d", 
                 conn_data->username, old_table_id);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    } else {
        raw_bytes = encode_response(R_LEAVE_TABLE_NOT_OK);
        snprintf(log_msg, sizeof(log_msg), "Leave table FAILED: user='%s' result=%d", 
                 conn_data->username, result);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }
    
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_LEAVE_TABLE, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*)&response->len);
    
    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free_packet(packet);
}

void handle_action_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Action request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    // Validate user is logged in and at a table
    if (conn_data->user_id == 0 || conn_data->table_id == 0) {
        ActionResult result = {
            .result = 403,
            .client_seq = 0,
        };
        strncpy(result.reason, "Not logged in or not at a table", sizeof(result.reason) - 1);
        
        RawBytes* result_bytes = encode_action_result(&result);
        if (result_bytes) {
            RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                              result_bytes->data, result_bytes->len);
            if (response) {
                sendall(conn_data->fd, response->data, (int*)&response->len);
                free(response->data);
                free(response);
            }
            free(result_bytes->data);
            free(result_bytes);
        }
        return;
    }
    
    // Find the table
    int table_idx = find_table_by_id(table_list, conn_data->table_id);
    if (table_idx < 0) {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Table not found", 1);
        return;
    }
    
    Table* table = &table_list->tables[table_idx];
    GameState* gs = table->game_state;
    
    if (!gs) {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Game state not found", 1);
        return;
    }
    
    // Decode action request
    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_ACTION_REQUEST) {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }
    
    ActionRequest* action_req = decode_action_request(packet->data);
    if (!action_req) {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to decode action request", 1);
        free_packet(packet);
        return;
    }
    
    snprintf(log_msg, sizeof(log_msg), "Action from user='%s': type='%s' amount=%d", 
             conn_data->username, action_req->action_type, action_req->amount);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    // Validate it's the player's turn
    if (gs->active_seat < 0 || gs->players[gs->active_seat].player_id != conn_data->user_id) {
        ActionResult result = {
            .result = 403,
            .client_seq = action_req->client_seq,
        };
        strncpy(result.reason, "Not your turn", sizeof(result.reason) - 1);
        
        RawBytes* result_bytes = encode_action_result(&result);
        if (result_bytes) {
            RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                              result_bytes->data, result_bytes->len);
            if (response) {
                sendall(conn_data->fd, response->data, (int*)&response->len);
                free(response->data);
                free(response);
            }
            free(result_bytes->data);
            free(result_bytes);
        }
        
        free(action_req);
        free_packet(packet);
        return;
    }
    
    // Convert action type string to ActionType enum
    Action action = {0};
    if (strcmp(action_req->action_type, "fold") == 0) {
        action.type = ACTION_FOLD;
    } else if (strcmp(action_req->action_type, "check") == 0) {
        action.type = ACTION_CHECK;
    } else if (strcmp(action_req->action_type, "call") == 0) {
        action.type = ACTION_CALL;
    } else if (strcmp(action_req->action_type, "bet") == 0) {
        action.type = ACTION_BET;
        action.amount = action_req->amount;
    } else if (strcmp(action_req->action_type, "raise") == 0) {
        action.type = ACTION_RAISE;
        action.amount = action_req->amount;
    } else if (strcmp(action_req->action_type, "all_in") == 0) {
        action.type = ACTION_ALL_IN;
    } else {
        ActionResult result = {
            .result = 400,
            .client_seq = action_req->client_seq,
        };
        strncpy(result.reason, "Invalid action type", sizeof(result.reason) - 1);
        
        RawBytes* result_bytes = encode_action_result(&result);
        if (result_bytes) {
            RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                              result_bytes->data, result_bytes->len);
            if (response) {
                sendall(conn_data->fd, response->data, (int*)&response->len);
                free(response->data);
                free(response);
            }
            free(result_bytes->data);
            free(result_bytes);
        }
        
        free(action_req);
        free_packet(packet);
        return;
    }
    
    // Validate action
    ActionValidation validation = game_validate_action(gs, conn_data->user_id, &action);
    if (!validation.is_valid) {
        ActionResult result = {
            .result = 409,
            .client_seq = action_req->client_seq,
        };
        strncpy(result.reason, validation.error_message ? validation.error_message : "Invalid action", 
                sizeof(result.reason) - 1);
        
        RawBytes* result_bytes = encode_action_result(&result);
        if (result_bytes) {
            RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                              result_bytes->data, result_bytes->len);
            if (response) {
                sendall(conn_data->fd, response->data, (int*)&response->len);
                free(response->data);
                free(response);
            }
            free(result_bytes->data);
            free(result_bytes);
        }
        
        free(action_req);
        free_packet(packet);
        return;
    }
    
    // Process the action
    int process_result = game_process_action(gs, conn_data->user_id, &action);
    if (process_result != 0) {
        ActionResult result = {
            .result = 500,
            .client_seq = action_req->client_seq,
        };
        strncpy(result.reason, "Failed to process action", sizeof(result.reason) - 1);
        
        RawBytes* result_bytes = encode_action_result(&result);
        if (result_bytes) {
            RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                              result_bytes->data, result_bytes->len);
            if (response) {
                sendall(conn_data->fd, response->data, (int*)&response->len);
                free(response->data);
                free(response);
            }
            free(result_bytes->data);
            free(result_bytes);
        }
        
        free(action_req);
        free_packet(packet);
        return;
    }
    
    // Action processed successfully
    ActionResult result = {
        .result = 0,
        .client_seq = action_req->client_seq,
    };
    
    RawBytes* result_bytes = encode_action_result(&result);
    if (result_bytes) {
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACTION_RESULT, 
                                          result_bytes->data, result_bytes->len);
        if (response) {
            sendall(conn_data->fd, response->data, (int*)&response->len);
            free(response->data);
            free(response);
        }
        free(result_bytes->data);
        free(result_bytes);
    }
    
    // Broadcast updated game state to all players at the table
    int broadcast_count = broadcast_game_state_to_table(table);
    if (broadcast_count <= 0) {
        snprintf(log_msg, sizeof(log_msg), "Warning: Failed to broadcast game state after action from user='%s'", 
                 conn_data->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    } else {
        // Update table's active_seat tracker after broadcasting
        table->active_seat = table->game_state->active_seat;
        
        // Check if hand is complete (showdown finished)
        if (table->game_state->betting_round == BETTING_ROUND_COMPLETE) {
            table->active_seat = -1;
            
            // Remove players who have no money left (busted out)
            // Only remove after hand is complete, not during play
            // Iterate backwards to safely remove multiple players
            for (int i = table->current_player - 1; i >= 0; i--) {
                if (table->connections[i] != NULL && table->connections[i]->seat >= 0) {
                    int seat = table->connections[i]->seat;
                    GamePlayer* p = &table->game_state->players[seat];
                    
                    // Only remove if player has no money left and is not already empty
                    if (p->money == 0 && p->state != PLAYER_STATE_EMPTY) {
                        snprintf(log_msg, sizeof(log_msg), "Player %s (seat %d) busted out at table %d (money=0), removing from table", 
                                 table->connections[i]->username, seat, table->id);
                        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
                        
                        // Remove player from game state
                        game_remove_player(table->game_state, seat);
                        
                        // Remove from connection tracking
                        table->seat_to_conn_idx[seat] = -1;
                        
                        // Mark connection as leaving
                        table->connections[i]->table_id = 0;
                        table->connections[i]->seat = -1;
                        table->connections[i] = NULL;
                        
                        // Shift connections array to remove gap
                        for (int j = i; j < table->current_player - 1; j++) {
                            table->connections[j] = table->connections[j + 1];
                            if (table->connections[j] != NULL && table->connections[j]->seat >= 0) {
                                table->seat_to_conn_idx[table->connections[j]->seat] = j;
                            }
                        }
                        table->connections[table->current_player - 1] = NULL;
                        
                        // Decrease current_player count
                        if (table->current_player > 0) {
                            table->current_player--;
                        }
                    }
                }
            }
            
            // Check if winner cleaned out the lobby
            // Count players who are not folded and have money > 0 (can continue playing)
            int players_with_money = 0;
            int winner_seat = table->game_state->winner_seat;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                GamePlayer* p = &table->game_state->players[i];
                if (p->state != PLAYER_STATE_EMPTY && 
                    p->state != PLAYER_STATE_FOLDED && 
                    p->money > 0) {
                    players_with_money++;
                }
            }
            
            // If only winner has money (or no one has money but there's a winner), remove table
            if (players_with_money <= 1 && winner_seat >= 0) {
                snprintf(log_msg, sizeof(log_msg), "Table %d cleaned out by winner (seat %d, players_with_money=%d, current_player=%d), removing table", 
                         table->id, winner_seat, players_with_money, table->current_player);
                logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
                
                // Mark all connections as leaving the table
                for (int i = 0; i < table->current_player; i++) {
                    if (table->connections[i] != NULL) {
                        table->connections[i]->table_id = 0;
                        table->connections[i]->seat = -1;
                    }
                }
                
                // Clean up game state
                if (table->game_state) {
                    game_state_destroy(table->game_state);
                    table->game_state = NULL;
                }
                
                // Remove table from list
                remove_table(table_list, table->id);
            } else {
                snprintf(log_msg, sizeof(log_msg), "Hand completed at table %d (players_with_money=%d, current_player=%d), preparing for next hand", 
                         table->id, players_with_money, table->current_player);
                logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
                
                // Try to start next hand if enough players remain
                start_game_if_ready(table);
            }
        }
    }
    
    snprintf(log_msg, sizeof(log_msg), "Action processed successfully for user='%s'", conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    free(action_req);
    free_packet(packet);
}
// ===== Friend Management Handlers =====

void handle_add_friend_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Add friend request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_ADD_FRIEND_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ADD_FRIEND, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_ADD_FRIEND)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    AddFriendRequest* request = decode_add_friend_request(packet->data);
    if (!request)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to decode request", 1);
        free_packet(packet);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "User '%s' adding friend '%s'", 
             conn_data->username, request->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    PGconn* conn = PQconnectdb(dbconninfo);
    int res = dbAddFriend(conn, conn_data->user_id, request->username);
    PQfinish(conn);

    RawBytes* raw_bytes;
    if (res == DB_OK)
    {
        raw_bytes = encode_response(R_ADD_FRIEND_OK);
        snprintf(log_msg, sizeof(log_msg), "Add friend SUCCESS: user='%s' added '%s'", 
                 conn_data->username, request->username);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    }
    else if (res == -1)
    {
        raw_bytes = encode_response_msg(R_ADD_FRIEND_NOT_OK, "User not found");
        snprintf(log_msg, sizeof(log_msg), "Add friend FAILED: user '%s' not found", request->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else if (res == -2)
    {
        raw_bytes = encode_response_msg(R_ADD_FRIEND_NOT_OK, "Cannot add yourself");
        logger_ex(MAIN_LOG, "WARN", __func__, "User tried to add themselves", 1);
    }
    else if (res == -3)
    {
        raw_bytes = encode_response(R_ADD_FRIEND_ALREADY_EXISTS);
        snprintf(log_msg, sizeof(log_msg), "Add friend FAILED: already friends with '%s'", 
                 request->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else
    {
        raw_bytes = encode_response(R_ADD_FRIEND_NOT_OK);
        snprintf(log_msg, sizeof(log_msg), "Add friend FAILED: error=%d", res);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }

    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ADD_FRIEND, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(request);
    free_packet(packet);
}

void handle_invite_friend_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Invite friend request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_INVITE_FRIEND_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_INVITE_FRIEND, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_INVITE_FRIEND)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    InviteFriendRequest* request = decode_invite_friend_request(packet->data);
    if (!request)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to decode request", 1);
        free_packet(packet);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "User '%s' inviting '%s'", 
             conn_data->username, request->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    PGconn* conn = PQconnectdb(dbconninfo);
    int res = dbSendFriendInvite(conn, conn_data->user_id, request->username);
    PQfinish(conn);

    RawBytes* raw_bytes;
    if (res == DB_OK)
    {
        raw_bytes = encode_response(R_INVITE_FRIEND_OK);
        snprintf(log_msg, sizeof(log_msg), "Invite friend SUCCESS: user='%s' invited '%s'", 
                 conn_data->username, request->username);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    }
    else if (res == -1)
    {
        raw_bytes = encode_response_msg(R_INVITE_FRIEND_NOT_OK, "User not found");
        snprintf(log_msg, sizeof(log_msg), "Invite friend FAILED: user '%s' not found", 
                 request->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else if (res == -2)
    {
        raw_bytes = encode_response_msg(R_INVITE_FRIEND_NOT_OK, "Cannot invite yourself");
        logger_ex(MAIN_LOG, "WARN", __func__, "User tried to invite themselves", 1);
    }
    else if (res == -3)
    {
        raw_bytes = encode_response_msg(R_INVITE_FRIEND_NOT_OK, "Already friends");
        snprintf(log_msg, sizeof(log_msg), "Invite friend FAILED: already friends with '%s'", 
                 request->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else if (res == -4)
    {
        raw_bytes = encode_response(R_INVITE_ALREADY_SENT);
        snprintf(log_msg, sizeof(log_msg), "Invite friend FAILED: invite already sent to '%s'", 
                 request->username);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else
    {
        raw_bytes = encode_response(R_INVITE_FRIEND_NOT_OK);
        snprintf(log_msg, sizeof(log_msg), "Invite friend FAILED: error=%d", res);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }

    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_INVITE_FRIEND, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(request);
    free_packet(packet);
}

void handle_accept_invite_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Accept invite request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_ACCEPT_INVITE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACCEPT_INVITE, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_ACCEPT_INVITE)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    InviteActionRequest* request = decode_invite_action_request(packet->data);
    if (!request)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to decode request", 1);
        free_packet(packet);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "User '%s' accepting invite_id=%d", 
             conn_data->username, request->invite_id);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    PGconn* conn = PQconnectdb(dbconninfo);
    int res = dbAcceptFriendInvite(conn, conn_data->user_id, request->invite_id);
    PQfinish(conn);

    RawBytes* raw_bytes;
    if (res == DB_OK)
    {
        raw_bytes = encode_response(R_ACCEPT_INVITE_OK);
        snprintf(log_msg, sizeof(log_msg), "Accept invite SUCCESS: user='%s' invite_id=%d", 
                 conn_data->username, request->invite_id);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    }
    else if (res == -1)
    {
        raw_bytes = encode_response_msg(R_ACCEPT_INVITE_NOT_OK, "Invite not found");
        snprintf(log_msg, sizeof(log_msg), "Accept invite FAILED: invite_id=%d not found", 
                 request->invite_id);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else if (res == -2)
    {
        raw_bytes = encode_response_msg(R_ACCEPT_INVITE_NOT_OK, "Invite already processed");
        snprintf(log_msg, sizeof(log_msg), "Accept invite FAILED: invite_id=%d already processed", 
                 request->invite_id);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else
    {
        raw_bytes = encode_response(R_ACCEPT_INVITE_NOT_OK);
        snprintf(log_msg, sizeof(log_msg), "Accept invite FAILED: error=%d", res);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }

    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_ACCEPT_INVITE, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(request);
    free_packet(packet);
}

void handle_reject_invite_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Reject invite request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_REJECT_INVITE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_REJECT_INVITE, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_REJECT_INVITE)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    InviteActionRequest* request = decode_invite_action_request(packet->data);
    if (!request)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to decode request", 1);
        free_packet(packet);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "User '%s' rejecting invite_id=%d", 
             conn_data->username, request->invite_id);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    PGconn* conn = PQconnectdb(dbconninfo);
    int res = dbRejectFriendInvite(conn, conn_data->user_id, request->invite_id);
    PQfinish(conn);

    RawBytes* raw_bytes;
    if (res == DB_OK)
    {
        raw_bytes = encode_response(R_REJECT_INVITE_OK);
        snprintf(log_msg, sizeof(log_msg), "Reject invite SUCCESS: user='%s' invite_id=%d", 
                 conn_data->username, request->invite_id);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    }
    else if (res == -1)
    {
        raw_bytes = encode_response_msg(R_REJECT_INVITE_NOT_OK, "Invite not found");
        snprintf(log_msg, sizeof(log_msg), "Reject invite FAILED: invite_id=%d not found", 
                 request->invite_id);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else if (res == -2)
    {
        raw_bytes = encode_response_msg(R_REJECT_INVITE_NOT_OK, "Invite already processed");
        snprintf(log_msg, sizeof(log_msg), "Reject invite FAILED: invite_id=%d already processed", 
                 request->invite_id);
        logger_ex(MAIN_LOG, "WARN", __func__, log_msg, 1);
    }
    else
    {
        raw_bytes = encode_response(R_REJECT_INVITE_NOT_OK);
        snprintf(log_msg, sizeof(log_msg), "Reject invite FAILED: error=%d", res);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
    }

    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_REJECT_INVITE, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(request);
    free_packet(packet);
}

void handle_get_invites_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Get invites request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_GET_INVITES_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_INVITES, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_GET_INVITES)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    PGconn* conn = PQconnectdb(dbconninfo);
    dbInviteList* invites = dbGetPendingInvites(conn, conn_data->user_id);
    PQfinish(conn);

    if (!invites)
    {
        RawBytes* raw_bytes = encode_response(R_GET_INVITES_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_INVITES, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to get invites from database", 1);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "Get invites SUCCESS: user='%s' has %d pending invites", 
             conn_data->username, invites->num);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    RawBytes* raw_bytes = encode_invites_response(invites);
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_INVITES, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(invites->invites);
    free(invites);
    free_packet(packet);
}

void handle_get_friend_list_request(conn_data_t* conn_data, char* data, size_t data_len)
{
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Get friend list request from fd=%d user='%s'", 
             conn_data->fd, conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    if (conn_data->user_id == 0)
    {
        RawBytes* raw_bytes = encode_response(R_GET_FRIEND_LIST_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_FRIEND_LIST, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        logger_ex(MAIN_LOG, "ERROR", __func__, "User not logged in", 1);
        return;
    }

    Packet* packet = decode_packet(data, data_len);
    if (!packet || packet->header->packet_type != PACKET_GET_FRIEND_LIST)
    {
        logger_ex(MAIN_LOG, "ERROR", __func__, "Invalid packet", 1);
        if (packet) free_packet(packet);
        return;
    }

    PGconn* conn = PQconnectdb(dbconninfo);
    dbFriendList* friends = dbGetFriendList(conn, conn_data->user_id);
    PQfinish(conn);

    if (!friends)
    {
        RawBytes* raw_bytes = encode_response(R_GET_FRIEND_LIST_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_FRIEND_LIST, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int*) &(response->len));
        free(response->data);
        free(response);
        free(raw_bytes->data);
        free(raw_bytes);
        free_packet(packet);
        logger_ex(MAIN_LOG, "ERROR", __func__, "Failed to get friend list from database", 1);
        return;
    }

    snprintf(log_msg, sizeof(log_msg), "Get friend list SUCCESS: user='%s' has %d friends", 
             conn_data->username, friends->num);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    RawBytes* raw_bytes = encode_friend_list_response(friends);
    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_GET_FRIEND_LIST, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int*) &(response->len));

    free(response->data);
    free(response);
    free(raw_bytes->data);
    free(raw_bytes);
    free(friends->friends);
    free(friends);
    free_packet(packet);
}
