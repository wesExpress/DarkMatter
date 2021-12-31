#include "dm_map.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "string.h"

unsigned long simple_hash_function(const char* str)
{
    unsigned long index = 0;
    for (int i=0; str[i]; i++)
    {
        index += str[i];
    }
    return index % CAPACITY;
}

dm_map* dm_create_map(int size)
{
    dm_map* map = (dm_map*)dm_alloc(sizeof(dm_map));
    map->size = size;
    map->count = 0;
    map->items = (dm_map_item**)calloc(map->size, sizeof(dm_map_item*));
    for (int i=0; i<size;i++) map->items[i] = NULL;
    return map;
}

dm_map_item* dm_create_map_item(char* key, char* value)
{
    dm_map_item* item = (dm_map_item*)dm_alloc(sizeof(dm_map_item));
    item->key = (char*)dm_alloc(strlen(key)+1);
    item->value = (char*)dm_alloc(strlen(value)+1);

    strcpy(item->key, key);
    strcpy(item->value, value);

    return item;
}

void dm_map_delete_item(dm_map_item* item)
{
    dm_free(item->key);
    dm_free(item->value);
    dm_free(item);
}

void dm_map_delete(dm_map* map)
{
    for(int i=0; i<map->size; i++)
    {
        if(map->items[i]) dm_map_delete_item(map->items[i]);
    }

    dm_free(map->items);
    dm_free(map);
}

void dm_map_insert(dm_map* map, char* key, char* value)
{
    int index = simple_hash_function(key);

    if(!map->items[index])
    {
        if(map->count == map->size)
        {
            DM_LOG_ERROR("Trying to insert into full map!");
            return;
        }

         dm_map_item* item = dm_create_map_item(key, value);
         map->items[index] = item;
         map->count++;
    }
    else
    {
        if(strcmp(map->items[index]->key, key) == 0)
        {
            strcpy(map->items[index]->value, value);
            return;
        }
        else
        {
            return;
        }
    }
}

char* dm_map_search(dm_map* map, char* key)
{
    int index = simple_hash_function(key);

    if(map->items[index])
    {
        if(strcmp(map->items[index]->key, key) == 0) return map->items[index]->value;
    }
    return NULL;
}

void dm_map_print(dm_map* map)
{
    for(int i =0; i<map->size; i++)
    {
        if(map->items[i])
        {
            DM_LOG_DEBUG("Index: %d; Key: %s; Value: %s", i, map->items[i]->key, map->items[i]->value);
        }
    }
}

void dm_map_search_print(dm_map* map, char* key)
{
    char* val;
    if((val = dm_map_search(map, key)) == NULL) DM_LOG_DEBUG("Key not found.");
    else DM_LOG_DEBUG("Key: %s; Value: %s", key, val);
}