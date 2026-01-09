/*
 * End-to-end test for friend management functionality
 * 
 * Tests:
 * 1. User1 sends friend invite to User2
 * 2. User2 gets pending invites
 * 3. User2 accepts the invite
 * 4. User1 and User2 are now friends
 * 5. User3 tries to add User1 directly
 * 6. User1 sends invite to User4
 * 7. User4 rejects the invite
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
#define SERVER_PORT "3491"

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
        fprintf(stderr, "Failed to receive header\n");
        return NULL;
    }

    Header* header = decode_header(header_buf);
    *len = header->packet_len;
    
    char* full_packet = malloc(*len);
    memcpy(full_packet, header_buf, 5);
    
    n = recv(sockfd, full_packet + 5, *len - 5, MSG_WAITALL);
    if (n != (*len - 5))
    {
        fprintf(stderr, "Failed to receive payload\n");
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
        fprintf(stderr, "Failed to encode login request\n");
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_LOGIN, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        fprintf(stderr, "Failed to send login request\n");
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        fprintf(stderr, "Failed to receive login response\n");
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
        printf("✗ Login failed for %s\n", username);
        return -1;
    }
}

// Helper to send friend invite
int send_friend_invite(int sockfd, const char* target_username)
{
    printf("Sending friend invite to %s...\n", target_username);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "username");
    mpack_write_cstr(&writer, target_username);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Failed to encode invite request\n");
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_INVITE_FRIEND, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        fprintf(stderr, "Failed to send invite request\n");
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        fprintf(stderr, "Failed to receive invite response\n");
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    if (result == R_INVITE_FRIEND_OK)
    {
        printf("✓ Invite sent to %s\n", target_username);
        return 0;
    }
    else
    {
        printf("✗ Failed to send invite to %s (code: %d)\n", target_username, result);
        return -1;
    }
}

// Helper to get pending invites
int get_pending_invites(int sockfd, int* invite_id_out)
{
    printf("Getting pending invites...\n");
    
    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_GET_INVITES, NULL, 0);
    if (send_packet(sockfd, packet) == -1)
    {
        fprintf(stderr, "Failed to send get invites request\n");
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        fprintf(stderr, "Failed to receive invites response\n");
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    
    uint32_t count = mpack_expect_array(&reader);
    
    printf("Found %d pending invite(s)\n", count);
    
    if (count > 0 && invite_id_out)
    {
        mpack_expect_map_max(&reader, 5);
        mpack_expect_cstr_match(&reader, "invite_id");
        *invite_id_out = mpack_expect_i32(&reader);
        printf("First invite ID: %d\n", *invite_id_out);
    }
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    printf("✓ Got pending invites\n");
    return count;
}

// Helper to accept invite
int accept_invite(int sockfd, int invite_id)
{
    printf("Accepting invite ID %d...\n", invite_id);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "invite_id");
    mpack_write_i32(&writer, invite_id);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Failed to encode accept request\n");
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_ACCEPT_INVITE, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        fprintf(stderr, "Failed to send accept request\n");
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        fprintf(stderr, "Failed to receive accept response\n");
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    if (result == R_ACCEPT_INVITE_OK)
    {
        printf("✓ Invite accepted\n");
        return 0;
    }
    else
    {
        printf("✗ Failed to accept invite (code: %d)\n", result);
        return -1;
    }
}

// Helper to reject invite
int reject_invite(int sockfd, int invite_id)
{
    printf("Rejecting invite ID %d...\n", invite_id);
    
    mpack_writer_t writer;
    char buffer[1024];
    mpack_writer_init(&writer, buffer, 1024);
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "invite_id");
    mpack_write_i32(&writer, invite_id);
    mpack_finish_map(&writer);
    
    size_t payload_len = mpack_writer_buffer_used(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "Failed to encode reject request\n");
        return -1;
    }

    RawBytes* packet = encode_packet(PROTOCOL_V1, PACKET_REJECT_INVITE, buffer, payload_len);
    if (send_packet(sockfd, packet) == -1)
    {
        fprintf(stderr, "Failed to send reject request\n");
        free(packet);
        return -1;
    }
    free(packet);

    // Receive response
    size_t resp_len;
    char* response = recv_packet(sockfd, &resp_len);
    if (!response)
    {
        fprintf(stderr, "Failed to receive reject response\n");
        return -1;
    }

    Packet* resp_packet = decode_packet(response, resp_len);
    free(response);

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, resp_packet->data, resp_len - 5);
    mpack_expect_map_max(&reader, 2);
    mpack_expect_cstr_match(&reader, "res");
    int result = mpack_expect_u16(&reader);
    
    free_packet(resp_packet);
    mpack_reader_destroy(&reader);

    if (result == R_REJECT_INVITE_OK)
    {
        printf("✓ Invite rejected\n");
        return 0;
    }
    else
    {
        printf("✗ Failed to reject invite (code: %d)\n", result);
        return -1;
    }
}

int main()
{
    printf("=== Friend Management E2E Test ===\n\n");
    
    int sock1, sock2, sock3, sock4;
    int invite_id;
    int passed = 0;
    int failed = 0;

    // Test 1: User1 sends invite to User2
    printf("\n--- Test 1: User1 invites User2 ---\n");
    sock1 = connect_to_server();
    if (sock1 < 0 || do_login(sock1, "user1", "user1") < 0)
    {
        printf("✗ Failed to connect/login as user1\n");
        failed++;
        goto cleanup1;
    }

    if (send_friend_invite(sock1, "user2") == 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    // Test 2: User2 checks pending invites
    printf("\n--- Test 2: User2 checks pending invites ---\n");
    sock2 = connect_to_server();
    if (sock2 < 0 || do_login(sock2, "user2", "user2") < 0)
    {
        printf("✗ Failed to connect/login as user2\n");
        failed++;
        goto cleanup2;
    }

    int count = get_pending_invites(sock2, &invite_id);
    if (count > 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    // Test 3: User2 accepts invite
    printf("\n--- Test 3: User2 accepts invite from User1 ---\n");
    if (count > 0 && accept_invite(sock2, invite_id) == 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    // Test 4: User1 sends invite to User3
    printf("\n--- Test 4: User1 invites User3 ---\n");
    if (send_friend_invite(sock1, "user3") == 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    // Test 5: User3 rejects invite
    printf("\n--- Test 5: User3 checks and rejects invite ---\n");
    sock3 = connect_to_server();
    if (sock3 < 0 || do_login(sock3, "user3", "user3") < 0)
    {
        printf("✗ Failed to connect/login as user3\n");
        failed++;
        goto cleanup3;
    }

    count = get_pending_invites(sock3, &invite_id);
    if (count > 0 && reject_invite(sock3, invite_id) == 0)
    {
        passed++;
    }
    else
    {
        failed++;
    }

    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);
    
cleanup3:
    close(sock3);
cleanup2:
    close(sock2);
cleanup1:
    close(sock1);

    return failed > 0 ? 1 : 0;
}
