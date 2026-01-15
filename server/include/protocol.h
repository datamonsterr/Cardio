#pragma once
#include "../lib/db/include/db.h"
#include "db.h"
#include "game.h"

#define MAXLINE 65540
#define PROTOCOL_V1 0x01

// Service packets
#define PACKET_PING 10
#define PACKET_PONG 11

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

#define PACKET_SCOREBOARD 800
#define R_SCOREBOARD_OK 801
#define R_SCOREBOARD_NOT_OK 802

#define PACKET_FRIENDLIST 900
#define R_FRIENDLIST_OK 901
#define R_FRIENDLIST_NOT_OK 902

#define PACKET_ADD_FRIEND 910
#define R_ADD_FRIEND_OK 911
#define R_ADD_FRIEND_NOT_OK 912
#define R_ADD_FRIEND_ALREADY_EXISTS 913

#define PACKET_INVITE_FRIEND 920
#define R_INVITE_FRIEND_OK 921
#define R_INVITE_FRIEND_NOT_OK 922
#define R_INVITE_ALREADY_SENT 923

#define PACKET_ACCEPT_INVITE 930
#define R_ACCEPT_INVITE_OK 931
#define R_ACCEPT_INVITE_NOT_OK 932

#define PACKET_REJECT_INVITE 940
#define R_REJECT_INVITE_OK 941
#define R_REJECT_INVITE_NOT_OK 942

#define PACKET_GET_INVITES 950
#define R_GET_INVITES_OK 951
#define R_GET_INVITES_NOT_OK 952

#define PACKET_GET_FRIEND_LIST 960
#define R_GET_FRIEND_LIST_OK 961
#define R_GET_FRIEND_LIST_NOT_OK 962

// Balance update notification (server -> client)
#define PACKET_BALANCE_UPDATE 970

// Table invite packets (invite friends to join a game table)
#define PACKET_INVITE_TO_TABLE 980
#define PACKET_TABLE_INVITE_NOTIFICATION 985  // Sent to invited user
#define R_INVITE_TO_TABLE_OK 981
#define R_INVITE_TO_TABLE_NOT_OK 982
#define R_INVITE_TO_TABLE_NOT_FRIENDS 983
#define R_INVITE_TO_TABLE_ALREADY_IN_GAME 984

// Game action packets (following protocol spec)
#define PACKET_ACTION_REQUEST 450
#define PACKET_ACTION_RESULT 451
#define PACKET_UPDATE_BUNDLE 460
#define PACKET_RESYNC_REQUEST 470
#define PACKET_RESYNC_RESPONSE 471

typedef struct
{
    char* data;
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
    Header* header;
    char* data;
} typedef Packet;

// Decode first 5 bytes of the packet and turn it into a Header struct. Also, apply network to host byte order
// conversion
Header* decode_header(char* data);
// Split the packet into header and payload
Packet* decode_packet(char* data, size_t data_len);
void free_packet(Packet* packet);

// Encode a packet with the given protocol version, packet type, payload and payload length. Apply host to network byte
// order conversion in the header
RawBytes* encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char* payload, size_t payload_len);

struct LoginRequest
{
    char username[32];
    char password[32];
} typedef LoginRequest;

LoginRequest* decode_login_request(char* data);
RawBytes* encode_response_msg(int res, char* msg);
RawBytes* encode_response(int res);
RawBytes* encode_create_table_response(int res, int table_id);

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
SignupRequest* decode_signup_request(char* payload);

struct CreateTableRequest
{
    char table_name[32];
    int max_player;
    int min_bet;
} typedef CreateTableRequest;

// Decode create TABLE request
// Table request will be in the form of: {"name": "table_name", "max_player": 5, "min_bet": 100}
CreateTableRequest* decode_create_table_request(char* payload);
// Find all tables in table list and encode them into a response which is an array of tables
// For example: [{"id": 1, "name": "Table 1", "max_player": 5, "min_bet": 100, "current_player": 1}, ...]
RawBytes* encode_full_tables_response(TableList* table_list);
// Decode join TABLE request
int decode_join_table_request(char* payload);

// Todo
// Decode refresh tables request
RawBytes* decode_refresh_tables_request(char* payload);

// Encode update tables response
RawBytes* encode_update_tables_response(TableList* new_tables);

// Encode scoreboard response
RawBytes* encode_scoreboard_response(dbScoreboard* dbScoreboard);

// Encode friendlist response
RawBytes* encode_friendlist_response(FriendList* friendlist);

// Encode login success response
RawBytes* encode_login_success_response(dbUser* user);

// ===== Friend Management =====

// Add friend request
typedef struct {
    char username[32];  // Username to add as friend
} AddFriendRequest;

// Invite friend request
typedef struct {
    char username[32];  // Username to invite
} InviteFriendRequest;

// Accept/Reject invite request
typedef struct {
    int invite_id;      // ID of the invite to accept/reject
} InviteActionRequest;

// Decode friend management requests
AddFriendRequest* decode_add_friend_request(char* payload);
InviteFriendRequest* decode_invite_friend_request(char* payload);
InviteActionRequest* decode_invite_action_request(char* payload);

// Encode friend invite list response
RawBytes* encode_invites_response(dbInviteList* invites);

// Encode friend list response
RawBytes* encode_friend_list_response(dbFriendList* friends);

// Encode balance update notification
RawBytes* encode_balance_update_notification(int new_balance, const char* reason);

// ===== Table Invite =====

// Table invite request
typedef struct {
    char friend_username[32];  // Username to invite to table
    int table_id;              // Table ID to invite them to
} TableInviteRequest;

// Decode table invite request
TableInviteRequest* decode_table_invite_request(char* payload);

// ===== Game Action Packets =====

// Action request from client
typedef struct {
    int game_id;
    char action_type[16];  // "fold", "check", "call", "bet", "raise", "all_in"
    int amount;            // For bet/raise actions
    uint32_t client_seq;   // Optional correlation ID
} ActionRequest;

// Action result response
typedef struct {
    int result;            // 0=OK, 400=BAD_ACTION, 403=NOT_YOUR_TURN, etc.
    uint32_t client_seq;   // Echoed from request
    char reason[128];      // Present on failure
} ActionResult;

// Decode action request
ActionRequest* decode_action_request(char* payload);
// Encode action result
RawBytes* encode_action_result(ActionResult* result);

// Encode full game state (for JOIN_TABLE_OK and RESYNC_RESPONSE)
RawBytes* encode_game_state(GameState* state, int viewer_player_id);

// Encode update bundle (for broadcasting game state changes)
RawBytes* encode_update_bundle(uint32_t seq, const char** notifications, int num_notifications,
                                const char** updates, int num_updates);