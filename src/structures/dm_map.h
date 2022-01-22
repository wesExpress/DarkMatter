#ifndef __DM_MAP_H__
#define __DM_MAP_H__

#define DM_MAP_DEFAULT_SIZE 16

#include <stdlib.h>
#include <stdbool.h>

typedef struct dm_map_item
{
	char* key;
	void* value;
} dm_map_item;

typedef struct dm_map_t
{
	size_t capacity, count, type_size;
	dm_map_item** items;
} dm_map_t;

dm_map_t* dm_map_create(size_t type_size, size_t capacity);
void dm_map_destroy(dm_map_t* map);

/*
Insert an element using linear probing
*/
void dm_map_insert(dm_map_t* map, char* key, void* value);
void dm_map_delete_elem(dm_map_t* map, char* key);
/*
It is the user's responsibility to cast this to whatever it actually is! Returns NULL if not found
*/
void* dm_map_get(dm_map_t* map, char* key);

bool dm_map_exists(dm_map_t* map, char* key);

#endif