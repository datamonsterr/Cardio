#pragma once
#include <stdlib.h>

struct
{
    char name[32];
    int id;
    int current_player;
    int max_player;
    int min_bet;
    int max_bet;
} typedef Room;

typedef struct
{
    Room *rooms;     // Dynamic array of rooms
    size_t size;     // Current number of rooms
    size_t capacity; // Maximum capacity before resizing
} RoomList;

RoomList *init_room_list(size_t capacity);        // returns pointer to RoomList, NULL on failure
int add_room(RoomList *room_list, Room room);     // returns 0 on success, -1 on failure
int remove_room(RoomList *room_list, int id);     // returns 0 on success, -1 on failure
int find_room_by_id(RoomList *room_list, int id); // returns index of room in list, -1 if not found
void free_room_list(RoomList *room_list);