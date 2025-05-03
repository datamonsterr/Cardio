#include "main.h"

void free_packet(Packet *packet)
{
    free(packet->header);
    free(packet);
}

RawBytes *encode_packet(__uint8_t protocol_ver, __uint16_t packet_type, char *payload, size_t payload_len)
{
    // Total packet length: header size + payload size
    size_t len = sizeof(Header) + payload_len;

    // Allocate memory for the entire packet
    char *buffer = malloc(len);
    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for packet\n");
        return NULL;
    }

    // Encode the header
    // Packet length (2 bytes)
    __uint16_t packet_len = htons((__uint16_t)len);
    memcpy(buffer, &packet_len, sizeof(packet_len));

    // Protocol version (1 byte)
    buffer[2] = protocol_ver;

    // Packet type (2 bytes, big-endian)
    __uint16_t packet_type_be = htons(packet_type);
    memcpy(buffer + 3, &packet_type_be, sizeof(packet_type_be));

    // Copy the payload into the buffer
    if (payload == NULL)
    {
        if (payload_len != 0)
        {
            fprintf(stderr, "Payload length mismatch\n");
            free(buffer);
            return NULL;
        }
        RawBytes *raw_bytes = malloc(sizeof(RawBytes));
        raw_bytes->data = buffer;
        raw_bytes->len = 5;
        return raw_bytes;
    }
    memcpy(buffer + sizeof(Header), payload, payload_len);

    // Wrap the raw bytes in a structure
    RawBytes *raw_bytes = malloc(sizeof(RawBytes));
    if (raw_bytes == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for RawBytes\n");
        free(buffer);
        return NULL;
    }

    raw_bytes->data = buffer;
    raw_bytes->len = len;

    return raw_bytes;
}

Header *decode_header(char *data)
{
    if (data == NULL)
    {
        fprintf(stderr, "decode_header: NULL data pointer\n");
        return NULL;
    }

    Header *header = malloc(sizeof(Header));
    if (header == NULL)
    {
        fprintf(stderr, "decode_header: Memory allocation failed\n");
        return NULL;
    }

    header->packet_len = ntohs(*(uint16_t *)&data[0]);  // Convert first 2 bytes
    header->protocol_ver = (uint8_t)data[2];            // Single byte (no conversion needed)
    header->packet_type = ntohs(*(uint16_t *)&data[3]); // Convert next 2 bytes

    return header;
}

Packet *decode_packet(char *data, size_t data_len)
{
    if (data == NULL || data_len < sizeof(Header))
    {
        fprintf(stderr, "decode_packet: Invalid data or insufficient data length\n");
        return NULL;
    }

    // Allocate memory for the Packet structure
    Packet *packet = malloc(sizeof(Packet));
    if (packet == NULL)
    {
        fprintf(stderr, "decode_packet: Memory allocation failed for Packet\n");
        return NULL;
    }

    // Decode the header
    packet->header = decode_header(data);
    if (packet->header == NULL)
    {
        fprintf(stderr, "decode_packet: Failed to decode header\n");
        free(packet);
        return NULL;
    }

    // Validate the packet length (convert to host byte order for validation)
    uint16_t packet_len = packet->header->packet_len;
    if (packet_len > data_len)
    {
        fprintf(stderr, "decode_packet: Packet length mismatch (expected: %u, received: %zu)\n",
                packet_len, data_len);
        free(packet->header);
        free(packet);
        return NULL;
    }

    // Calculate payload length
    size_t payload_len = packet_len - sizeof(Header);
    if (payload_len > 0)
    {
        // Allocate memory for the payload
        packet->data = malloc(payload_len);
        if (packet->data == NULL)
        {
            fprintf(stderr, "decode_packet: Memory allocation failed for payload\n");
            free(packet->header);
            free(packet);
            return NULL;
        }

        // Copy the payload data
        memcpy(packet->data, data + sizeof(Header), payload_len);
    }
    else
    {
        // No payload
        packet->data = NULL;
    }

    return packet;
}

LoginRequest *decode_login_request(char *data) // login payload: {username: "vietanh", password: "viet1234"} is already extracted from the packet
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, 1024);

    mpack_expect_map_max(&reader, 2);

    mpack_expect_cstr_match(&reader, "user");
    const char *username = mpack_expect_cstr_alloc(&reader, 32);
    mpack_expect_cstr_match(&reader, "pass");
    const char *password = mpack_expect_cstr_alloc(&reader, 32);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "An error occurred decoding the message\n");
        return NULL;
    }

    LoginRequest *login_request = malloc(sizeof(LoginRequest));
    strcpy(login_request->username, username);
    strcpy(login_request->password, password);

    return login_request;
}

RawBytes *encode_response(int res)
{
    mpack_writer_t writer;
    char buffer[100];
    mpack_writer_init(&writer, buffer, 100);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "res");
    mpack_write_u16(&writer, res);
    mpack_finish_map(&writer);

    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Encode response: An error occurred encoding the message\n");
        return NULL;
    }

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));

    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);
    memcpy(raw_bytes->data, buffer, raw_bytes->len);

    return raw_bytes;
}

RawBytes *encode_response_msg(int res, char *msg)
{
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "res");
    mpack_write_u16(&writer, res);
    mpack_write_cstr(&writer, "msg");
    mpack_write_cstr(&writer, msg);
    mpack_finish_map(&writer);

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));

    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);
    memcpy(raw_bytes->data, buffer, raw_bytes->len);

    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Encode response msg: An error occurred encoding the message\n");
        return NULL;
    }

    return raw_bytes;
}

SignupRequest *decode_signup_request(char *payload)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, payload, 4096);

    mpack_expect_map_max(&reader, 9);

    mpack_expect_cstr_match(&reader, "user");
    const char *username = mpack_expect_cstr_alloc(&reader, 32);
    mpack_expect_cstr_match(&reader, "pass");
    const char *password = mpack_expect_cstr_alloc(&reader, 32);

    mpack_expect_cstr_match(&reader, "fullname");
    const char *fullname = mpack_expect_cstr_alloc(&reader, 64);

    mpack_expect_cstr_match(&reader, "phone");
    const char *phone = mpack_expect_cstr_alloc(&reader, 16);

    mpack_expect_cstr_match(&reader, "dob");
    const char *dob = mpack_expect_cstr_alloc(&reader, 16);

    mpack_expect_cstr_match(&reader, "email");
    const char *email = mpack_expect_cstr_alloc(&reader, 64);

    mpack_expect_cstr_match(&reader, "country");
    const char *country = mpack_expect_cstr_alloc(&reader, 32);

    mpack_expect_cstr_match(&reader, "gender");
    const char *gender = mpack_expect_cstr_alloc(&reader, 8);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "decode_signup_request: An error occurred decoding the message %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
        return NULL;
    }

    SignupRequest *signup_request = malloc(sizeof(SignupRequest));
    strncpy(signup_request->username, username, 32);
    strncpy(signup_request->password, password, 32);
    strncpy(signup_request->fullname, fullname, 64);
    strncpy(signup_request->email, email, 64);
    strncpy(signup_request->country, country, 32);
    strncpy(signup_request->phone, phone, 16);
    strncpy(signup_request->dob, dob, 16);
    strncpy(signup_request->gender, gender, 8);

    return signup_request;
}

CreateTableRequest *decode_create_table_request(char *payload)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, payload, 4096);

    mpack_expect_map_max(&reader, 3);

    mpack_expect_cstr_match(&reader, "tableName");
    const char *table_name = mpack_expect_cstr_alloc(&reader, 32);

    mpack_expect_cstr_match(&reader, "maxPlayer");
    int max_player = mpack_expect_int(&reader);

    mpack_expect_cstr_match(&reader, "minBet");
    int min_bet = mpack_expect_int(&reader);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "decode_create_table_request: An error occurred decoding the message %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
        return NULL;
    }

    CreateTableRequest *create_table_request = malloc(sizeof(CreateTableRequest));
    strncpy(create_table_request->table_name, table_name, 32);
    create_table_request->max_player = max_player;
    create_table_request->min_bet = min_bet;

    return create_table_request;
}

RawBytes *encode_scoreboard_response(dbScoreboard *leaderboard)
{
    mpack_writer_t writer;
    char buffer[MAXLINE];
    mpack_writer_init(&writer, buffer, MAXLINE);

    mpack_start_array(&writer, 20);

    for (int i = 0; i < 20; i++)
    {
        mpack_start_map(&writer, 3);
        mpack_write_cstr(&writer, "rank");
        mpack_write_i32(&writer, i + 1);
        mpack_write_cstr(&writer, "id");
        mpack_write_i32(&writer, leaderboard->players[i].user_id);
        mpack_write_cstr(&writer, "balance");
        mpack_write_i32(&writer, leaderboard->players[i].balance);
        mpack_finish_map(&writer);
    }

    mpack_finish_array(&writer);

    char *data = writer.buffer;
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(mpack_writer_destroy(&writer)));
        return NULL;
    }

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));
    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);

    memcpy(raw_bytes->data, data, raw_bytes->len);

    return raw_bytes;
}

RawBytes *encode_friendlist_response(FriendList *friendlist)
{
    mpack_writer_t writer;
    char buffer[MAXLINE];
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_array(&writer, friendlist->num);
    for (int i = 0; i < friendlist->num; i++)
    {
        mpack_start_map(&writer, 2);
        mpack_write_cstr(&writer, "id");
        mpack_write_i32(&writer, friendlist->friends[i].user_id);
        mpack_write_cstr(&writer, "username");
        mpack_write_cstr(&writer, friendlist->friends[i].user_name);
        mpack_finish_map(&writer);
    }

    mpack_finish_array(&writer);

    char *data = writer.buffer;
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(mpack_writer_destroy(&writer)));
        return NULL;
    }

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));
    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);

    memcpy(raw_bytes->data, data, raw_bytes->len);

    return raw_bytes;
}

RawBytes *encode_full_tables_response(TableList *table_list)
{
    mpack_writer_t writer;
    char buffer[MAXLINE];
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "size");
    mpack_write_i32(&writer, table_list->size);

    mpack_write_cstr(&writer, "tables");
    mpack_start_array(&writer, table_list->size);
    for (int i = 0; i < table_list->size; i++)
    {
        mpack_start_map(&writer, 5);
        mpack_write_cstr(&writer, "id");
        mpack_write_i32(&writer, table_list->tables[i].id);
        mpack_write_cstr(&writer, "tableName");
        mpack_write_cstr(&writer, table_list->tables[i].name);
        mpack_write_cstr(&writer, "maxPlayer");
        mpack_write_i32(&writer, table_list->tables[i].max_player);
        mpack_write_cstr(&writer, "minBet");
        mpack_write_i32(&writer, table_list->tables[i].min_bet);
        mpack_write_cstr(&writer, "currentPlayer");
        mpack_write_i32(&writer, table_list->tables[i].current_player);
        mpack_finish_map(&writer);
    }

    mpack_finish_array(&writer);
    mpack_finish_map(&writer);

    char *data = writer.buffer;
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(mpack_writer_destroy(&writer)));
        return NULL;
    }

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));
    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);

    memcpy(raw_bytes->data, data, raw_bytes->len);

    return raw_bytes;
}

int decode_join_table_request(char *payload)
{
    mpack_reader_t reader;
    mpack_reader_init(&reader, payload, 100, 100);

    mpack_expect_map_max(&reader, 1);

    mpack_expect_cstr_match(&reader, "tableId");
    int table_id = mpack_expect_i32(&reader);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "decode_join_table_request: An error occurred decoding the message %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
        return -1;
    }

    return table_id;
}

int decode_leave_table_request(char *payload) 
{
    mpack_reader_t reader;
    mpack_reader_init(&reader, payload, 100, 100);

    mpack_expect_map_max(&reader, 1);

    mpack_expect_cstr_match(&reader, "tableID");
    int table_id = mpack_expect_i32(&reader);

    if (mpack_reader_destroy(&reader) != mpack_ok) {
        fprintf(stderr, "decode_leave_table_response: An error occurred decoding the message %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
        return -1;
    }

    return table_id;
}

RawBytes *encode_login_success_response(dbUser *user)
{
    mpack_writer_t writer;
    char buffer[MAXLINE];
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 10);
    mpack_write_cstr(&writer, "res");
    mpack_write_u16(&writer, R_LOGIN_OK);
    mpack_write_cstr(&writer, "userId");
    mpack_write_i32(&writer, user->user_id);
    mpack_write_cstr(&writer, "username");
    mpack_write_cstr(&writer, user->username);
    mpack_write_cstr(&writer, "balance");
    mpack_write_i32(&writer, user->balance);
    mpack_write_cstr(&writer, "fullname");
    mpack_write_cstr(&writer, user->fullname);
    mpack_write_cstr(&writer, "email");
    mpack_write_cstr(&writer, user->email);
    mpack_write_cstr(&writer, "phone");
    mpack_write_cstr(&writer, user->phone);
    mpack_write_cstr(&writer, "dob");
    mpack_write_cstr(&writer, user->dob);
    mpack_write_cstr(&writer, "country");
    mpack_write_cstr(&writer, user->country);
    mpack_write_cstr(&writer, "gender");
    mpack_write_cstr(&writer, user->gender);
    mpack_finish_map(&writer);

    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Encode login response: An error occurred encoding the message\n");
        return NULL;
    }

    RawBytes *raw_bytes = malloc(sizeof(RawBytes));

    raw_bytes->len = mpack_writer_buffer_used(&writer);
    raw_bytes->data = malloc(raw_bytes->len);
    memcpy(raw_bytes->data, buffer, raw_bytes->len);

    return raw_bytes;
}