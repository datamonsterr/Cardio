#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generic_map.h"

// Create a new map
Map *map_create(CompareFunc compare, FreeFunc free_key, FreeFunc free_value)
{
    Map *map = (Map *)malloc(sizeof(Map));
    map->root = NULL;
    map->compare = compare;
    map->free_key = free_key;
    map->free_value = free_value;
    return map;
}

// Helper function to create a new entry
MapEntry *create_entry(void *key, void *value)
{
    MapEntry *entry = (MapEntry *)malloc(sizeof(MapEntry));
    entry->key = key;
    entry->value = value;
    entry->left = entry->right = NULL;
    return entry;
}

// Recursive helper for insertion
MapEntry *insert_entry(MapEntry *node, void *key, void *value, CompareFunc compare)
{
    if (node == NULL)
    {
        return create_entry(key, value);
    }
    int cmp = compare(key, node->key);
    if (cmp < 0)
    {
        node->left = insert_entry(node->left, key, value, compare);
    }
    else if (cmp > 0)
    {
        node->right = insert_entry(node->right, key, value, compare);
    }
    else
    {
        // Update existing value
        free(node->value);
        node->value = value;
    }
    return node;
}

// Insert a key-value pair into the map
void map_insert(Map *map, void *key, void *value)
{
    map->root = insert_entry(map->root, key, value, map->compare);
}

// Recursive helper for searching
void *search_entry(MapEntry *node, void *key, CompareFunc compare)
{
    if (node == NULL)
    {
        return NULL;
    }
    int cmp = compare(key, node->key);
    if (cmp < 0)
    {
        return search_entry(node->left, key, compare);
    }
    else if (cmp > 0)
    {
        return search_entry(node->right, key, compare);
    }
    else
    {
        return node->value;
    }
}

// Search for a value by key
void *map_search(Map *map, void *key)
{
    return search_entry(map->root, key, map->compare);
}

// Recursive helper to find the minimum value in a subtree
MapEntry *find_min(MapEntry *node)
{
    while (node->left != NULL)
    {
        node = node->left;
    }
    return node;
}

// Recursive helper for deletion
MapEntry *remove_entry(MapEntry *node, void *key, CompareFunc compare, FreeFunc free_key, FreeFunc free_value)
{
    if (node == NULL)
    {
        return NULL;
    }
    int cmp = compare(key, node->key);
    if (cmp < 0)
    {
        node->left = remove_entry(node->left, key, compare, free_key, free_value);
    }
    else if (cmp > 0)
    {
        node->right = remove_entry(node->right, key, compare, free_key, free_value);
    }
    else
    {
        // Key found, handle deletion
        if (node->left == NULL)
        {
            MapEntry *temp = node->right;
            if (free_key)
                free_key(node->key);
            if (free_value)
                free_value(node->value);
            free(node);
            return temp;
        }
        else if (node->right == NULL)
        {
            MapEntry *temp = node->left;
            if (free_key)
                free_key(node->key);
            if (free_value)
                free_value(node->value);
            free(node);
            return temp;
        }
        else
        {
            // Replace with in-order successor
            MapEntry *temp = find_min(node->right);
            if (free_key)
                free_key(node->key);
            node->key = temp->key;
            node->value = temp->value;
            node->right = remove_entry(node->right, temp->key, compare, free_key, free_value);
        }
    }
    return node;
}

// Remove a key-value pair
void map_remove(Map *map, void *key)
{
    map->root = remove_entry(map->root, key, map->compare, map->free_key, map->free_value);
}

// Recursive helper to destroy a map
void destroy_entries(MapEntry *node, FreeFunc free_key, FreeFunc free_value)
{
    if (node != NULL)
    {
        destroy_entries(node->left, free_key, free_value);
        destroy_entries(node->right, free_key, free_value);
        if (free_key)
            free_key(node->key);
        if (free_value)
            free_value(node->value);
        free(node);
    }
}

// Destroy the map
void map_destroy(Map *map)
{
    destroy_entries(map->root, map->free_key, map->free_value);
    free(map);
}