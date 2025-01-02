#include "test.h"

TEST(test_decode_packet)
{
    char *data = "hello";
    RawBytes *encoded = encode_packet(1, 100, data, strlen(data));

    Packet *packet = decode_packet(encoded->data, encoded->len);

    ASSERT(packet->header->packet_len == 10);
    ASSERT(packet->header->protocol_ver == 1);
    ASSERT(packet->header->packet_type == 100);
    ASSERT(compare_raw_bytes(packet->data, "hello", 5) == 1);
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

    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "Table 1");
    mpack_write_cstr(&writer, "max_player");
    mpack_write_int(&writer, 5);
    mpack_write_cstr(&writer, "min_bet");
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
}