#pragma once

#include <unistd.h>

#define PROTOCOL_V1 0x01

// Packet code are 16-bit unsigned integer, so the range is 0 - 65535
#define PACKET_LOGIN 100
#define PACKET_SIGNUP 200
#define PACKET_CREATE_ROOM 300
#define PACKET_JOIN_ROOM 400
#define PACKET_REFRESH_TABLES 500
#define PACKET_UPDATE_GS 600
#define PACKET_LEAVE_ROOM 700

struct
{
    __uint16_t packet_len;
    __uint8_t protocol_ver;
    __uint16_t packet_type;
} typedef Header;