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
int add_table(TableList *table_list, Table table)
{
    if (table_list->size == table_list->capacity)
    {
        Table *new_tables = realloc(table_list->tables, table_list->capacity * 2 * sizeof(Table));
        if (new_tables == NULL)
        {
            return -1;
        }
        table_list->tables = new_tables;
        table_list->capacity *= 2;
    }
    table_list->tables[table_list->size] = table;
    table_list->size++;
    return 0;
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

void free_table_list(TableList *table_list)
{
    free(table_list->tables);
    free(table_list);
}