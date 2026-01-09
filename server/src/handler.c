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
    if (user_id > 0)
    {
        struct dbUser user_info = dbGetUserInfo(conn, user_id);
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
    strncpy(user->username, signup_request->username, 32);
    strncpy(user->password, signup_request->password, 32);
    strncpy(user->phone, signup_request->phone, 16);
    strncpy(user->email, signup_request->email, 64);
    strncpy(user->phone, signup_request->phone, 16);
    strncpy(user->fullname, signup_request->fullname, 64);
    strncpy(user->country, signup_request->country, 32);
    strncpy(user->gender, signup_request->gender, 8);
    
    snprintf(log_msg, sizeof(log_msg), "Attempting signup for user='%s' email='%s'", 
             signup_request->username, signup_request->email);
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
    
    snprintf(log_msg, sizeof(log_msg), "Signup FAILED: user='%s' fd=%d", signup_request->username, conn_data->fd);
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

    if (conn_data->table_id != 0)
    {
        snprintf(log_msg, sizeof(log_msg), "User already at table (table_id=%d)", conn_data->table_id);
        logger_ex(MAIN_LOG, "ERROR", __func__, log_msg, 1);
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
        free(response);
        free(raw_bytes);
        free_packet(packet);
        return;
    }

    int table_id = decode_join_table_request(packet->data);
    snprintf(log_msg, sizeof(log_msg), "User '%s' attempting to join table_id=%d", 
             conn_data->username, table_id);
    logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);

    int res = join_table(conn_data, table_list, table_id);
    printf("res = %d\n", res);
    RawBytes* raw_bytes = malloc(sizeof(RawBytes));
    RawBytes* response = malloc(sizeof(RawBytes));
    if (res >= 0)
    {
        snprintf(log_msg, sizeof(log_msg), "Join table SUCCESS: user='%s' table_id=%d", 
                 conn_data->username, table_id);
        logger_ex(MAIN_LOG, "INFO", __func__, log_msg, 1);
        raw_bytes = encode_response(R_JOIN_TABLE_OK);
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