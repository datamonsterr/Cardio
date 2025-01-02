#pragma once
#include "main.h"

struct
{
    char name[32];
    int id;
    int current_player;
    int max_player;
    int min_bet;
    int max_bet;
} typedef Table;

typedef struct
{
    Table *tables;   // Dynamic array of rooms
    size_t size;     // Current number of rooms
    size_t capacity; // Maximum capacity before resizing
} TableList;

TableList *init_table_list(size_t capacity); // returns pointer to TableList, NULL on failure
int add_table(TableList *table_list, char *table_name, int max_player, int min_bet);
int remove_table(TableList *table_list, int id);     // returns 0 on success, -1 on failure
int find_table_by_id(TableList *table_list, int id); // returns index of room in list, -1 if not found
void free_table_list(TableList *table_list);
