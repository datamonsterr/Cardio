#pragma once
#include "game.h"

#define MAXLINE 65540
#define PROTOCOL_V1 0x01

// Packet code are 16-bit unsigned integer, so the range is 0 - 65535
#define PACKET_LOGIN 100
#define R_LOGIN_OK 101
#define R_LOGIN_NOT_OK 102

#define PACKET_SIGNUP 200
#define R_SIGNUP_OK 201
#define R_SIGNUP_NOT_OK 202

#define PACKET_CREATE_TABLE 300
#define R_CREATE_TABLE_OK 301
#define R_CREATE_TABLE_NOT_OK 302

#define PACKET_JOIN_TABLE 400
#define R_JOIN_TABLE_OK 401
#define R_JOIN_TABLE_NOT_OK 402

#define PACKET_TABLES 500

#define PACKET_UPDATE_GAMESTATE 600

#define PACKET_LEAVE_TABLE 700
#define R_LEAVE_TABLE_OK 701
#define R_LEAVE_TABLE_NOT_OK 702

struct
{
    char *data;
    size_t len;
} typedef RawBytes;

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

RawBytes *encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char *payload, size_t payload_len);

struct
{
    char username[32];
    char password[32];
} typedef LoginRequest;

LoginRequest *decode_login_request(char *data);
RawBytes *encode_response_msg(int res, char *msg);
RawBytes *encode_response(int res);

// Todo

// Decode signup request
RawBytes *decode_signup_request(char *payload);

// Decode create TABLE request
RawBytes *decode_create_table_request(char *payload);

// Decode join TABLE request
RawBytes *decode_join_table_request(char *payload);

// Decode refresh tables request
RawBytes *decode_refresh_tables_request(char *payload);

// Encode update tables response
RawBytes *encode_update_tables_response(TableList *new_tables);

// Encode Full Table
RawBytes *encode_full_tables_response(TableList *table_list, int table_id);

// Encode scoreboard response
RawBytes *encode_scoreboard_response();