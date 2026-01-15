/*
 * End-to-end test for table invite functionality
 * 
 * Tests:
 * 1. User1 creates a table
 * 2. User1 joins the table
 * 3. User1 invites User2 (a friend) to join the table
 * 4. Verify invitation succeeds
 * 5. User1 tries to invite User3 (not a friend) - should fail
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT "8080"

// Helper function to connect to server
int connect_to_server()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(SERVER_HOST, SERVER_PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

// Helper to send packet
int send_packet(int sockfd, RawBytes* packet)
{
    int total = 0;
    int bytesleft = packet->len;
    int n;

    while (total < packet->len)
    {
        n = send(sockfd, packet->data + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    return n == -1 ? -1 : 0;
}

// Helper to receive packet
char* recv_packet(int sockfd, size_t* len)
{
    char header_buf[5];
    int n = recv(sockfd, header_buf, 5, MSG_WAITALL);
    if (n != 5)
    {
        return NULL;
    }

    Header* header = decode_header(header_buf);
    *len = header->packet_len;
    
    char* full_packet = malloc(*len);
    memcpy(full_packet, header_buf, 5);
    
    n = recv(sockfd, full_packet + 5, *len - 5, MSG_WAITALL);
    if (n != (*len - 5))
    {
        free(full_packet);
        free(header);
        return NULL;
    }

    free(header);
    return full_packet;
}

// Helper to login
int do_login(int sockfd, const char* username, const char* password)
{
    printf("Logging in as %s...\n", username);
    
    // Create login request
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, password);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_LOGIN, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    // Check if login was successful
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    
    mpack_expect_map_max(&reader, 10);
    mpack_expect_cstr_match(&reader, "result");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    if (result == 0)
    {
        printf("✓ Login successful as %s\n", username);
        return 0;
    }
    else
    {
        printf("✗ Login failed for %s (result: %d)\n", username, result);
        return -1;
    }
}

// Helper to create table
int create_table(int sockfd, const char* table_name, int* table_id_out)
{
    printf("Creating table '%s'...\n", table_name);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, table_name);
    mpack_write_cstr(&writer, "max_player");
    mpack_write_int(&writer, 5);
    mpack_write_cstr(&writer, "min_bet");
    mpack_write_int(&writer, 10);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_CREATE_TABLE, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        free(packet);
        return -1;
    }
    free(packet);

    // The server sends two packets:
    // 1. PACKET_BALANCE_UPDATE (970) - from join_table
    // 2. PACKET_CREATE_TABLE response (300)
    // We need to read both
    
    // First, read balance update notification
    size_t balance_len;
    char* balance_response = recv_packet(sockfd, &balance_len);
    if (balance_response)
    {
        Packet* balance_packet = decode_packet(balance_response, balance_len);
        printf("DEBUG: Received balance update packet type=%d\n", balance_packet->header->packet_type);
        free_packet(balance_packet);
        free(balance_response);
    }

    // Now read the actual create table response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    printf("DEBUG [create_table]: Received packet type=%d, len=%zu\n", 
           resp_packet->header->packet_type, resp_len);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    
    int map_size = mpack_expect_map(&reader);
    int result = 0;
    int table_id = 0;
    
    printf("DEBUG: Map size = %d, mpack error = %d\n", map_size, mpack_reader_error(&reader));
    
    for (int i = 0; i < map_size; i++)
    {
        char key[32];
        mpack_expect_cstr(&reader, key, sizeof(key));
        
        printf("DEBUG: Key[%d] = '%s', mpack error after key read = %d\n", i, key, mpack_reader_error(&reader));
        
        if (strcmp(key, "res") == 0)
        {
            result = mpack_expect_u16(&reader);
            printf("DEBUG: result = %d, mpack error = %d\n", result, mpack_reader_error(&reader));
        }
        else if (strcmp(key, "table_id") == 0)
        {
            table_id = mpack_expect_int(&reader);
            printf("DEBUG: table_id = %d, mpack error = %d\n", table_id, mpack_reader_error(&reader));
        }
        else
        {
            mpack_discard(&reader);
            printf("DEBUG: Discarded unknown key, mpack error = %d\n", mpack_reader_error(&reader));
        }
    }
    
    mpack_done_map(&reader);
    
    if (result == R_CREATE_TABLE_OK)
    {
        *table_id_out = table_id;
        printf("✓ Table created with ID %d\n", *table_id_out);
        free_packet(resp_packet);
        mpack_reader_destroy(&reader);
        return 0;
    }
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);
    printf("✗ Failed to create table (result: %d)\n", result);
    return -1;
}

// Helper to join table
int test_join_table(int sockfd, int table_id)
{
    printf("Joining table %d...\n", table_id);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "tableId");
    mpack_write_int(&writer, table_id);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_JOIN_TABLE, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);
    free_packet(resp_packet);

    printf("✓ Joined table\n");
    return 0;
}

// Helper to send table invite
int send_table_invite(int sockfd, const char* friend_username, int table_id)
{
    printf("Inviting '%s' to table %d...\n", friend_username, table_id);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "friend_username");
    mpack_write_cstr(&writer, friend_username);
    mpack_write_cstr(&writer, "table_id");
    mpack_write_int(&writer, table_id);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_INVITE_TO_TABLE, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    mpack_expect_map_max(&reader, 3);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    if (result == R_INVITE_TO_TABLE_OK)
    {
        printf("✓ Table invite sent to %s\n", friend_username);
        return 0;
    }
    else if (result == R_INVITE_TO_TABLE_NOT_FRIENDS)
    {
        printf("✗ Not friends with %s (expected for non-friend test)\n", friend_username);
        return result;
    }
    else
    {
        printf("✗ Failed to send table invite (code: %d)\n", result);
        return result;
    }
}

int main()
{
    printf("=== Table Invite E2E Test ===\n\n");
    
    int sock1;
    int table_id;
    int passed = 0;
    int failed = 0;

    // Test 1: User1 logs in, creates table, and joins
    printf("\n--- Test 1: User1 creates and joins table ---\n");
    sock1 = connect_to_server();
    if (sock1 < 0 || do_login(sock1, "user1", "password12345") < 0)
    {
        printf("✗ Failed to connect/login as user1\n");
        failed++;
        return 1;
    }

    if (create_table(sock1, "Test Table", &table_id) < 0)
    {
        printf("✗ Failed to create table\n");
        failed++;
        close(sock1);
        return 1;
    }

    if (test_join_table(sock1, table_id) < 0)
    {
        printf("✗ Failed to join table\n");
        failed++;
        close(sock1);
        return 1;
    }
    passed++;

    // Test 2: User1 invites User2 (friend) to table
    printf("\n--- Test 2: User1 invites User2 (friend) to table ---\n");
    int result = send_table_invite(sock1, "user2", table_id);
    if (result == 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    // Test 3: User1 invites User6 (not a friend) to table - should fail
    printf("\n--- Test 3: User1 invites User6 (non-friend) - should fail ---\n");
    result = send_table_invite(sock1, "user6", table_id);
    if (result == R_INVITE_TO_TABLE_NOT_FRIENDS)
    {
        passed++;
        printf("✓ Correctly rejected non-friend invite\n");
    }
    else
    {
        failed++;
    }

    // Test 4: User1 invites to non-existent table - should fail
    printf("\n--- Test 4: User1 invites to non-existent table ---\n");
    result = send_table_invite(sock1, "user2", 99999);
    if (result != 0)
    {
        passed++;
        printf("✓ Correctly rejected invalid table\n");
    }
    else
    {
        failed++;
    }

    close(sock1);

    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n\n", passed + failed);

    return (failed > 0) ? 1 : 0;
}
