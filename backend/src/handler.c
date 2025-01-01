#include "main.h"

int handle_login_request(conn_data_t *conn_data, int client_fd, char *data, size_t data_len)
{
    PGconn *conn = PQconnectdb(dbconninfo);
    Packet *packet = decode_packet(data);
    if (packet->header->packet_type != 100)
    {
        fprintf(stderr, "Handle login: invalid packet type\n");
        return -1;
    }

    if (packet->header->packet_len != data_len)
    {
        fprintf(stderr, "Handle login: invalid packet length\n");
        return -1;
    }

    LoginRequest *login_request = decode_login_request(packet->data);
    int res = dbLogin(conn, login_request->username, login_request->password);
    if (res == LOGIN_OK)
    {
        RawBytes *raw_bytes = encode_response(R_LOGIN_OK);
        RawBytes *response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
        sendall(client_fd, response->data, (int *)&(response->len));

        struct dbUser user_info = dbGetUserInfo(conn, login_request->username);
        memccpy(conn_data->username, login_request->username, 0, 32);
        conn_data->is_active = true;
        conn_data->balance = user_info.balance;
        conn_data->user_id = user_info.player_id;

        free(response);
        free(raw_bytes);
        free(login_request);
        fprintf(stdout, "Handle login: Login success from socket %d\n", client_fd);
        fprintf(stdout, "Username: %s\n User ID: %d\n Balance: %f\n", conn_data->username, conn_data->user_id, conn_data->balance);
        return LOGIN_OK;
    }

    RawBytes *raw_bytes = encode_response(R_LOGIN_NOT_OK);
    RawBytes *response = encode_packet(PROTOCOL_V1, 100, raw_bytes->data, raw_bytes->len);
    sendall(client_fd, response->data, (int *)&(response->len));
    fprintf(stderr, "Handle login: Login failed from socket %d\n", client_fd);

    PQfinish(conn);

    free(response);
    free(raw_bytes);
    free_packet(packet);
    free(login_request);

    return -1;
}