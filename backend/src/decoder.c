#include "main.h"

Header *decode_header(char *data)
{
    Header *header = malloc(sizeof(Header));

    header->packet_len = (data[0] << 8) | data[1];
    header->protocol_ver = data[2];
    header->packet_type = (data[3] << 8) | data[4];

    return header;
}

Packet *decode_packet(char *data)
{
    Packet *packet = malloc(sizeof(Packet));

    packet->header = *decode_header(data);
    memccpy(packet->data, data + sizeof(Header), packet->header.packet_len - sizeof(Header), packet->header.packet_len - sizeof(Header));

    return packet;
}