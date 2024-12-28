#include "main.h"

char *encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char *payload, size_t payload_len)
{
    // header is all numbers, we need to turn it into bytes: length is __uint16_t, protocol_ver is __uint8_t, packet_type is __uint16_t
    // payload is a string, we need to turn it into bytes
    // we need to allocate a new buffer to store the bytes

    Header *header = malloc(sizeof(Header));

    header->packet_len = payload_len + sizeof(Header);
    header->protocol_ver = protocol_ver;
    header->packet_type = packet_type;

    char *buffer = malloc(header->packet_len + sizeof(Header));

    // packet length is 2 bytes
    buffer[0] = (header->packet_len >> 8) & 0xFF;
    buffer[1] = header->packet_len & 0xFF;

    // protocol version is 1 byte
    buffer[2] = header->protocol_ver;

    // packet type is 2 bytes
    buffer[3] = (header->packet_type >> 8) & 0xFF;
    buffer[4] = header->packet_type & 0xFF;

    // copy the payload into the buffer
    memcpy(buffer + sizeof(Header), payload, header->packet_len - sizeof(Header));
    free(header);

    return buffer;
}