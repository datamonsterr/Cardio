#include "main.h"

int handle_login_request(int client_fd, char *data, size_t data_len)
{
    PGconn *conn = PQconnectdb(dbconninfo);
    Packet *packet = decode_packet(data);
    if (packet->header->packet_type != 100)
    {
        return -1;
    }

    if (packet->header->packet_len != data_len)
    {
        return -1;
    }

    LoginRequest *login_request = decode_login_request(packet->data);
    int res = dbLogin(conn, login_request->username, login_request->password);
    if (res == LOGIN_OK)
    {
        char *msg = "Login successful";
        char *response = encode_packet(PROTOCOL_V1, 100, msg, strlen(msg));
        sendall(client_fd, response, 16);
        free(response);
    }
    else
    {
        char *msg = encode_error_message("Login failed");
        char *response = encode_packet(PROTOCOL_V1, 100, msg, strlen(msg));
        sendall(client_fd, response, 13);
        free(response);
    }

    PQfinish(conn);

    free_packet(packet);
}