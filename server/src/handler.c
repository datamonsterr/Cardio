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

        free(response);
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

    free(response);
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

        free(response);
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

    free(response);
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
        free(response);
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
        raw_bytes = encode_response(R_CREATE_TABLE_OK);
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

    free(response);
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

    free(response);
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

    int table_id = decode_join_table_request(packet->data);
    
    // If user is already at a table, check if it's the same table
    if (conn_data->table_id != 0)
    {
        if (conn_data->table_id == table_id)
        {
            // User is already at this table - send game state instead of rejecting
            snprintf(log_msg, sizeof(log_msg), "User already at table (table_id=%d), sending game state", conn_data->table_id);
            logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
            
            int table_index = find_table_by_id(table_list, table_id);
            if (table_index >= 0)
            {
                Table* table = &table_list->tables[table_index];
                RawBytes* game_state_data = encode_game_state(table->game_state, conn_data->user_id);
                
                if (game_state_data) {
                    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, game_state_data->data, game_state_data->len);
                    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
                    {
                        logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send game state", 1);
                    }
                    free(response);
                    free(game_state_data);
                } else {
                    RawBytes* raw_bytes = encode_response(R_JOIN_TABLE_OK);
                    RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);
                    if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
                    {
                        logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
                    }
                    free(response);
                    free(raw_bytes);
                }
            } else {
                RawBytes* raw_bytes = encode_response(R_JOIN_TABLE_NOT_OK);
                RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);
                if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
                {
                    logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
                }
                free(response);
                free(raw_bytes);
            }
            
            free_packet(packet);
            return;
        }
        else
        {
            // User is at a different table - reject
            snprintf(log_msg, sizeof(log_msg), "User already at different table (table_id=%d, requested=%d)", conn_data->table_id, table_id);
            logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
            is_valid = 0;
        }
    }

    if (is_valid == 0)
    {
        RawBytes* raw_bytes = encode_response(R_JOIN_TABLE_NOT_OK);
        RawBytes* response = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, raw_bytes->data, raw_bytes->len);
        if (sendall(conn_data->fd, response->data, (int*) &(response->len)) == -1)
        {
            logger_ex(MAIN_LOG, "ERROR", __func__, "Cannot send response", 1);
        }
        free(response);
        free(raw_bytes);
        free_packet(packet);
        return;
    }
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
        
        // Send game state to the joining player
        Table* table = &table_list->tables[res];
        RawBytes* game_state_data = encode_game_state(table->game_state, conn_data->user_id);
        
        snprintf(log_msg, sizeof(log_msg), "encode_game_state returned: %p (size=%zu)", 
                 (void*)game_state_data, game_state_data ? game_state_data->len : 0);
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
        
        // Try to start game if we have enough players
        start_game_if_ready(table);
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
    free(response);
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

    free(response);
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

    free(response);
    free(raw_bytes);
    free_packet(packet);
    PQfinish(conn);
}

void handle_leave_table_request(conn_data_t* conn_data, char* data, size_t data_len, TableList* table_list)
{
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
                free(response);
            }
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
                free(response);
            }
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
                free(response);
            }
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
                free(response);
            }
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
                free(response);
            }
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
            free(response);
        }
        free(result_bytes);
    }
    
    // Broadcast updated game state to all players at the table
    for (int i = 0; i < table->current_player; i++) {
        if (table->connections[i] != NULL && table->connections[i]->fd > 0) {
            RawBytes* game_state_data = encode_game_state(gs, table->connections[i]->user_id);
            if (game_state_data) {
                RawBytes* broadcast_packet = encode_packet(PROTOCOL_V1, PACKET_UPDATE_GAMESTATE, 
                                                          game_state_data->data, game_state_data->len);
                if (broadcast_packet) {
                    sendall(table->connections[i]->fd, broadcast_packet->data, (int*)&broadcast_packet->len);
                    free(broadcast_packet);
                }
                free(game_state_data);
            }
        }
    }
    
    snprintf(log_msg, sizeof(log_msg), "Action processed successfully for user='%s'", conn_data->username);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
    
    free(action_req);
    free_packet(packet);
}