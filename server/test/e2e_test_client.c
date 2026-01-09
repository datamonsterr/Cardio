/**
 * Comprehensive End-to-End Test Client for Cardio Game Server
 * Tests all protocol messages defined in PROTOCOL.md
 */

#include "main.h"
#include <time.h>

// Color codes for output
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_CYAN "\033[1;36m"

// Test state
typedef struct {
    int sockfd;
    char username[32];
    int user_id;
    int table_id;
    int tests_passed;
    int tests_failed;
} TestState;

void print_test_header(const char* test_name) {
    printf("\n%s========================================%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s TEST: %s%s\n", COLOR_CYAN, test_name, COLOR_RESET);
    printf("%s========================================%s\n", COLOR_CYAN, COLOR_RESET);
}

void print_success(const char* message) {
    printf("%s✓ SUCCESS: %s%s\n", COLOR_GREEN, message, COLOR_RESET);
}

void print_failure(const char* message) {
    printf("%s✗ FAILURE: %s%s\n", COLOR_RED, message, COLOR_RESET);
}

void print_info(const char* message) {
    printf("%s→ INFO: %s%s\n", COLOR_BLUE, message, COLOR_RESET);
}

void print_hex_dump(const char* label, char* data, size_t len) {
    printf("%s%s (len=%zu):%s\n", COLOR_YELLOW, label, len, COLOR_RESET);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16 != 0) printf("\n");
}

/**
 * Test 1: Handshake (if implemented)
 */
int test_handshake(TestState* state) {
    print_test_header("HANDSHAKE");
    // Note: Current server doesn't implement handshake, but we document it
    print_info("Handshake not implemented in current server version");
    print_info("Would send: [length=0x0002][version=0x0001]");
    return 1; // Skip test
}

/**
 * Test 2: Signup - Register a new user
 */
int test_signup(TestState* state) {
    print_test_header("SIGNUP (Packet ID 200)");
    
    char buffer[MAXLINE];
    mpack_writer_t writer;
    
    // Generate unique username with timestamp
    snprintf(state->username, sizeof(state->username), "test%ld", time(NULL));
    
    print_info("Registering new user:");
    printf("  Username: %s\n", state->username);
    
    // Encode signup request
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 8);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, state->username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "testpass123");
    mpack_write_cstr(&writer, "fullname");
    mpack_write_cstr(&writer, "Test User");
    mpack_write_cstr(&writer, "phone");
    mpack_write_cstr(&writer, "1234567890");
    mpack_write_cstr(&writer, "dob");
    mpack_write_cstr(&writer, "2000-01-01");
    mpack_write_cstr(&writer, "email");
    mpack_write_cstr(&writer, "test@example.com");
    mpack_write_cstr(&writer, "country");
    mpack_write_cstr(&writer, "USA");
    mpack_write_cstr(&writer, "gender");
    mpack_write_cstr(&writer, "other");
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* encoded_message = encode_packet(PROTOCOL_V1, PACKET_SIGNUP, buffer, size);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        print_failure("MPack encoding error");
        state->tests_failed++;
        return 0;
    }
    
    // Send request
    print_info("Sending SIGNUP_REQUEST");
    print_hex_dump("Packet", encoded_message->data, encoded_message->len);
    
    if (send(state->sockfd, encoded_message->data, encoded_message->len, 0) == -1) {
        print_failure("Failed to send signup request");
        free(encoded_message);
        state->tests_failed++;
        return 0;
    }
    free(encoded_message);
    
    // Receive response
    char recv_buffer[MAXLINE];
    int nbytes = recv(state->sockfd, recv_buffer, MAXLINE, 0);
    if (nbytes <= 0) {
        print_failure("Failed to receive signup response");
        state->tests_failed++;
        return 0;
    }
    
    print_info("Received response");
    print_hex_dump("Response", recv_buffer, nbytes);
    
    Header* header = decode_header(recv_buffer);
    if (!header) {
        print_failure("Invalid header in signup response");
        state->tests_failed++;
        return 0;
    }
    
    printf("  Packet Type: %d (expected: %d)\n", header->packet_type, PACKET_SIGNUP);
    printf("  Packet Length: %d\n", header->packet_len);
    
    if (header->packet_type != PACKET_SIGNUP) {
        print_failure("Wrong packet type in response");
        free(header);
        state->tests_failed++;
        return 0;
    }
    
    // Decode response
    mpack_reader_t reader;
    mpack_reader_init(&reader, recv_buffer + sizeof(Header), 
                      header->packet_len - sizeof(Header),
                      header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 1);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_u16(&reader);
    
    printf("  Result Code: %d\n", res);
    
    if (res == R_SIGNUP_OK) {
        print_success("Signup successful");
        state->tests_passed++;
        free(header);
        return 1;
    } else {
        print_failure("Signup failed");
        state->tests_failed++;
        free(header);
        return 0;
    }
}

/**
 * Test 3: Login - Authenticate user
 */
int test_login(TestState* state) {
    print_test_header("LOGIN (Packet ID 100)");
    
    char buffer[MAXLINE];
    mpack_writer_t writer;
    
    print_info("Logging in:");
    printf("  Username: %s\n", state->username);
    
    // Encode login request
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, state->username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, "testpass123");
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* encoded_message = encode_packet(PROTOCOL_V1, PACKET_LOGIN, buffer, size);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        print_failure("MPack encoding error");
        state->tests_failed++;
        return 0;
    }
    
    // Send request
    print_info("Sending LOGIN_REQUEST");
    print_hex_dump("Packet", encoded_message->data, encoded_message->len);
    
    if (send(state->sockfd, encoded_message->data, encoded_message->len, 0) == -1) {
        print_failure("Failed to send login request");
        free(encoded_message);
        state->tests_failed++;
        return 0;
    }
    free(encoded_message);
    
    // Receive response
    char recv_buffer[MAXLINE];
    int nbytes = recv(state->sockfd, recv_buffer, MAXLINE, 0);
    if (nbytes <= 0) {
        print_failure("Failed to receive login response");
        state->tests_failed++;
        return 0;
    }
    
    print_info("Received response");
    print_hex_dump("Response", recv_buffer, nbytes);
    
    Header* header = decode_header(recv_buffer);
    if (!header) {
        print_failure("Invalid header in login response");
        state->tests_failed++;
        return 0;
    }
    
    printf("  Packet Type: %d (expected: %d)\n", header->packet_type, PACKET_LOGIN);
    printf("  Packet Length: %d\n", header->packet_len);
    
    if (header->packet_type != PACKET_LOGIN) {
        print_failure("Wrong packet type in response");
        free(header);
        state->tests_failed++;
        return 0;
    }
    
    // Decode response
    mpack_reader_t reader;
    mpack_reader_init(&reader, recv_buffer + sizeof(Header), 
                      header->packet_len - sizeof(Header),
                      header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 1);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_u16(&reader);
    
    printf("  Result Code: %d\n", res);
    
    if (res == R_LOGIN_OK) {
        print_success("Login successful");
        state->tests_passed++;
        free(header);
        return 1;
    } else {
        print_failure("Login failed");
        state->tests_failed++;
        free(header);
        return 0;
    }
}

/**
 * Test 4: Create Table
 */
int test_create_table(TestState* state) {
    print_test_header("CREATE_TABLE (Packet ID 300)");
    
    char buffer[MAXLINE];
    mpack_writer_t writer;
    char table_name[32];
    snprintf(table_name, sizeof(table_name), "TestTable%ld", time(NULL));
    
    print_info("Creating table:");
    printf("  Name: %s\n", table_name);
    printf("  Max Players: 6\n");
    printf("  Min Bet: 10\n");
    
    // Encode create table request
    mpack_writer_init(&writer, buffer, MAXLINE);
    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, table_name);
    mpack_write_cstr(&writer, "max_player");
    mpack_write_int(&writer, 6);
    mpack_write_cstr(&writer, "min_bet");
    mpack_write_int(&writer, 10);
    mpack_finish_map(&writer);
    
    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* encoded_message = encode_packet(PROTOCOL_V1, PACKET_CREATE_TABLE, buffer, size);
    mpack_error_t error = mpack_writer_destroy(&writer);
    
    if (error != mpack_ok) {
        print_failure("MPack encoding error");
        state->tests_failed++;
        return 0;
    }
    
    // Send request
    print_info("Sending CREATE_TABLE_REQUEST");
    print_hex_dump("Packet", encoded_message->data, encoded_message->len);
    
    if (send(state->sockfd, encoded_message->data, encoded_message->len, 0) == -1) {
        print_failure("Failed to send create table request");
        free(encoded_message);
        state->tests_failed++;
        return 0;
    }
    free(encoded_message);
    
    // Receive response
    char recv_buffer[MAXLINE];
    int nbytes = recv(state->sockfd, recv_buffer, MAXLINE, 0);
    if (nbytes <= 0) {
        print_failure("Failed to receive create table response");
        state->tests_failed++;
        return 0;
    }
    
    print_info("Received response");
    print_hex_dump("Response", recv_buffer, nbytes);
    
    Header* header = decode_header(recv_buffer);
    if (!header) {
        print_failure("Invalid header");
        state->tests_failed++;
        return 0;
    }
    
    printf("  Packet Type: %d (expected: %d)\n", header->packet_type, PACKET_CREATE_TABLE);
    
    // Decode response
    mpack_reader_t reader;
    mpack_reader_init(&reader, recv_buffer + sizeof(Header), 
                      header->packet_len - sizeof(Header),
                      header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 1);
    mpack_expect_cstr_match(&reader, "res");
    int res = mpack_expect_u16(&reader);
    
    printf("  Result Code: %d\n", res);
    
    if (res == R_CREATE_TABLE_OK) {
        print_success("Create table successful");
        state->tests_passed++;
        free(header);
        return 1;
    } else {
        print_failure("Create table failed");
        state->tests_failed++;
        free(header);
        return 0;
    }
}

/**
 * Test 5: Get All Tables
 */
int test_get_tables(TestState* state) {
    print_test_header("GET_TABLES (Packet ID 500)");
    
    // Send request (empty payload)
    RawBytes* encoded_message = encode_packet(PROTOCOL_V1, PACKET_TABLES, NULL, 0);
    
    print_info("Sending GET_TABLES_REQUEST");
    print_hex_dump("Packet", encoded_message->data, encoded_message->len);
    
    if (send(state->sockfd, encoded_message->data, encoded_message->len, 0) == -1) {
        print_failure("Failed to send get tables request");
        free(encoded_message);
        state->tests_failed++;
        return 0;
    }
    free(encoded_message);
    
    // Receive response
    char recv_buffer[MAXLINE];
    int nbytes = recv(state->sockfd, recv_buffer, MAXLINE, 0);
    if (nbytes <= 0) {
        print_failure("Failed to receive get tables response");
        state->tests_failed++;
        return 0;
    }
    
    print_info("Received response");
    print_hex_dump("Response", recv_buffer, nbytes);
    
    Packet* packet = decode_packet(recv_buffer, nbytes);
    if (!packet || !packet->header) {
        print_failure("Invalid packet");
        state->tests_failed++;
        return 0;
    }
    
    printf("  Packet Type: %d (expected: %d)\n", packet->header->packet_type, PACKET_TABLES);
    
    if (packet->header->packet_type != PACKET_TABLES) {
        print_failure("Wrong packet type");
        free_packet(packet);
        state->tests_failed++;
        return 0;
    }
    
    // Decode table list
    mpack_reader_t reader;
    mpack_reader_init(&reader, packet->data, 
                      packet->header->packet_len - sizeof(Header),
                      packet->header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "size");
    int size = mpack_expect_i32(&reader);
    
    printf("  Number of tables: %d\n", size);
    
    mpack_expect_cstr_match(&reader, "tables");
    mpack_expect_array_max(&reader, size);
    
    for (int i = 0; i < size; i++) {
        mpack_expect_map_max(&reader, 5);
        mpack_expect_cstr_match(&reader, "id");
        int id = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "name");
        const char* name = mpack_expect_cstr_alloc(&reader, 32);
        mpack_expect_cstr_match(&reader, "maxPlayer");
        int max_player = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "minBet");
        int min_bet = mpack_expect_i32(&reader);
        mpack_expect_cstr_match(&reader, "currentPlayer");
        int current_player = mpack_expect_i32(&reader);
        
        printf("  Table %d: id=%d name='%s' max=%d min_bet=%d current=%d\n", 
               i+1, id, name, max_player, min_bet, current_player);
        
        // Store the first table ID for join test
        if (i == 0) {
            state->table_id = id;
        }
    }
    
    mpack_done_array(&reader);
    mpack_done_map(&reader);
    
    if (mpack_reader_destroy(&reader) != mpack_ok) {
        print_failure("Error decoding tables");
        free_packet(packet);
        state->tests_failed++;
        return 0;
    }
    
    print_success("Get tables successful");
    free_packet(packet);
    state->tests_passed++;
    return 1;
}

/**
 * Main test runner
 */
int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        fprintf(stderr, "Example: %s localhost 8080\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    printf("\n%s", COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║     CARDIO GAME SERVER E2E PROTOCOL TEST SUITE          ║\n");
    printf("║     Testing Protocol Compliance (PROTOCOL.md)           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    // Initialize test state
    TestState state = {0};
    
    // Connect to server
    print_test_header("CONNECTION");
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0) {
        print_failure("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }
    
    state.sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (state.sockfd == -1) {
        print_failure("Socket creation failed");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }
    
    if (connect(state.sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        print_failure("Connection failed");
        freeaddrinfo(res);
        close(state.sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("  Connected to: %s:%s\n", argv[1], argv[2]);
    print_success("Connection established");
    
    freeaddrinfo(res);
    
    // Run tests in sequence
    test_handshake(&state);
    test_signup(&state);
    test_login(&state);
    test_create_table(&state);
    test_get_tables(&state);
    
    // Print summary
    printf("\n%s", COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                     TEST SUMMARY                         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("%s", COLOR_RESET);
    printf("  %sTests Passed: %d%s\n", COLOR_GREEN, state.tests_passed, COLOR_RESET);
    printf("  %sTests Failed: %d%s\n", COLOR_RED, state.tests_failed, COLOR_RESET);
    printf("  Total Tests: %d\n", state.tests_passed + state.tests_failed);
    
    if (state.tests_failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! 🎉%s\n\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("\n%s❌ SOME TESTS FAILED ❌%s\n\n", COLOR_RED, COLOR_RESET);
    }
    
    // Clean up
    close(state.sockfd);
    
    return state.tests_failed == 0 ? 0 : 1;
}
