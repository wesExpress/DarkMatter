#include "dm_map.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_string.h"
#include <stdint.h>
#include <string.h>

#define DM_MAP_DEFAULT_CAPACITY 16
#define DM_MAP_RESIZE_FACTOR 2
#define DM_MAP_LOAD_FACTOR 0.75

// forward declare "private functions"
void dm_map_resize(dm_map_t* map);
dm_map_item* dm_map_create_item(const char* key, void* value, size_t type_size);
void dm_map_destroy_item(dm_map_item* item, size_t type_size);

// just add the characters together
uint32_t dm_map_simple_hash_function(dm_map_t* map, char* key)
{
	uint32_t hash = 0;
	for (int i = 0; key[i]; i++)
	{
		hash += key[i];
	}

	return hash % map->capacity;
}

// FNV-1a algorithm
uint64_t dm_map_hash(const char* key, dm_map_t* map)
{
	uint64_t hash = 14695981039346656037UL;
	for (int i = 0; key[i]; i++)
	{
		hash ^= key[i];
		hash *= 1099511628211;
	}

	return hash % map->capacity;
}

dm_map_t* dm_map_create(size_t type_size, size_t capacity)
{
	dm_map_t* map = dm_alloc(sizeof(dm_map_t), DM_MEM_MAP);

	if (capacity == 0)capacity = DM_MAP_DEFAULT_CAPACITY;
	map->capacity = capacity;
	map->type_size = type_size;

	map->items = dm_alloc(sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
	map->tombstones = dm_alloc(sizeof(bool) * map->capacity, DM_MEM_MAP);

	return map;
}

void dm_map_destroy(dm_map_t* map)
{
	if(map)
	{
		for (int i = 0; i < map->capacity; i++)
		{
			// items
			if (map->items[i]) dm_map_destroy_item(map->items[i], map->type_size);
		}
		dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
		dm_free(map->tombstones, sizeof(bool) * map->capacity, DM_MEM_MAP);
		dm_free(map, sizeof(dm_map_t), DM_MEM_MAP);
	}
	else
	{
		DM_LOG_ERROR("Trying to destroy NULL map!");
	}
}

void dm_map_insert(dm_map_t* map, const char* key, void* value)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);
		uint32_t hash = index;

		while(map->items[index] || map->tombstones[index])
		{
			if (map->items[index])
			{
				if (strcmp(map->items[index]->key, key) == 0)
				{
					map->items[index]->value = value;
					break;
				}
			}
			else
			{
				if(index >= map->capacity) index=0;
				index++;
			}
		}

		// insert element
		map->items[index] = dm_map_create_item(key, value, map->type_size);
		map->count++;
		if(index==hash) map->tombstones[index] = false;
		if(( (float)map->count / (float)map->capacity) >= DM_MAP_LOAD_FACTOR) dm_map_resize(map);
		
		// attach its next node
		uint32_t runner = index + 1;
		while (runner++)
		{
			if (map->items[runner])
			{
				map->items[index]->next = map->items[runner];
			}
			if (runner > map->capacity) break;
		}

		// deal with the head
		runner = 0;
		while (runner++)
		{
			if (map->items[runner] == map->head) break;
		}
		if (index < runner)map->head = map->items[index];
	}
	else DM_LOG_ERROR("Trying to insert into NULL map!");
}

void dm_map_delete_elem(dm_map_t* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);

		while(map->items[index])
		{
			if (strcmp(map->items[index]->key, key) == 0)
			{
				dm_map_destroy_item(map->items[index], map->type_size);
				map->tombstones[index] = true;
				map->count--;

				return;
			}

			index++;
			if(index >= map->capacity) index=0;
		}
		DM_LOG_WARN("Trying to delete invalid index from map.");
	}
	else DM_LOG_ERROR("Trying to delete from NULL map!");
}

void* dm_map_get(dm_map_t* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);

		while(map->items[index] || map->tombstones[index])
		{
			if (map->items[index])
			{
				if (strcmp(map->items[index]->key, key) == 0) 
					return map->items[index]->value;
			}
			
			index++;
			if(index>=map->capacity) index=0;
		}
	}
	else DM_LOG_ERROR("Map is NULL! Returning NULL...");

	// not found;
	return NULL;
}

bool dm_map_exists(dm_map_t* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);

		while(map->items[index] || map->tombstones[index])
		{
			if (map->items[index])
			{
				if (strcmp(map->items[index]->key, key) == 0) return true;
			}
			
			if(index>=map->capacity) index = 0;

			index++;
		}
	}
	else DM_LOG_ERROR("Map is NULL! Returning false...");
	
	return false;
}

void dm_map_resize(dm_map_t* map)
{
	dm_map_t* new_map = dm_map_create(map->type_size, map->capacity * DM_MAP_RESIZE_FACTOR);

	for (int i = 0; i < map->capacity; i++)
	{
		if (map->items[i])
		{
			dm_map_insert(new_map, map->items[i]->key, map->items[i]->value);
			dm_map_destroy_item(map->items[i], map->type_size);
		}
	}
	
	dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
	*map = *new_map;
}

dm_map_item* dm_map_create_item(const char* key, void* value, size_t type_size)
{
	dm_map_item* item = dm_alloc(sizeof(dm_map_item), DM_MEM_MAP);
	item->key = dm_strdup(key);
	item->value = dm_alloc(type_size, DM_MEM_MAP);
	dm_memcpy(item->value, value, type_size);

	return item;
}

void dm_map_destroy_item(dm_map_item* item, size_t type_size)
{
	dm_strdel(item->key);
	dm_free(item->value, type_size, DM_MEM_MAP);
	dm_free(item, sizeof(dm_map_item), DM_MEM_MAP);
}