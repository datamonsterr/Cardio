#include "main.h"
#include "game.h"

RoomList *init_room_list(size_t capacity)
{
    RoomList *room_list = malloc(sizeof(RoomList));
    if (room_list == NULL)
    {
        return NULL;
    }
    room_list->rooms = malloc(capacity * sizeof(Room));
    if (room_list->rooms == NULL)
    {
        free(room_list);
        return NULL;
    }
    room_list->size = 0;
    room_list->capacity = capacity;
    return room_list;
}
int add_room(RoomList *room_list, Room room)
{
    if (room_list->size == room_list->capacity)
    {
        Room *new_rooms = realloc(room_list->rooms, room_list->capacity * 2 * sizeof(Room));
        if (new_rooms == NULL)
        {
            return -1;
        }
        room_list->rooms = new_rooms;
        room_list->capacity *= 2;
    }
    room_list->rooms[room_list->size] = room;
    room_list->size++;
    return 0;
}
int remove_room(RoomList *room_list, int id)
{
    int index = find_room_by_id(room_list, id);
    if (index == -1)
    {
        return -1;
    }
    for (int i = index; i < room_list->size - 1; i++)
    {
        room_list->rooms[i] = room_list->rooms[i + 1];
    }
    room_list->size--;
    return 0;
}
Room *find_room_by_id(RoomList *room_list, int id)
{
    for (int i = 0; i < room_list->size; i++)
    {
        if (room_list->rooms[i].id == id)
        {
            return &room_list->rooms[i];
        }
    }
    return NULL;
}

void free_room_list(RoomList *room_list)
{
    free(room_list->rooms);
    free(room_list);
}