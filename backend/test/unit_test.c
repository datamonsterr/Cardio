#include "test.h"

TEST(test_decode_packet)
{
    char *data = "hello";
    RawBytes *encoded = encode_packet(1, 100, data, strlen(data));
    RawBytes *encoded_null = encode_packet(1, 100, NULL, 0);

    Packet *packet = decode_packet(encoded->data, encoded->len);
    Packet *packet_null = decode_packet(encoded_null->data, encoded_null->len);

    ASSERT(packet->header->packet_len == 10);
    ASSERT(packet->header->protocol_ver == 1);
    ASSERT(packet->header->packet_type == 100);
    ASSERT(compare_raw_bytes(packet->data, "hello", 5) == 1);

    ASSERT(packet_null->header->packet_len == 5);
    ASSERT(packet_null->header->packet_type == 100);
    free_packet(packet);
}

TEST(test_encode_packet)
{
    char *data = "hello";
    RawBytes *encoded = encode_packet(1, 0, data, strlen(data));
    Header *header = decode_header(encoded->data);
    ASSERT(encoded->len == 10);
    ASSERT(header->packet_len == 10);
    ASSERT(compare_raw_bytes(encoded->data + 5, "hello", encoded->len - 5) == 1);
    free(encoded);
}

TEST(test_decode_login_request)
{
    char *username = "username";
    char *password = "viet1234";
    mpack_writer_t writer;
    char buffer[4096];
    mpack_writer_init(&writer, buffer, 4096);

    // Write data to the MPack map
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, password);
    mpack_finish_map(&writer);

    // Check for MPack errors

    // Retrieve MPack buffer
    char *data = writer.buffer;
    size_t size = mpack_writer_buffer_used(&writer);

    LoginRequest *login_request = decode_login_request(data);

    ASSERT(strcmp(login_request->username, "username") == 0);
    ASSERT(strcmp(login_request->password, "viet1234") == 0);

    mpack_writer_destroy(&writer);

    free(login_request);
}

TEST(test_encode_response)
{

    RawBytes *success = encode_response(R_LOGIN_OK);

    mpack_reader_t reader;
    mpack_reader_init(&reader, success->data, 1024, 1024);
    mpack_expect_map_max(&reader, 1);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_u16(&reader);

    ASSERT(res == R_LOGIN_OK);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "Error: %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
    }

    free(success);
}

TEST(test_encode_response_message)
{
    char *msg = "Login success";
    RawBytes *success = encode_response_msg(R_LOGIN_OK, msg);

    mpack_reader_t reader;
    mpack_reader_init(&reader, success->data, 1024, 1024);
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_u16(&reader);
    mpack_expect_cstr_match(&reader, "msg");
    const char *message = mpack_expect_cstr_alloc(&reader, 30);

    ASSERT(res == R_LOGIN_OK);
    ASSERT(strcmp(message, msg) == 0);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "Error: %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
    }

    free(success);
}

TEST(test_decode_signup_request)
{
    mpack_writer_t writer;
    char buffer[65536];
    mpack_writer_init(&writer, buffer, 65536);
    mpack_start_map(&writer, 8);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, "Tester2");
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "abc1234");
    mpack_write_cstr(&writer, "fullname");
    mpack_write_cstr(&writer, "Pham Thanh Dat");
    mpack_write_cstr(&writer, "phone");
    mpack_write_cstr(&writer, "1234567890");
    mpack_write_cstr(&writer, "dob");
    mpack_write_cstr(&writer, "2004/01/01");
    mpack_write_cstr(&writer, "email");
    mpack_write_cstr(&writer, "abcde@gmail.com");
    mpack_write_cstr(&writer, "country");
    mpack_write_cstr(&writer, "Vietnam");
    mpack_write_cstr(&writer, "gender");
    mpack_write_cstr(&writer, "Male");
    mpack_finish_map(&writer);

    char *data = writer.buffer;
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(mpack_writer_destroy(&writer)));
    }
    RawBytes *encoded = encode_packet(1, 200, data, mpack_writer_buffer_used(&writer));
    Packet *decoded = decode_packet(encoded->data, encoded->len);

    ASSERT(decoded->header->packet_type == 200);
    ASSERT(decoded->header->packet_len == encoded->len);

    SignupRequest *signup_request = decode_signup_request(data);

    ASSERT(strcmp(signup_request->username, "Tester2") == 0);
    ASSERT(strcmp(signup_request->password, "abc1234") == 0);
    ASSERT(strcmp(signup_request->fullname, "Pham Thanh Dat") == 0);
    ASSERT(strcmp(signup_request->phone, "1234567890") == 0);
    ASSERT(strcmp(signup_request->dob, "2004/01/01") == 0);
    ASSERT(strcmp(signup_request->email, "abcde@gmail.com") == 0);
    ASSERT(strcmp(signup_request->country, "Vietnam") == 0);
    ASSERT(strcmp(signup_request->gender, "Male") == 0);
}

TEST(test_db_conn)
{
    PGconn *conn = PQconnectdb(dbconninfo);
    ASSERT(PQstatus(conn) == CONNECTION_OK);
    PQfinish(conn);
}

TEST(test_decode_create_table_request)
{
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);

    mpack_start_map(&writer, 3);

    mpack_write_cstr(&writer, "tableName");
    mpack_write_cstr(&writer, "Table 1");
    mpack_write_cstr(&writer, "maxPlayer");
    mpack_write_int(&writer, 5);
    mpack_write_cstr(&writer, "minBet");
    mpack_write_int(&writer, 100);

    mpack_finish_map(&writer);

    char *data = writer.buffer;
    CreateTableRequest *table_request = decode_create_table_request(data);

    ASSERT(strcmp(table_request->table_name, "Table 1") == 0);
    ASSERT(table_request->max_player == 5);
    ASSERT(table_request->min_bet == 100);
}

TEST(test_logger)
{
    logger("test.log", "Info", "Test logger");
    FILE *f = fopen("test.log", "r");
    if (f == NULL)
    {
        fprintf(stderr, "Cannot open file\n");
        return;
    }
    char line[1024];
    fgets(line, 1024, f);
    printf("Log: %s\n", line);
    fclose(f);
}

TEST(test_encode_full_tables_resp)
{
    TableList *table_list = init_table_list(3);
    add_table(table_list, "Table 1", 5, 100);
    add_table(table_list, "Table 2", 3, 130);

    RawBytes *encoded = encode_full_tables_response(table_list);

    mpack_reader_t reader;
    mpack_reader_init(&reader, encoded->data, 1024, 1024);
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "size");
    int size = mpack_expect_i32(&reader);
    ASSERT(size == table_list->size);

    mpack_expect_cstr_match(&reader, "tables");
    mpack_expect_array_max(&reader, size);
    for (int i = 0; i < size; i++)
    {
        mpack_expect_map_max(&reader, 5);

        mpack_expect_cstr_match(&reader, "id");
        int id = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "tableName");
        const char *name = mpack_expect_cstr_alloc(&reader, 32);
        mpack_expect_cstr_match(&reader, "maxPlayer");
        int max_player = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "minBet");
        int min_bet = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "currentPlayer");
        int current_player = mpack_expect_i32(&reader);

        ASSERT(strcmp(name, table_list->tables[i].name) == 0);
    }
}

TEST(test_decode_join_table_req)
{
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "tableId");
    mpack_write_i32(&writer, 1);
    mpack_finish_map(&writer);

    char *data = writer.buffer;
    RawBytes *encoded = encode_packet(1, PACKET_JOIN_TABLE, data, mpack_writer_buffer_used(&writer));
    Packet *decoded = decode_packet(encoded->data, encoded->len);

    ASSERT(decoded->header->packet_type == PACKET_JOIN_TABLE);
    ASSERT(decoded->header->packet_len == encoded->len);

    mpack_reader_t reader;
}

TEST(test_encode_login_success_resp)
{
    dbUser *user = malloc(sizeof(dbUser));

    user->user_id = 1;
    user->balance = 1000;
    strcpy(user->username, "Tester");
    strcpy(user->fullname, "Pham Thanh Dat");
    strcpy(user->email, "test@gmail.com");
    strcpy(user->phone, "1234567890");
    strcpy(user->dob, "2004/01/01");
    strcpy(user->country, "Vietnam");
    strcpy(user->gender, "Male");
    RawBytes *rawbytes = encode_login_success_response(user);

    mpack_reader_t reader;
    mpack_reader_init(&reader, rawbytes->data, 1024, 1024);
    mpack_expect_map_max(&reader, 10);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_i32(&reader);
    mpack_expect_cstr_match(&reader, "userId");
    int user_id = mpack_expect_i32(&reader);
    mpack_expect_cstr_match(&reader, "username");
    const char *username = mpack_expect_cstr_alloc(&reader, 32);
    mpack_expect_cstr_match(&reader, "balance");
    int balance = mpack_expect_i32(&reader);
    mpack_expect_cstr_match(&reader, "fullname");
    const char *fullname = mpack_expect_cstr_alloc(&reader, 64);
    mpack_expect_cstr_match(&reader, "email");
    const char *email = mpack_expect_cstr_alloc(&reader, 64);
    mpack_expect_cstr_match(&reader, "phone");
    const char *phone = mpack_expect_cstr_alloc(&reader, 16);
    mpack_expect_cstr_match(&reader, "dob");
    const char *dob = mpack_expect_cstr_alloc(&reader, 16);
    mpack_expect_cstr_match(&reader, "country");
    const char *country = mpack_expect_cstr_alloc(&reader, 32);
    mpack_expect_cstr_match(&reader, "gender");
    const char *gender = mpack_expect_cstr_alloc(&reader, 8);

    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "Error: %s\n", mpack_error_to_string(mpack_reader_destroy(&reader)));
    }

    ASSERT(res == R_LOGIN_OK);
    ASSERT(user_id == 1);
    ASSERT(balance == 1000);
    ASSERT(strcmp(username, "Tester") == 0);
    ASSERT(strcmp(fullname, "Pham Thanh Dat") == 0);
}

int main()
{
    RUN_TEST(test_db_conn);
    RUN_TEST(test_decode_packet);
    RUN_TEST(test_encode_packet);
    RUN_TEST(test_decode_login_request);
    RUN_TEST(test_encode_response);
    RUN_TEST(test_encode_response_message);
    RUN_TEST(test_decode_signup_request);
    RUN_TEST(test_decode_create_table_request);
    RUN_TEST(test_logger);
    RUN_TEST(test_encode_full_tables_resp);
    RUN_TEST(test_decode_join_table_req);
    RUN_TEST(test_encode_login_success_resp);
}