#pragma once
#include "game.h"
#include "db.h"
#include "../lib/db/include/db.h"

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
#define R_JOIN_TABLE_FULL 403

#define PACKET_TABLES 500

#define PACKET_UPDATE_GAMESTATE 600

#define PACKET_LEAVE_TABLE 700
#define R_LEAVE_TABLE_OK 701
#define R_LEAVE_TABLE_NOT_OK 702
#define R_LEAVE_TABLE_EMPTY 703

#define PACKET_SCOREBOARD 800
#define R_SCOREBOARD_OK 801
#define R_SCOREBOARD_NOT_OK 802

#define PACKET_FRIENDLIST 900
#define R_FRIENDLIST_OK 901
#define R_FRIENDLIST_NOT_OK 902

typedef struct
{
    char *data;
    size_t len;
} RawBytes;

typedef struct __attribute__((packed))
{
    __uint16_t packet_len;
    __uint8_t protocol_ver;
    __uint16_t packet_type;
} Header;

struct
{
    Header *header;
    char *data;
} typedef Packet;

// Decode first 5 bytes of the packet and turn it into a Header struct. Also, apply network to host byte order conversion
Header *decode_header(char *data);
// Split the packet into header and payload
Packet *decode_packet(char *data, size_t data_len);
void free_packet(Packet *packet);

// Encode a packet with the given protocol version, packet type, payload and payload length. Apply host to network byte order conversion in the header
RawBytes *encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char *payload, size_t payload_len);

struct LoginRequest
{
    char username[32];
    char password[32];
} typedef LoginRequest;

LoginRequest *decode_login_request(char *data);
RawBytes *encode_response_msg(int res, char *msg);
RawBytes *encode_response(int res);

struct SignupRequest
{
    char username[32];
    char password[32];
    char fullname[64];
    char email[64];
    char phone[16];
    char dob[16];
    char country[32];
    char gender[8];
} typedef SignupRequest;

// Decode signup request
SignupRequest *decode_signup_request(char *payload);

struct CreateTableRequest
{
    char table_name[32];
    int max_player;
    int min_bet;
} typedef CreateTableRequest;

// Decode create TABLE request
// Table request will be in the form of: {"name": "table_name", "max_player": 5, "min_bet": 100}
CreateTableRequest *decode_create_table_request(char *payload);
// Find all tables in table list and encode them into a response which is an array of tables
// For example: [{"id": 1, "name": "Table 1", "max_player": 5, "min_bet": 100, "current_player": 1}, ...]
RawBytes *encode_full_tables_response(TableList *table_list);
// Decode join TABLE request
int decode_join_table_request(char *payload);
// Decode leave Table resquest
int decode_leave_table_request(char *payload);

// Todo
// Decode refresh tables request
RawBytes *decode_refresh_tables_request(char *payload);

// Encode update tables response
RawBytes *encode_update_tables_response(TableList *new_tables);

// Encode scoreboard response
RawBytes *encode_scoreboard_response(dbScoreboard *dbScoreboard);

// Encode friendlist response
RawBytes *encode_friendlist_response(FriendList *friendlist);

// Encode login success response
RawBytes *encode_login_success_response(dbUser *user);