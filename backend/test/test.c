#include "test.h"

TEST(test_decode_msgpack)
{
    char buffer[MAXLINE];

    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, "admin");
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "admin");
    mpack_finish_map(&writer);

    uint16_t len = mpack_writer_buffer_used(&writer);
    memcpy(buffer, writer.buffer, len);
    mpack_writer_destroy(&writer);

    Map *m = decode_msgpack(buffer, len);
    MapEntry *entry = map_search(m, "user");
    ASSERT(entry != NULL);
    ASSERT_STR_EQ((char *)entry->value, "admin");
    entry = map_search(m, "pass");
    ASSERT(entry != NULL);
    ASSERT_STR_EQ((char *)entry->value, "admin");
}

int main()
{
    RUN_TEST(test_decode_msgpack);
}