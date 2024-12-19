#pragma once
#include <stddef.h>

// Type for the comparison function
typedef int (*CompareFunc)(const void *a, const void *b);

// Type for custom key/value cleanup
typedef void (*FreeFunc)(void *data);

// Map entry structure
typedef struct MapEntry
{
    void *key;
    void *value;
    struct MapEntry *left;
    struct MapEntry *right;
} MapEntry;

// Map structure
typedef struct Map
{
    MapEntry *root;      // Root of the BST
    CompareFunc compare; // Function to compare keys
    FreeFunc free_key;   // Function to free keys
    FreeFunc free_value; // Function to free values
} Map;

// Function prototypes
Map *map_create(CompareFunc compare, FreeFunc free_key, FreeFunc free_value);
void map_insert(Map *map, void *key, void *value);
void *map_search(Map *map, void *key);
void map_remove(Map *map, void *key);
void map_destroy(Map *map);
