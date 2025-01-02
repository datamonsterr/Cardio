#include "main.h"

void handle_login_request(conn_data_t *conn_data, char *data, size_t data_len)
{
    PGconn *conn = PQconnectdb(dbconninfo);
    Packet *packet = decode_packet(data, data_len);
    if (packet->header->packet_type != 100)
    {
        logger(MAIN_LOG, "Error", "Handle login: invalid packet type");
    }

    if (packet->header->packet_len != data_len)
    {
        logger(MAIN_LOG, "Error", "Handle login: invalid packet length");
    }

    LoginRequest *login_request = decode_login_request(packet->data);
    int user_id = dbLogin(conn, login_request->username, login_request->password);
    if (user_id > 0)
    {
        RawBytes *raw_bytes = encode_response(R_LOGIN_OK);
        RawBytes *response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int *)&(response->len));

        struct dbUser user_info = dbGetUserInfo(conn, user_id);
        strncpy(conn_data->username, user_info.username, 32);
        conn_data->user_id = user_id;
        conn_data->is_active = true;
        conn_data->balance = user_info.balance;

        free(response);
        free(raw_bytes);
        free(login_request);
        logger(MAIN_LOG, "Info", "Handle login: Login success");
        char *log_msg = malloc(100);
        sprintf(log_msg, "Handle login: Login success from socket %d\n", conn_data->fd);
        logger(MAIN_LOG, "Info", log_msg);
        return;
    }

    RawBytes *raw_bytes = encode_response(R_LOGIN_NOT_OK);
    RawBytes *response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int *)&(response->len));
    char *log_msg = malloc(100);
    sprintf(log_msg, "Handle login: Login failed from socket %d\n", conn_data->fd);
    logger(MAIN_LOG, "Error", log_msg);

    PQfinish(conn);

    free(response);
    free(raw_bytes);
    free_packet(packet);
    free(login_request);
}

void handle_signup_request(conn_data_t *conn_data, char *data, size_t data_len)
{
    PGconn *conn = PQconnectdb(dbconninfo);
    Packet *packet = decode_packet(data, data_len);
    if (packet->header->packet_type != 200)
    {
        logger(MAIN_LOG, "Error", "Handle signup: invalid packet type");
    }

    if (packet->header->packet_len != data_len)
    {
        logger(MAIN_LOG, "Error", "Handle signup: invalid packet length");
    }

    SignupRequest *signup_request = decode_signup_request(packet->data);
    struct dbUser *user = malloc(sizeof(struct dbUser));
    strncpy(user->username, signup_request->username, 32);
    strncpy(user->password, signup_request->password, 32);
    strncpy(user->phone, signup_request->phone, 16);
    strncpy(user->email, signup_request->email, 64);
    strncpy(user->phone, signup_request->phone, 16);
    strncpy(user->fullname, signup_request->fullname, 64);
    strncpy(user->country, signup_request->country, 32);
    strncpy(user->gender, signup_request->gender, 8);
    int res = dbSignup(conn, user);

    if (res == DB_OK)
    {
        RawBytes *raw_bytes = encode_response(R_SIGNUP_OK);
        RawBytes *response = encode_packet(PROTOCOL_V1, 200, raw_bytes->data, raw_bytes->len);
        sendall(conn_data->fd, response->data, (int *)&(response->len));

        free(response);
        free(raw_bytes);
        free(signup_request);
        logger(MAIN_LOG, "Info", "Handle signup: Signup success");
        return;
    }

    RawBytes *raw_bytes = encode_response(R_SIGNUP_NOT_OK);
    RawBytes *response = encode_packet(PROTOCOL_V1, 200, raw_bytes->data, raw_bytes->len);
    sendall(conn_data->fd, response->data, (int *)&(response->len));
    logger(MAIN_LOG, "Error", "Handle signup: Signup failed");

    PQfinish(conn);

    free(response);
    free(raw_bytes);
    free_packet(packet);
    free(signup_request);
}