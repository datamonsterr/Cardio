// #include "../include/db.h"

// void encode_account(msgpack_sbuffer *buffer, const char *username, const char *password) {
//     // initialize a MessagePack packer
//     msgpack_packer pk;
//     msgpack_packer_init(&pk, buffer, msgpack_sbuffer_write);

//     // pac username and password as a map
//     msgpack_pack_map(&pk,2);

//     // pack username key (the first msgpack input is considered as keys)
//     msgpack_pack_str(&pk,strlen("Username"));
//     msgpack_pack_str_body(&pk,"Username",strlen("Username"));
//     msgpack_pack_str(&pk,strlen("Password"));
//     msgpack_pack_str_body(&pk,"Password",strlen("Password"));

//     msgpack_pack_str(&pk,strlen(username));
//     msgpack_pack_str_body(&pk,username,strlen(username));

//     msgpack_pack_str(&pk,strlen(password));
//     msgpack_pack_str_body(&pk,password,strlen(password));
// }

// struct Player decode_account(const msgpack_sbuffer *buffer) {
//     struct Player player;
//     // unpack the MessagePack data
//     msgpack_unpacked msg;
//     msgpack_unpacked_init(&msg);

//     // unpack until there is no chunk (map) left in buffer
//     if (msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL)) {
//         msgpack_object obj = msg.data;

//         // ensure the object is a map 
//         if (obj.type == MSGPACK_OBJECT_MAP) {
//             msgpack_object_kv *kv = obj.via.map.ptr;
//             for (int i=0;i<obj.via.map.size;i++) {
//                 strncpy(player.fullname, kv[i].val.via.str.ptr, sizeof(player.fullname)-1);
//             }
//             return player;
//         }
//     }
//     else {
//         printf("Decoded object is not a map.\n");
//     }

//     // cleanup
//     msgpack_unpacked_destroy(&msg);
// }

// // int main() {
// //     char username[50];
// //     char password[50];
// //     printf("Enter username: ");
// //     scanf("%49s", username);
// //     printf("Enter password: ");
// //     scanf("%49s", password);

// //     // encode user name and password
// //     msgpack_sbuffer buffer;
// //     msgpack_sbuffer_init(&buffer);
// //     encode_account(&buffer, username, password);

// //     printf("Encoded data size: %zu bytes\n", buffer.size);
// //     struct Player player;
// //     player = decode_account(&buffer);
// //     // decode data to username and password
// //     printf("Decoded data: %s %s\n", player.fullname, player.password);

    
// //     return 0;
// // }