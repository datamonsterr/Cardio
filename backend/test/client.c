#include "main.h"

void display_menu()
{
    printf("1. Login\n");
    printf("2. Signup\n");
    printf("3. Exit\n");
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
        mpack_error_t error;
        Header *header;
        size_t size;
        RawBytes *encoded_message;
        char buffer[MAXLINE];
        char *data;
        mpack_writer_t writer;

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
            Header *encoded_header = decode_header(encoded_message->data);
            printf("header len %hu\n", encoded_header->packet_len);
            printf("header type %hu\n", encoded_header->packet_type);
            printf("header ver %hhu\n", encoded_header->protocol_ver);

            // printf("Sending login request len %zu\n", encoded_message->len);

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
            close(servfd);
            freeaddrinfo(res);
            exit(EXIT_SUCCESS);
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