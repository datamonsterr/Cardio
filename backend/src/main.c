#include <stdio.h>
#include "mpack.h"
#include <string.h>

void write_messagepack(char *buffer, size_t *size)
{
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, 1024);

    // Write data
    mpack_start_map(&writer, 2); // Start a map with 2 key-value pairs

    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "John Doe");

    mpack_write_cstr(&writer, "age");
    mpack_write_u32(&writer, 30);

    mpack_finish_map(&writer); // Finish the map

    // Get the size of the written data
    *size = mpack_writer_buffer_used(&writer);

    // Check for errors
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Error encoding data to MessagePack format\n");
        return;
    }

    printf("Serialized %zu bytes!\n", *size);
}

void read_messagepack(const char *buffer, size_t size)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, buffer, size);

    // Parse data
    mpack_expect_map(&reader);

    for (size_t i = 0; i < 2; i++)
    {                                                           // Loop through 2 key-value pairs
        const char *key = mpack_expect_cstr_alloc(&reader, 32); // Max key length: 32
        if (strcmp(key, "name") == 0)
        {
            const char *name = mpack_expect_cstr_alloc(&reader, 64); // Max value length: 64
            printf("Name: %s\n", name);
            free((void *)name);
        }
        else if (strcmp(key, "age") == 0)
        {
            uint32_t age = mpack_expect_u32(&reader);
            printf("Age: %u\n", age);
        }
        free((void *)key);
    }

    mpack_done_map(&reader);

    // Check for errors
    if (mpack_reader_destroy(&reader) != mpack_ok)
    {
        fprintf(stderr, "Error decoding MessagePack data\n");
    }
}

int main(void)
{
    char buffer[1024];
    size_t size = 0;

    write_messagepack(buffer, &size);
    read_messagepack(buffer, size);

    return 0;
}