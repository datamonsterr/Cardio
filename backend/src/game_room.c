#include "main.h"
#include "game.h"

TableList *init_table_list(size_t capacity)
{
    TableList *table_list = malloc(sizeof(TableList));
    if (table_list == NULL)
    {
        return NULL;
    }
    table_list->tables = malloc(capacity * sizeof(Table));
    if (table_list->tables == NULL)
    {
        free(table_list);
        return NULL;
    }
    table_list->size = 0;
    table_list->capacity = capacity;
    return table_list;
}
int add_table(TableList *table_list, char *table_name, int max_player, int min_bet)
{

    if (table_list->size == table_list->capacity)
    {
        TableList *new_table_list = realloc(table_list->tables, 2 * table_list->capacity * sizeof(Table));
        if (new_table_list == NULL)
        {
            logger(MAIN_LOG, "Error", "add_table: Cannot allocate memory for table list");
            return -1;
        }
        table_list->tables = (Table *)new_table_list;
        table_list->capacity *= 2;
    }

    int id = table_list->size + 1;
    while (find_table_by_id(table_list, id) != -1)
    {
        id++;
    }
    table_list->tables[table_list->size].id = id;
    strncpy(table_list->tables[table_list->size].name, table_name, 32);
    table_list->tables[table_list->size].max_player = max_player;
    table_list->tables[table_list->size].min_bet = min_bet;
    table_list->tables[table_list->size].current_player = 0;
    table_list->size++;
    return id;
}
int remove_table(TableList *table_list, int id)
{
    int index = find_table_by_id(table_list, id);
    if (index == -1)
    {
        return -1;
    }
    for (int i = index; i < table_list->size - 1; i++)
    {
        table_list->tables[i] = table_list->tables[i + 1];
    }
    free(&table_list->tables[table_list->size - 1]);
    table_list->size--;
    return 0;
}

int find_table_by_id(TableList *table_list, int id)
{
    for (int i = 0; i < table_list->size; i++)
    {
        if (table_list->tables[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

int join_table(conn_data_t *conn_data, TableList *table_list, int table_id)
{
    int index = find_table_by_id(table_list, table_id);
    if (index == -1)
    {
        logger(MAIN_LOG, "Error", "join_table: Table not found");
        return -1;
    }
    if (table_list->tables[index].current_player == table_list->tables[index].max_player)
    {
        logger(MAIN_LOG, "Error", "join_table: Table is full");
        return -2;
    }
    table_list->tables[index].current_player++;
    if (conn_data->table_id != 0)
    {
        logger(MAIN_LOG, "Error", "join_table: Player is already at a table");
        return -3;
    }
    conn_data->table_id = table_id;

    char *msg;
    sprintf(msg, "join_table: Player %s joined table %d", conn_data->username, table_id);
    logger(MAIN_LOG, "Info", msg);
    return index;
}

int leave_table(conn_data_t *conn_data, TableList *table_list)
{
    int index = find_table_by_id(table_list, conn_data->table_id);
    if (index == -1)
    {
        return -1;
    }
    int current_player = table_list->tables[index].current_player;
    if (current_player == 0)
    {
        remove_table(table_list, conn_data->table_id);
        return -2;
    }
    table_list->tables[index].current_player--;
    conn_data->table_id = 0;
    return index;
}

void free_table_list(TableList *table_list)
{
    free(table_list->tables);
    free(table_list);
}