/**
 * E2E Test for Action Request with Signed int32 client_seq
 * 
 * This test verifies that the server correctly handles action requests
 * when the client_seq field is encoded as a signed int32 (which happens
 * when JavaScript msgpack encodes a value with the high bit set).
 */

#include "main.h"
#include <time.h>

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_CYAN "\033[1;36m"

// Don't redefine MAXLINE - use the one from protocol.h

typedef struct {
    int sockfd;
    char username[32];
    int user_id;
    int table_id;
    int balance;
    int seat;
    int game_id;
} E2ETestPlayer;

void print_success(const char* msg) {
    printf("%s✓ %s%s\n", COLOR_GREEN, msg, COLOR_RESET);
}

void print_failure(const char* msg) {
    printf("%s✗ %s%s\n", COLOR_RED, msg, COLOR_RESET);
}

void print_info(const char* msg) {
    printf("%s→ %s%s\n", COLOR_BLUE, msg, COLOR_RESET);
}

int connect_to_server(const char* host, const char* port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &servinfo) != 0) {
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);
    return (p == NULL) ? -1 : sockfd;
}

// Login using existing test user
int do_login(E2ETestPlayer* player, const char* username, const char* password) {
    char buffer[4096];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, sizeof(buffer));

    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, password);
    mpack_finish_map(&writer);

    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* packet = encode_packet(PROTOCOL_V1, 100, buffer, size);
    mpack_writer_destroy(&writer);

    send(player->sockfd, packet->data, packet->len, 0);
    free(packet->data);
    free(packet);

    // Receive response
    char recv_buf[4096];
    int nbytes = recv(player->sockfd, recv_buf, sizeof(recv_buf), 0);
    if (nbytes <= 0) return -1;

    Packet* resp = decode_packet(recv_buf, nbytes);
    if (!resp || resp->header->packet_type != 100) {
        if (resp) free_packet(resp);
        return -1;
    }

    // Parse response using mpack reader (the server sends result=0 for success)
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp->data, resp->header->packet_len - 5);
    
    uint32_t map_count = mpack_expect_map(&reader);
    
    int result = -1;
    for (uint32_t i = 0; i < map_count; i++) {
        char key[32];
        mpack_expect_cstr(&reader, key, sizeof(key));
        
        if (strcmp(key, "result") == 0) {
            result = mpack_expect_u16(&reader);
        } else if (strcmp(key, "user_id") == 0) {
            player->user_id = mpack_expect_i32(&reader);
        } else if (strcmp(key, "balance") == 0) {
            player->balance = mpack_expect_i32(&reader);
        } else {
            mpack_discard(&reader);
        }
    }
    
    mpack_error_t error = mpack_reader_destroy(&reader);
    if (error != mpack_ok) {
        free_packet(resp);
        return -1;
    }

    if (result == 0) {  // Success
        strncpy(player->username, username, sizeof(player->username) - 1);
    }

    free_packet(resp);
    return (result == 0) ? 0 : -1;
}

// Create a table
int create_table(E2ETestPlayer* player) {
    char buffer[4096];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, sizeof(buffer));

    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "SignedSeqTest");
    mpack_write_cstr(&writer, "max_player");
    mpack_write_int(&writer, 6);
    mpack_write_cstr(&writer, "min_bet");
    mpack_write_int(&writer, 10);
    mpack_finish_map(&writer);

    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* packet = encode_packet(PROTOCOL_V1, 300, buffer, size);
    mpack_writer_destroy(&writer);

    send(player->sockfd, packet->data, packet->len, 0);
    free(packet->data);
    free(packet);

    char recv_buf[4096];
    int nbytes = recv(player->sockfd, recv_buf, sizeof(recv_buf), 0);
    if (nbytes <= 0) return -1;

    Packet* resp = decode_packet(recv_buf, nbytes);
    if (!resp) return -1;

    // Parse response to get table_id
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp->data, resp->header->packet_len - 5);
    
    uint32_t map_count = mpack_expect_map(&reader);
    int res = -1;
    
    for (uint32_t i = 0; i < map_count; i++) {
        char key[32];
        mpack_expect_cstr(&reader, key, sizeof(key));
        
        if (strcmp(key, "res") == 0) {
            res = mpack_expect_int(&reader);
        } else if (strcmp(key, "table_id") == 0) {
            player->table_id = mpack_expect_i32(&reader);
        } else {
            mpack_discard(&reader);
        }
    }
    
    mpack_reader_destroy(&reader);
    free_packet(resp);
    return (res == R_CREATE_TABLE_OK) ? 0 : -1;
}

// Join a table
int e2e_join_table(E2ETestPlayer* player, int table_id) {
    char buffer[4096];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, sizeof(buffer));

    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "tableId");
    mpack_write_int(&writer, table_id);
    mpack_finish_map(&writer);

    size_t size = mpack_writer_buffer_used(&writer);
    RawBytes* packet = encode_packet(PROTOCOL_V1, 400, buffer, size);
    mpack_writer_destroy(&writer);

    send(player->sockfd, packet->data, packet->len, 0);
    free(packet->data);
    free(packet);

    // Receive responses (may get multiple: join result + game state)
    char recv_buf[MAXLINE];
    usleep(100000);  // Wait 100ms for response
    int nbytes = recv(player->sockfd, recv_buf, sizeof(recv_buf), MSG_DONTWAIT);
    if (nbytes <= 0) return -1;

    // Find JOIN response
    int offset = 0;
    while (offset < nbytes) {
        if (offset + 5 > nbytes) break;
        
        Header* header = decode_header(recv_buf + offset);
        if (!header || header->packet_len > nbytes - offset) {
            if (header) free(header);
            break;
        }

        if (header->packet_type == 400) {
            Packet* resp = decode_packet(recv_buf + offset, header->packet_len);
            if (resp) {
                mpack_tree_t tree;
                mpack_tree_init_data(&tree, resp->data, resp->header->packet_len - 5);
                mpack_tree_parse(&tree);
                mpack_node_t root = mpack_tree_root(&tree);
                
                // Check if we received a simple response (has "res" field)
                // OR a full game state (has "game_id" field when game starts)
                mpack_node_t res_node = mpack_node_map_cstr_optional(root, "res");
                mpack_node_t game_id_node = mpack_node_map_cstr_optional(root, "game_id");
                
                if (!mpack_node_is_missing(game_id_node)) {
                    // Game started - we got full game state
                    // Extract seat from players array if needed
                    player->table_id = table_id;
                    // The seat was assigned on the server side
                    player->seat = 1;  // Second player gets seat 1
                    mpack_tree_destroy(&tree);
                    free_packet(resp);
                    free(header);
                    return 0;  // Success - game state received
                }
                
                int res = mpack_node_int(res_node);
                if (res == R_JOIN_TABLE_OK) {  // 401
                    player->seat = mpack_node_int(mpack_node_map_cstr(root, "seat"));
                    player->table_id = table_id;
                }
                
                mpack_tree_destroy(&tree);
                free_packet(resp);
                free(header);
                return (res == R_JOIN_TABLE_OK) ? 0 : -1;
            }
        }
        offset += header->packet_len;
        free(header);
    }

    return -1;
}

/**
 * Send action with signed int32 client_seq (simulating JS msgpack behavior)
 * Returns: 0 on success, -1 on failure
 */
int send_action_with_signed_seq(E2ETestPlayer* player, int game_id, const char* action_type, uint32_t client_seq) {
    char buffer[4096];
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, sizeof(buffer));

    mpack_start_map(&writer, 3);

    mpack_write_cstr(&writer, "game_id");
    mpack_write_int(&writer, game_id);

    mpack_write_cstr(&writer, "action");
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "type");
    mpack_write_cstr(&writer, action_type);
    mpack_finish_map(&writer);

    mpack_write_cstr(&writer, "client_seq");
    // IMPORTANT: Write as signed int32 to simulate JavaScript msgpack behavior
    // when the high bit is set, JS encodes as int32 (d2) not uint32 (ce)
    mpack_write_i32(&writer, (int32_t)client_seq);

    mpack_finish_map(&writer);

    size_t size = mpack_writer_buffer_used(&writer);
    mpack_error_t error = mpack_writer_destroy(&writer);
    if (error != mpack_ok) return -1;

    RawBytes* packet = encode_packet(PROTOCOL_V1, 450, buffer, size);
    int result = send(player->sockfd, packet->data, packet->len, 0);
    free(packet->data);
    free(packet);

    return (result > 0) ? 0 : -1;
}

/**
 * Receive action result
 * Returns: result code (0=success), -1 on failure, -2 on timeout
 */
int receive_action_result(E2ETestPlayer* player, uint32_t expected_seq) {
    char recv_buf[MAXLINE];
    
    for (int attempt = 0; attempt < 5; attempt++) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(player->sockfd, &readfds);
        
        struct timeval tv = {2, 0};
        int ret = select(player->sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0) return -2;

        int nbytes = recv(player->sockfd, recv_buf, sizeof(recv_buf), 0);
        if (nbytes <= 0) return -1;

        // Parse packets to find action result (451)
        int offset = 0;
        while (offset < nbytes) {
            if (offset + 5 > nbytes) break;
            
            Header* header = decode_header(recv_buf + offset);
            if (!header || header->packet_len > nbytes - offset) {
                if (header) free(header);
                break;
            }

            if (header->packet_type == 451) {  // ACTION_RESULT
                Packet* resp = decode_packet(recv_buf + offset, header->packet_len);
                if (resp) {
                    mpack_tree_t tree;
                    mpack_tree_init_data(&tree, resp->data, resp->header->packet_len - 5);
                    mpack_tree_parse(&tree);
                    mpack_node_t root = mpack_tree_root(&tree);
                    
                    int result = mpack_node_int(mpack_node_map_cstr(root, "result"));
                    uint32_t seq = mpack_node_u32(mpack_node_map_cstr(root, "client_seq"));
                    
                    mpack_tree_destroy(&tree);
                    free_packet(resp);
                    free(header);
                    
                    if (seq == expected_seq) {
                        return result;
                    }
                }
            }
            offset += header->packet_len;
            free(header);
        }
    }

    return -2;  // Timeout
}

int main(int argc, char* argv[]) {
    const char* host = "localhost";
    const char* port = "8080";
    int tests_passed = 0;
    int tests_failed = 0;

    printf("\n%s═══════════════════════════════════════════════════════════%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s  E2E Test: Action Request with Signed int32 client_seq    %s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s═══════════════════════════════════════════════════════════%s\n\n", COLOR_CYAN, COLOR_RESET);

    // Initialize players
    E2ETestPlayer player1 = {0}, player2 = {0};

    // Connect both players
    print_info("Connecting player 1...");
    player1.sockfd = connect_to_server(host, port);
    if (player1.sockfd < 0) {
        print_failure("Player 1 failed to connect");
        return 1;
    }
    print_success("Player 1 connected");

    print_info("Connecting player 2...");
    player2.sockfd = connect_to_server(host, port);
    if (player2.sockfd < 0) {
        print_failure("Player 2 failed to connect");
        close(player1.sockfd);
        return 1;
    }
    print_success("Player 2 connected");

    // Login using existing test users from data.sql
    print_info("Logging in player 1 (user1)...");
    if (do_login(&player1, "user1", "password12345") != 0) {
        print_failure("Player 1 login failed");
        tests_failed++;
        goto cleanup;
    }
    print_success("Player 1 logged in");
    tests_passed++;

    print_info("Logging in player 2 (user2)...");
    if (do_login(&player2, "user2", "password12345") != 0) {
        print_failure("Player 2 login failed");
        tests_failed++;
        goto cleanup;
    }
    print_success("Player 2 logged in");
    tests_passed++;

    // Create table (player 1 is auto-joined when creating)
    print_info("Player 1 creating table (auto-joins)...");
    if (create_table(&player1) != 0) {
        print_failure("Failed to create table");
        tests_failed++;
        goto cleanup;
    }
    printf("  Table created with ID: %d\n", player1.table_id);
    player1.seat = 0;  // Creator is placed at seat 0
    print_success("Table created, player 1 auto-joined");
    tests_passed++;

    // Player 2 joins
    print_info("Player 2 joining table...");
    if (e2e_join_table(&player2, player1.table_id) != 0) {
        print_failure("Player 2 failed to join table");
        tests_failed++;
        goto cleanup;
    }
    printf("  Player 2 seated at: %d\n", player2.seat);
    print_success("Player 2 joined table");
    tests_passed++;

    // Wait for game to start
    usleep(500000);  // 500ms

    // Drain any pending game state updates
    char drain_buf[MAXLINE];
    recv(player1.sockfd, drain_buf, sizeof(drain_buf), MSG_DONTWAIT);
    recv(player2.sockfd, drain_buf, sizeof(drain_buf), MSG_DONTWAIT);

    // TEST: Send action with signed int32 client_seq (high bit set)
    // Value 0xa68faaae has high bit set, so JS msgpack encodes it as int32
    uint32_t test_seq = 0xa68faaae;
    
    printf("\n%s═══ KEY TEST: Signed int32 client_seq ═══%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Testing client_seq value: 0x%08X (%u)\n", test_seq, test_seq);
    printf("  (This value has high bit set, JS msgpack encodes as signed int32)\n");
    
    print_info("Sending action with signed int32 client_seq...");
    if (send_action_with_signed_seq(&player1, 1, "call", test_seq) != 0) {
        print_failure("Failed to send action");
        tests_failed++;
        goto cleanup;
    }
    print_success("Action sent successfully");

    // Receive and validate action result
    print_info("Waiting for action result...");
    int result = receive_action_result(&player1, test_seq);
    
    if (result == -2) {
        print_failure("Timeout waiting for action result");
        tests_failed++;
    } else if (result == -1) {
        print_failure("Failed to receive action result");
        tests_failed++;
    } else {
        printf("  Action result code: %d\n", result);
        // We expect either success (0) or a game logic error (403/409)
        // The key is that we don't get a decode error (which would be no response)
        if (result == 0 || result == 403 || result == 409) {
            print_success("Server correctly decoded action with signed int32 client_seq!");
            tests_passed++;
        } else if (result == 500) {
            print_failure("Server returned internal error - decoding may have failed");
            tests_failed++;
        } else {
            printf("  Unexpected result code: %d\n", result);
            tests_failed++;
        }
    }

cleanup:
    close(player1.sockfd);
    close(player2.sockfd);

    printf("\n%s═══════════════════════════════════════════════════════════%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  RESULTS: %s%d passed%s, %s%d failed%s\n",
           COLOR_GREEN, tests_passed, COLOR_RESET,
           tests_failed > 0 ? COLOR_RED : COLOR_GREEN, tests_failed, COLOR_RESET);
    printf("%s═══════════════════════════════════════════════════════════%s\n\n", COLOR_CYAN, COLOR_RESET);

    return tests_failed > 0 ? 1 : 0;
}
