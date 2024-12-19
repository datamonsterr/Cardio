#include "main.h"

int string_compare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

// how to decode messagepack data and return a map of the data
Map *decode_msgpack(const char *data, size_t len)
{
    mpack_tree_t tree;
    mpack_tree_init_data(&tree, data, len);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    Map *map = map_create(strcmp, free, free);
    if (mpack_node_type(root) != mpack_type_map)
    {
        return map;
    }

    size_t size = mpack_node_map_count(root);
    for (size_t i = 0; i < size; i++)
    {
        mpack_node_t key = mpack_node_map_key_at(root, i);
        mpack_node_t val = mpack_node_map_value_at(root, i);

        if (mpack_node_type(key) != mpack_type_str)
        {
            continue;
        }

        void *key_str = (void *)mpack_node_str(key);
        if (mpack_node_type(val) == mpack_type_str)
        {
            const char *val_str = mpack_node_str(val);
            map_insert(map, key_str, (void *)val_str);
        }
        else if (mpack_node_type(val) == mpack_type_int)
        {
            int val_int = mpack_node_i32(val);
            map_insert(map, key_str, (void *)&val_int);
        }
        else if (mpack_node_type(val) == mpack_type_float)
        {
            float val_float = mpack_node_float(val);
            map_insert(map, key_str, (void *)&val_float);
        }
        else if (mpack_node_type(val) == mpack_type_double)
        {
            double val_double = mpack_node_double(val);
            map_insert(map, key_str, (void *)&val_double);
        }
        else if (mpack_node_type(val) == mpack_type_bool)
        {
            bool val_bool = mpack_node_bool(val);
            map_insert(map, key_str, (void *)&val_bool);
        }
    }

    mpack_tree_destroy(&tree);
    return map;
}