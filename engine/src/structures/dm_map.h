#ifndef __DM_MAP_H__
#define __DM_MAP_H__

#include <stdlib.h>
#include <stdbool.h>
#include "dm_list.h"

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
    dm_map_item* begin;
    dm_map_item* end;
} dm_map;

dm_map* dm_map_create(size_t type_size, size_t capacity);
void dm_map_destroy(dm_map* map);

/*
special function to specifically destroy a map with list items
*/
void dm_map_list_destroy(dm_map* map);

/*
Insert an element using linear probing
*/
void dm_map_insert(dm_map* map, const char* key, void* value);

/*
Special function to specifically insert lists into a map
*/
void dm_map_insert_list(dm_map* map, const char* key, dm_list* list);

void dm_map_delete_elem(dm_map* map, const char* key);
/*
It is the user's responsibility to cast this to whatever it actually is! Returns NULL if not found
*/
void* dm_map_get(dm_map* map, const char* key);

bool dm_map_exists(dm_map* map, const char* key);

#endif