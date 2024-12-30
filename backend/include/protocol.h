#pragma once
#define MAXLINE 66531

#define PROTOCOL_V1 0x01

// Packet code are 16-bit unsigned integer, so the range is 0 - 65535
#define PACKET_LOGIN 100
#define PACKET_SIGNUP 200
#define PACKET_CREATE_ROOM 300
#define PACKET_JOIN_ROOM 400
#define PACKET_REFRESH_TABLES 500
#define PACKET_UPDATE_GS 600
#define PACKET_LEAVE_ROOM 700

typedef struct __attribute__((packed))
{
    __uint16_t packet_len;
    __uint8_t protocol_ver;
    __uint16_t packet_type;
} Header;

struct
{
    Header *header;
    char data[MAXLINE];
} typedef Packet;

Header *decode_header(char *data);
Packet *decode_packet(char *data);
void free_packet(Packet *packet);

char *encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char *payload, size_t payload_len);

struct
{
    char username[30];
    char password[30];
} typedef LoginRequest;

LoginRequest *decode_login_request(char *data);
char *encode_error_message(char *msg);