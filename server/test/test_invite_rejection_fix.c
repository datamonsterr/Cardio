#include "../include/main.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TEST_PORT "8080"
#define TEST_HOST "127.0.0.1"

int connect_to_server() {
    int sockfd;
    struct sockaddr_in server_addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(TEST_PORT));
    
    if (inet_pton(AF_INET, TEST_HOST, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int send_login(int sockfd, const char* username, const char* password) {
    // Create login request
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, sizeof(buffer));
    
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "user");
    mpack_write_cstr(&writer, username);
    mpack_write_cstr(&writer, "pass");
    mpack_write_cstr(&writer, password);
    mpack_finish_map(&writer);
    
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        printf("Failed to encode login request\n");
        return -1;
    }
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    RawBytes* packet = encode_packet(PROTOCOL_V1, 100, buffer, payload_len);
    
    int len = packet->len;
    if (sendall(sockfd, packet->data, &len) == -1) {
        printf("Failed to send login packet\n");
        free(packet->data);
        free(packet);
        return -1;
    }
    
    free(packet->data);
    free(packet);
    
    // Read response
    char response[1024];
    int bytes_read = recv(sockfd, response, sizeof(response), 0);
    if (bytes_read <= 0) {
        printf("Failed to read login response\n");
        return -1;
    }
    
    Packet* resp_packet = decode_packet(response, bytes_read);
    if (!resp_packet) {
        printf("Failed to decode response packet\n");
        return -1;
    }
    
    // Parse response
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_packet->header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 10);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);
    
    return result;
}

int send_invite_friend(int sockfd, const char* friend_username) {
    // Create invite friend request
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, sizeof(buffer));
    
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "username");
    mpack_write_cstr(&writer, friend_username);
    mpack_finish_map(&writer);
    
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        printf("Failed to encode invite request\n");
        return -1;
    }
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_INVITE_FRIEND, buffer, payload_len);
    
    int len = packet->len;
    if (sendall(sockfd, packet->data, &len) == -1) {
        printf("Failed to send invite packet\n");
        free(packet->data);
        free(packet);
        return -1;
    }
    
    free(packet->data);
    free(packet);
    
    // Read response
    char response[1024];
    int bytes_read = recv(sockfd, response, sizeof(response), 0);
    if (bytes_read <= 0) {
        printf("Failed to read invite response\n");
        return -1;
    }
    
    Packet* resp_packet = decode_packet(response, bytes_read);
    if (!resp_packet) {
        printf("Failed to decode response packet\n");
        return -1;
    }
    
    // Parse response
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_packet->header->packet_len - sizeof(Header));
    
    mpack_expect_map_max(&reader, 10);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);
    
    return result;
}

int main() {
    printf("=== Testing Friend Invite Rejection Fix ===\n\n");
    
    // Test scenario: User1 invites User2, User2 rejects, User1 invites again
    printf("--- Step 1: User1 logs in ---\n");
    int user1_sock = connect_to_server();
    if (user1_sock < 0) {
        printf("✗ Failed to connect as user1\n");
        return 1;
    }
    
    int login_result = send_login(user1_sock, "user1", "pass123");
    if (login_result != R_LOGIN_OK) {
        printf("✗ User1 login failed (result=%d)\n", login_result);
        close(user1_sock);
        return 1;
    }
    printf("✓ User1 logged in successfully\n");
    
    printf("\n--- Step 2: User1 sends invite to user2 ---\n");
    int invite_result = send_invite_friend(user1_sock, "user2");
    if (invite_result != R_INVITE_FRIEND_OK) {
        printf("✗ First invite failed (result=%d)\n", invite_result);
        close(user1_sock);
        return 1;
    }
    printf("✓ First invite sent successfully\n");
    
    // Sleep a bit to let the server process
    sleep(1);
    
    printf("\n--- Step 3: User2 logs in and rejects invite ---\n");
    int user2_sock = connect_to_server();
    if (user2_sock < 0) {
        printf("✗ Failed to connect as user2\n");
        close(user1_sock);
        return 1;
    }
    
    login_result = send_login(user2_sock, "user2", "pass123");
    if (login_result != R_LOGIN_OK) {
        printf("✗ User2 login failed (result=%d)\n", login_result);
        close(user1_sock);
        close(user2_sock);
        return 1;
    }
    printf("✓ User2 logged in successfully\n");
    
    // Here we would need to get invite ID and reject it
    // For this test, let's simulate by directly updating the database
    printf("~ Simulating rejection by updating database directly\n");
    
    PGconn* conn = PQconnectdb(dbconninfo);
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        printf("✗ Failed to connect to database\n");
        close(user1_sock);
        close(user2_sock);
        return 1;
    }
    
    // Update the invite to rejected status
    const char* update_query = "UPDATE friend_invites SET status = 'rejected' WHERE from_user_id = 1 AND to_user_id = 2";
    PGresult* res = PQexec(conn, update_query);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("✗ Failed to update invite status: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        close(user1_sock);
        close(user2_sock);
        return 1;
    }
    PQclear(res);
    PQfinish(conn);
    printf("✓ Invite marked as rejected\n");
    
    close(user2_sock);
    
    printf("\n--- Step 4: User1 sends invite again (this should work now) ---\n");
    invite_result = send_invite_friend(user1_sock, "user2");
    if (invite_result != R_INVITE_FRIEND_OK) {
        printf("✗ Second invite failed (result=%d) - BUG NOT FIXED!\n", invite_result);
        close(user1_sock);
        return 1;
    }
    printf("✓ Second invite sent successfully - BUG FIXED!\n");
    
    close(user1_sock);
    
    printf("\n=== Test Completed Successfully ===\n");
    printf("The duplicate key constraint bug has been fixed!\n");
    
    return 0;
}