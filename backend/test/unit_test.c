#include "test.h"

TEST(test_decode_packet)
{
    char *data = "\x00\x0A\x01\x00\x00hello";

    Packet *packet = decode_packet(data);

    ASSERT(packet->header->packet_len == 10);
    ASSERT(packet->header->protocol_ver == 1);
    ASSERT(packet->header->packet_type == 0);
    ASSERT(compare_raw_bytes(packet->data, "hello", 5) == 1);
}

TEST(test_encode_packet)
{
    char *data = "hello";
    RawBytes *encoded = encode_packet(1, 0, data, strlen(data));
    ASSERT(encoded->len == 10);
    ASSERT(compare_raw_bytes(encoded->data, "\x00\x0A\x01\x00\x00hello", 10) == 1);
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
    const char *data = writer.buffer;
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

int main()
{
    RUN_TEST(test_decode_packet);
    RUN_TEST(test_encode_packet);
    RUN_TEST(test_decode_login_request);
    RUN_TEST(test_encode_response);
    RUN_TEST(test_encode_response_message);
}