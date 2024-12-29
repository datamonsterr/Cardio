#include "test.h"

TEST(test_decode_header)
{
    Header *header = decode_header("\x00\x0A\x01\x00\x64");
    ASSERT(header->packet_len == 10);
    ASSERT(header->protocol_ver == 1);
    ASSERT(header->packet_type == 100);
}

TEST(test_decode_login_request)
{
    mpack_writer_t writer;
    char buf[1024];
    size_t size;
    mpack_writer_init(&writer, buf, sizeof(buf));

    mpack_build_map(&writer);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, "admin");
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "admin");
    mpack_complete_map(&writer);
    mpack_writer_destroy(&writer);

    LoginRequest *login_request = decode_login_request(buf);

    ASSERT_STR_EQ(login_request->username, "admin");
    ASSERT_STR_EQ(login_request->password, "admin");

    free(login_request);
}

TEST(test_database_login)
{
    PGconn *conn = db_connect("host=localhost dbname=postgres user=postgres")
}

int main()
{
    RUN_TEST(test_decode_header);
    RUN_TEST(test_decode_login_request);
    return failed;
}