#ifndef __DM_MAP_H__
#define __DM_MAP_H__

#include <stdlib.h>
#include <stdbool.h>
#include "dm_list.h"

typedef enum dm_map_key_type
{
    DM_MAP_KEY_STRING,
    DM_MAP_KEY_UINT32,
    DM_MAP_KEY_UINT64,
    DM_MAP_KEY_UNKNOWN
} dm_map_key_type;

typedef struct dm_map_item
{
	void* key;
	void* value;
    struct dm_map_item* next;
    struct dm_map_item* prev;
} dm_map_item;

typedef struct dm_map_t
{
	size_t capacity, count, type_size;
	dm_map_item** items;
    dm_map_key_type key_type;
    dm_map_item* begin;
    dm_map_item* end;
} dm_map;

/*
create a map with a specified size. If 0, then default size
*/
dm_map* dm_map_create(dm_map_key_type key_type, size_t type_size, size_t capacity);

/*
destroys a map
*/
void dm_map_destroy(dm_map* map);

/*
special function to specifically destroy a map with list items
*/
void dm_map_list_destroy(dm_map* map);

/*
Insert an element using linear probing
*/
void dm_map_insert(dm_map* map, void* key, void* value);

/*
Special function to specifically insert lists into a map
*/
void dm_map_insert_list(dm_map* map, void* key, dm_list* list);

/*
delete a specific element with a key
*/
void dm_map_delete_elem(dm_map* map, void* key);

/*
It is the user's responsibility to cast this to whatever it actually is! Returns NULL if not found
*/
void* dm_map_get(dm_map* map, void* key);

/*
checks if key exists in the map
*/
bool dm_map_exists(dm_map* map, void* key);

/*
checks if map is empty
*/
bool dm_map_is_empty(dm_map* map);

#endif