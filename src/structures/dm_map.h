#ifndef __DM_MAP_H__
#define __DM_MAP_H__

#define CAPACITY 50000

typedef struct dm_map_item
{
    char* key;
    char* value;
} dm_map_item;

typedef struct dm_map
{
    int size, count;
    dm_map_item** items;
} dm_map;

unsigned long simple_hash_function(const char* str);
dm_map* dm_create_map(int size);
dm_map_item* dm_create_map_item(char* key, char* value);
void dm_map_delete_item(dm_map_item* item);
void dm_map_delete(dm_map* map);

void dm_map_insert(dm_map* map, char* key, char* value);
char* dm_map_search(dm_map* map, char* key);

void dm_map_print(dm_map* map);
void dm_map_search_print(dm_map* map, char* key);
#endif