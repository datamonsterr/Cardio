#include "main.h"

void display_menu()
{
    printf("1. Login\n");
    printf("2. Signup\n");
    printf("3. Create table\n");
    printf("4. Join table\n");
    printf("5. Leave table\n");
    printf("6. Bet\n");
    printf("7. Hit\n");
    printf("8. Stand\n");
    printf("9. Double\n");
    printf("10. Split\n");
    printf("11. Surrender\n");
    printf("12. Logout\n");
    printf("13. Exit\n");
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s [host] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int servfd;
    struct addrinfo hints, *res;

    // Prepare for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Create socket
    servfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (servfd == -1)
    {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(servfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("connect");
        freeaddrinfo(res);
        close(servfd);
        exit(EXIT_FAILURE);
    }

    char recv_buffer[MAXLINE];

    while (1)
    {
        display_menu();

        int choice;
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Consume newline

        char username[32];
        char password[32];
        char fullname[64];
        char email[64];
        char country[32];
        char gender[8];
        char phone[16];
        char dob[16];
        int nbytes;
        mpack_error_t error;
        Header *header;
        size_t size;
        RawBytes *encoded_message;
        Packet *decoded_packet;
        char buffer[MAXLINE];
        char *data;
        mpack_writer_t writer;
        char table_name[32];
        int max_player;
        int min_bet;

        switch (choice)
        {
        case 1:
            printf("Enter username <space> password: ");

            // Use safer input to avoid overflow
            if (scanf("%29s %29s", username, password) != 2)
            {
                fprintf(stderr, "Invalid input. Please enter username and password.\n");
                continue;
            }

            // Initialize MPack writer
            mpack_writer_init(&writer, buffer, 4096);

            // Write data to the MPack map
            mpack_start_map(&writer, 2);
            mpack_write_cstr(&writer, "user");
            mpack_write_cstr(&writer, username);
            mpack_write_cstr(&writer, "pass");
            mpack_write_cstr(&writer, password);
            mpack_finish_map(&writer);

            // Retrieve MPack buffer
            data = writer.buffer;
            size = mpack_writer_buffer_used(&writer);

            // Encode the packet
            encoded_message = encode_packet(1, 100, data, size);
            printf("Sending login request len %zu\n", encoded_message->len);

            // Send the encoded message
            if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
            {
                perror("send");
                free(encoded_message);
                break;
            }
            error = mpack_writer_destroy(&writer);
            if (error != mpack_ok)
            {
                fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(error));
                break;
            }

            if (recv(servfd, recv_buffer, MAXLINE, 0) == -1)
            {
                perror("recv");
                break;
            }

            // Decode the response
            header = decode_header(recv_buffer);
            if (header == NULL)
            {
                fprintf(stderr, "Invalid header\n");
                break;
            }

            if (header->packet_type == 100)
            {
                mpack_reader_t reader;
                mpack_reader_init(&reader, recv_buffer + sizeof(Header), header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));
                // if there is error, the payload will contains 2 fields: res and msg, if not, it will contains only res

                mpack_expect_map_max(&reader, 1);
                mpack_expect_cstr_match(&reader, "res");
                int res = mpack_expect_u16(&reader);

                if (res == R_LOGIN_OK)
                {
                    printf("Login success with code %d\n", res);
                }
                else
                {
                    printf("Login failed with code %d\n", res);
                }
            }
            else
            {
                fprintf(stderr, "Invalid packet type\n");
            }

            // Free dynamically allocated memory
            free(encoded_message);
            break;
        case 2:
            printf("Username: ");
            scanf("%31s", username);
            printf("Password: ");
            scanf("%31s", password);
            printf("Fullname: ");
            scanf("%63s", fullname);
            printf("Phone: ");
            scanf("%15s", phone);
            printf("DOB: ");
            scanf("%15s", dob);
            printf("Email: ");
            scanf("%63s", email);
            printf("Country: ");
            scanf("%15s", country);
            printf("Gender: ");
            scanf("%7s", gender);

            /*          printf("Username: %s\n", username);
                        printf("Password: %s\n", password);
                        printf("Fullname: %s\n", fullname);
                        printf("Phone: %s\n", phone);
                        printf("DOB: %s\n", dob);
                        printf("Email: %s\n", email);
                        printf("Country: %s\n", country);
                        printf("Gender: %s\n", gender);
             */
            // Initialize MPack writer
            mpack_writer_init(&writer, buffer, MAXLINE);

            // Write data to the MPack map
            mpack_start_map(&writer, 8);
            mpack_write_cstr(&writer, "user");
            mpack_write_cstr(&writer, username);
            mpack_write_cstr(&writer, "pass");
            mpack_write_cstr(&writer, password);
            mpack_write_cstr(&writer, "fullname");
            mpack_write_cstr(&writer, fullname);
            mpack_write_cstr(&writer, "phone");
            mpack_write_cstr(&writer, phone);
            mpack_write_cstr(&writer, "dob");
            mpack_write_cstr(&writer, dob);
            mpack_write_cstr(&writer, "email");
            mpack_write_cstr(&writer, email);
            mpack_write_cstr(&writer, "country");
            mpack_write_cstr(&writer, country);
            mpack_write_cstr(&writer, "gender");
            mpack_write_cstr(&writer, gender);
            mpack_finish_map(&writer);

            // Retrieve MPack buffer
            data = writer.buffer;
            size = mpack_writer_buffer_used(&writer);

            // Encode the packet
            encoded_message = encode_packet(1, 200, data, size);

            error = mpack_writer_destroy(&writer);
            if (error != mpack_ok)
            {
                fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(error));
                break;
            }
            // Send the encoded message
            if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
            {
                perror("send");
                free(encoded_message);
                break;
            }

            if (recv(servfd, recv_buffer, MAXLINE, 0) == -1)
            {
                perror("recv");
                break;
            }

            // Decode the response
            header = decode_header(recv_buffer);
            if (header == NULL)
            {
                fprintf(stderr, "Invalid header\n");
                break;
            }

            if (header->packet_type == 200)
            {
                mpack_reader_t reader;
                mpack_reader_init(&reader, recv_buffer + sizeof(Header), header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));
                // if there is error, the payload will contains 2 fields: res and msg, if not, it will contains only res

                mpack_expect_map_max(&reader, 1);
                mpack_expect_cstr_match(&reader, "res");
                int res = mpack_expect_u16(&reader);

                if (res == R_SIGNUP_OK)
                {
                    printf("Signup success with code %d\n", res);
                }
                else
                {
                    printf("Signup failed with code %d\n", res);
                }
            }
            else
            {
                fprintf(stderr, "Invalid packet type\n");
            }

            break;

        case 3:
            printf("Table name:");
            scanf("%31s", table_name);
            printf("Max player:");
            scanf("%d", &max_player);
            printf("Min bet:");
            scanf("%d", &min_bet);

            // Initialize MPack writer
            mpack_writer_init(&writer, buffer, MAXLINE);
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "name");
            mpack_write_cstr(&writer, table_name);
            mpack_write_cstr(&writer, "max_player");
            mpack_write_int(&writer, max_player);
            mpack_write_cstr(&writer, "min_bet");
            mpack_write_int(&writer, min_bet);
            mpack_finish_map(&writer);

            // Retrieve MPack buffer
            data = writer.buffer;
            size = mpack_writer_buffer_used(&writer);

            // Encode the packet
            encoded_message = encode_packet(1, 300, data, size);

            error = mpack_writer_destroy(&writer);
            if (error != mpack_ok)
            {
                fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(error));
                break;
            }

            // Send the encoded message
            if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
            {
                perror("send");
                free(encoded_message);
                break;
            }

            if (recv(servfd, recv_buffer, MAXLINE, 0) == -1)
            {
                perror("recv");
                break;
            }

            // Decode the response
            header = decode_header(recv_buffer);
            if (header == NULL)
            {
                fprintf(stderr, "Invalid header\n");
                break;
            }

            if (header->packet_type == 300)
            {
                mpack_reader_t reader;
                mpack_reader_init(&reader, recv_buffer + sizeof(Header), header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));
                // if there is error, the payload will contains 2 fields: res and msg, if not, it will contains only res

                mpack_expect_map_max(&reader, 1);
                mpack_expect_cstr_match(&reader, "res");
                int res = mpack_expect_u16(&reader);

                if (res == R_CREATE_TABLE_OK)
                {
                    printf("Create table success with code %d\n", res);
                }
                else
                {
                    printf("Create table failed with code %d\n", res);
                }
            }
            else
            {
                fprintf(stderr, "Invalid packet type\n");
            }

            break;
        case 4:
            // send get all tables request
            encoded_message = encode_packet(1, PACKET_TABLES, NULL, 0);

            if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
            {
                perror("send");
                free(encoded_message);
                break;
            }

            // receive response
            if (recv(servfd, recv_buffer, MAXLINE, 0) == -1)
            {
                perror("recv");
                break;
            }

            // Decode the response
            decoded_packet = decode_packet(recv_buffer, MAXLINE);
            header = decoded_packet->header;

            if (header == NULL)
            {
                fprintf(stderr, "Invalid packet\n");
                break;
            }

            if (header->packet_type == PACKET_TABLES)
            {
                mpack_reader_t reader;
                mpack_reader_init(&reader, decoded_packet->data, header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));

                mpack_expect_map_max(&reader, 2);

                mpack_expect_cstr_match(&reader, "size");
                int size = mpack_expect_i32(&reader);

                mpack_expect_cstr_match(&reader, "tables");
                mpack_expect_array_max(&reader, size);
                for (int i = 0; i < size; i++)
                {
                    mpack_expect_map_max(&reader, 5);
                    mpack_expect_cstr_match(&reader, "id");
                    int id = mpack_expect_i32(&reader);
                    mpack_expect_cstr_match(&reader, "name");
                    const char *name = mpack_expect_cstr_alloc(&reader, 32);
                    mpack_expect_cstr_match(&reader, "maxPlayer");
                    int max_player = mpack_expect_i32(&reader);
                    mpack_expect_cstr_match(&reader, "minBet");
                    int min_bet = mpack_expect_i32(&reader);
                    mpack_expect_cstr_match(&reader, "currentPlayer");
                    int current_player = mpack_expect_i32(&reader);

                    printf("%d: %s, max player: %d, min bet: %d, current player: %d\n", id, name, max_player, min_bet, current_player);
                }

                mpack_done_array(&reader);
                mpack_done_map(&reader);
                if (mpack_reader_destroy(&reader) != mpack_ok)
                {
                    fprintf(stderr, "An error occurred decoding the message\n");
                }
                int table_id;
                printf("Enter table id: ");
                scanf("%d", &table_id);

                // Initialize MPack writer
                mpack_writer_init(&writer, buffer, MAXLINE);
                mpack_start_map(&writer, 1);
                mpack_write_cstr(&writer, "table_id");
                mpack_write_i32(&writer, table_id);
                mpack_finish_map(&writer);

                encoded_message = encode_packet(1, PACKET_JOIN_TABLE, writer.buffer, mpack_writer_buffer_used(&writer));

                error = mpack_writer_destroy(&writer);
                if (error != mpack_ok)
                {
                    fprintf(stderr, "MPack encoding error: %s\n", mpack_error_to_string(error));
                    break;
                }

                // Send the encoded message
                if (send(servfd, encoded_message->data, encoded_message->len, 0) == -1)
                {
                    perror("send");
                    free(encoded_message);
                    break;
                }

                nbytes = recv(servfd, recv_buffer, MAXLINE, 0);
                printf("Received %d bytes\n", nbytes);
                if (nbytes == -1)
                {
                    perror("recv");
                    break;
                }

                // Decode the response
                header = decode_header(recv_buffer);
                if (header == NULL)
                {
                    fprintf(stderr, "Invalid header\n");
                    break;
                }

                printf("Header len %d\n", header->packet_len);
                printf("Header type %d\n", header->packet_type);
                printf("Header ver %d\n", header->protocol_ver);
                if (header->packet_type == PACKET_JOIN_TABLE)
                {
                    mpack_reader_init(&reader, decoded_packet->data, header->packet_len - sizeof(Header), header->packet_len - sizeof(Header));

                    mpack_expect_map_max(&reader, 1);
                    mpack_expect_cstr_match(&reader, "res");
                    int res = mpack_expect_u16(&reader);

                    if (res == R_JOIN_TABLE_OK)
                    {
                        printf("Join table success with code %d\n", res);
                    }
                    else if (res == R_JOIN_TABLE_FULL)
                    {
                        printf("Table is full with code %d\n", res);
                    }
                    else
                    {
                        printf("Join table failed with code %d\n", res);
                    }
                }
                else
                {
                    fprintf(stderr, "Invalid packet type %d abc\n", header->packet_type);
                }
            }
            else
            {
                fprintf(stderr, "Invalid packet type %d\n", header->packet_type);
            }
            break;

        default:
            break;
        }

        memset(recv_buffer, 0, MAXLINE);
        memset(buffer, 0, MAXLINE);
    }

    // Clean up resources
    freeaddrinfo(res);
    close(servfd);

    return 0;
}