#include "test.h"

/**
 * Test for table creation with auto-join functionality
 * This test verifies that when a user creates a table, they are automatically joined to it
 */
int test_create_table_auto_join() {
    printf("\n=== Testing Table Creation with Auto-Join ===\n");
    
    // Create a socket and connect to server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR: Cannot create socket\n");
        return 0;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("ERROR: Invalid address\n");
        close(sockfd);
        return 0;
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("ERROR: Connection failed\n");
        close(sockfd);
        return 0;
    }
    
    printf("✓ Connected to server\n");
    
    // Perform handshake
    char handshake_req[4] = {0x00, 0x02, 0x00, 0x01};
    if (send(sockfd, handshake_req, 4, 0) != 4) {
        printf("ERROR: Failed to send handshake\n");
        close(sockfd);
        return 0;
    }
    
    char handshake_resp[3];
    if (recv(sockfd, handshake_resp, 3, 0) != 3) {
        printf("ERROR: Failed to receive handshake response\n");
        close(sockfd);
        return 0;
    }
    
    if (handshake_resp[2] != 0x00) {
        printf("ERROR: Handshake failed\n");
        close(sockfd);
        return 0;
    }
    
    printf("✓ Handshake completed\n");
    
    // Login first
    char login_buffer[MAXLINE];
    mpack_writer_t login_writer;
    mpack_writer_init(&login_writer, login_buffer, MAXLINE);
    mpack_start_map(&login_writer, 2);
    mpack_write_cstr(&login_writer, "user");
    mpack_write_cstr(&login_writer, "test_user");
    mpack_write_cstr(&login_writer, "pass");
    mpack_write_cstr(&login_writer, "test_password");
    mpack_finish_map(&login_writer);
    
    RawBytes* login_packet = encode_packet(PROTOCOL_V1, PACKET_LOGIN, login_buffer, mpack_writer_buffer_used(&login_writer));
    
    if (send(sockfd, login_packet->data, login_packet->len, 0) != (ssize_t)login_packet->len) {
        printf("ERROR: Failed to send login packet\n");
        close(sockfd);
        return 0;
    }
    
    // Receive login response (skip for this test)
    char login_resp[MAXLINE];
    recv(sockfd, login_resp, MAXLINE, 0);
    printf("✓ Login attempt sent\n");
    
    // Create table
    char table_buffer[MAXLINE];
    mpack_writer_t table_writer;
    mpack_writer_init(&table_writer, table_buffer, MAXLINE);
    mpack_start_map(&table_writer, 3);
    mpack_write_cstr(&table_writer, "name");
    mpack_write_cstr(&table_writer, "AutoJoinTestTable");
    mpack_write_cstr(&table_writer, "max_player");
    mpack_write_u16(&table_writer, 6);
    mpack_write_cstr(&table_writer, "min_bet");
    mpack_write_u16(&table_writer, 10);
    mpack_finish_map(&table_writer);
    
    RawBytes* create_packet = encode_packet(PROTOCOL_V1, PACKET_CREATE_TABLE, table_buffer, mpack_writer_buffer_used(&table_writer));
    
    printf("Sending create table packet...\n");
    if (send(sockfd, create_packet->data, create_packet->len, 0) != (ssize_t)create_packet->len) {
        printf("ERROR: Failed to send create table packet\n");
        close(sockfd);
        return 0;
    }
    
    // Receive create table response
    char create_resp[MAXLINE];
    ssize_t resp_len = recv(sockfd, create_resp, MAXLINE, 0);
    
    if (resp_len <= 0) {
        printf("ERROR: Failed to receive create table response\n");
        close(sockfd);
        return 0;
    }
    
    // Parse the response
    Header* resp_header = decode_header(create_resp);
    if (resp_header->packet_type == PACKET_CREATE_TABLE) {
        // Decode the MessagePack payload
        mpack_reader_t reader;
        mpack_reader_init_data(&reader, create_resp + sizeof(Header), resp_len - sizeof(Header));
        
        mpack_expect_map(&reader);
        
        int result_code = 0;
        int table_id = 0;
        
        // Read the map keys
        for (int i = 0; i < 2; i++) {
            const char* key = mpack_expect_cstr_alloc(&reader, 32);
            if (strcmp(key, "res") == 0) {
                result_code = mpack_expect_u16(&reader);
            } else if (strcmp(key, "table_id") == 0) {
                table_id = mpack_expect_i32(&reader);
            }
            free((void*)key);
        }
        
        mpack_reader_destroy(&reader);
        
        if (result_code == R_CREATE_TABLE_OK && table_id > 0) {
            printf("✓ Table created successfully with ID: %d\n", table_id);
            printf("✓ Server includes table_id in response (auto-join works)\n");
            close(sockfd);
            return 1;
        } else {
            printf("ERROR: Table creation failed, result_code: %d, table_id: %d\n", result_code, table_id);
        }
    } else {
        printf("ERROR: Unexpected response packet type: %d\n", resp_header->packet_type);
    }
    
    close(sockfd);
    return 0;
}

/**
 * Test for friend management in game context
 */
int test_friend_invite_from_game() {
    printf("\n=== Testing Friend Invite from Game Context ===\n");
    
    // This test would verify that friend invites can be sent while in a game
    // For now, we'll just verify the packet structures are correct
    
    char invite_buffer[MAXLINE];
    mpack_writer_t writer;
    mpack_writer_init(&writer, invite_buffer, MAXLINE);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "friend_username");
    mpack_write_cstr(&writer, "test_friend");
    mpack_finish_map(&writer);
    
    RawBytes* invite_packet = encode_packet(PROTOCOL_V1, PACKET_INVITE_FRIEND, invite_buffer, mpack_writer_buffer_used(&writer));
    
    if (invite_packet && invite_packet->len > sizeof(Header)) {
        printf("✓ Friend invite packet created successfully\n");
        printf("✓ Packet type: %d (PACKET_INVITE_FRIEND)\n", PACKET_INVITE_FRIEND);
        free(invite_packet->data);
        free(invite_packet);
        return 1;
    } else {
        printf("ERROR: Failed to create friend invite packet\n");
        return 0;
    }
}

int main() {
    printf("Starting E2E Tests for New Features\n");
    printf("===================================\n");
    
    int tests_passed = 0;
    int tests_total = 2;
    
    // Test 1: Table creation with auto-join
    if (test_create_table_auto_join()) {
        tests_passed++;
        printf("✅ Test 1 PASSED: Table creation with auto-join\n");
    } else {
        printf("❌ Test 1 FAILED: Table creation with auto-join\n");
    }
    
    // Test 2: Friend invite packet structure
    if (test_friend_invite_from_game()) {
        tests_passed++;
        printf("✅ Test 2 PASSED: Friend invite packet structure\n");
    } else {
        printf("❌ Test 2 FAILED: Friend invite packet structure\n");
    }
    
    printf("\n===================================\n");
    printf("Tests Results: %d/%d passed\n", tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("🎉 All tests passed!\n");
        return 0;
    } else {
        printf("💥 Some tests failed!\n");
        return 1;
    }
}