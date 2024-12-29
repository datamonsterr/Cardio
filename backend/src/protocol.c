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

LoginRequest *decode_login_request(char *data) // login payload: {username: "vietanh", password: "viet1234"} is already extracted from the packet
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, 1024);

    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "username");
    const char *username = mpack_expect_cstr_alloc(&reader, 30);
    mpack_expect_cstr_match(&reader, "password");
    const char *password = mpack_expect_cstr_alloc(&reader, 30);

    LoginRequest *login_request = malloc(sizeof(LoginRequest));
    strcpy(login_request->username, username);
    strcpy(login_request->password, password);

    return login_request;
}