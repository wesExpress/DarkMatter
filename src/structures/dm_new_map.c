#include "dm_new_map.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include <string.h>
#include <stdint.h>

#define DM_MAP_RESIZE_FACTOR 2
#define DM_MAP_LOAD_FACTOR 0.75

// forward declare "private functions"
void dm_map_resize(dm_map_t* map);
dm_map_item* dm_map_create_item(char* key, void* value, size_t type_size);
void dm_map_destroy_item(dm_map_item* item, dm_map_t* map);
void dm_map_handle_collision(dm_map_t* map, uint32_t index, dm_map_item* item);
dm_map_link_list* dm_map_list_insert(dm_map_link_list* list, dm_map_item* item);
dm_map_link_list* dm_map_list_create(dm_map_item* item);

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
uint32_t dm_map_hash(const char* key, dm_map_t* map)
{
	uint32_t hash = 14695981039346656037;
	for (int i = 0; key[i]; i++)
	{
		hash ^= key[i];
		hash *= 1099511628211;
	}

	return hash % map->capacity;
}

dm_map_t* dm_map_create(size_t type_size, size_t capacity)
{
	dm_map_t* map = (dm_map_t*)dm_alloc(sizeof(dm_map_t), DM_MEM_MAP);

	map->capacity = capacity;
	map->type_size = type_size;

	map->items = (dm_map_item**)dm_alloc(sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
	map->overflow_buckets = (dm_map_link_list**)dm_alloc(sizeof(dm_map_link_list*) * map->capacity, DM_MEM_MAP);

	return map;
}

void dm_map_destroy(dm_map_t* map)
{
	for (int i = 0; i < map->capacity; i++)
	{
		// items
		if (map->items[i]) dm_map_destroy_item(map->items[i], map);

		// overflow buckets
		if (map->overflow_buckets[i])
		{
			dm_map_link_list* temp = map->overflow_buckets[i];
			dm_map_link_list* runner = map->overflow_buckets[i];

			while (runner)
			{
				temp = runner;
				runner = runner->next;

				dm_map_destroy_item(temp->item, map);
				dm_free(temp, sizeof(dm_map_link_list), DM_MEM_MAP);
			}
		}
	}
	dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
	dm_free(map->overflow_buckets, sizeof(dm_map_link_list*) * map->capacity, DM_MEM_MAP);
	
	// map itself
	dm_free(map, sizeof(dm_map_t), DM_MEM_MAP);
}

void dm_map_insert(dm_map_t* map, char* key, void* value)
{
	uint32_t index = dm_map_hash(key, map);

	if (!map->items[index])
	{
		if (((float)map->count / (float)map->capacity) > DM_MAP_LOAD_FACTOR)
		{
			dm_map_resize(map);
		}

		dm_map_item* item = dm_map_create_item(key, value, map->type_size);

		map->items[index] = item;
		map->count++;
	}
	else
	{
		//DM_LOG_ERROR("Haven't implemented map collisions yet!");
		if (strcmp(map->items[index]->key, key) == 0)
		{
			map->items[index]->value = value;
		}
		else
		{
			dm_map_item* item = dm_map_create_item(key, value, map->type_size);
			dm_map_handle_collision(map, index, item);
		}
	}
}

void dm_map_delete(dm_map_t* map, char* key)
{
	uint32_t index = dm_map_hash(key, map);

	// check the index right away
	if (map->items[index])
	{
		if (strcmp(map->items[index]->key, key) == 0)
		{
			dm_map_destroy_item(map->items[index], map);
			map->count--;
		}
	}

	// check the overflow buckets
	if (map->overflow_buckets[index])
	{
		dm_map_link_list* head = map->overflow_buckets[index];

		if (strcmp(head->item->key, key) == 0)
		{
			if(head->next) map->overflow_buckets[index] = head->next;

			dm_map_destroy_item(head->item, map);
		}

		// run through the overflow buckets
		if (head->next)
		{
			dm_map_link_list* runner = head;
			while (runner->next->next)
			{
				if (strcmp(runner->next->item->key, key) == 0)
				{
					head->next = runner->next;
					dm_map_destroy_item(runner->item, map);
					break;
				}

				runner = runner->next;
			}
		}
	}
}

void* dm_map_get(dm_map_t* map, char* key)
{
	uint32_t index = dm_map_hash(key, map);

	// check the index right away
	if (map->items[index])
	{
		if (strcmp(map->items[index]->key, key) == 0) return map->items[index]->value;
	}

	// check the overflow buckets
	if (map->overflow_buckets[index])
	{
		dm_map_link_list* head = map->overflow_buckets[index];

		if (strcmp(head->item->key, key) == 0) return head->item->value;

		// run through the overflow buckets
		if (head->next)
		{
			dm_map_link_list* runner = head;
			while (runner->next->next)
			{
				runner = runner->next;

				if (strcmp(runner->item->key, key) == 0) return runner->item->value;
			}
		}
	}

	// not found;
	return NULL;
}

bool dm_map_exists(dm_map_t* map, char* key)
{
	uint32_t index = dm_map_hash(key, map);

	// check the index right away
	if (map->items[index])
	{
		if (strcmp(map->items[index]->key, key) == 0) return true;
	}

	// check the overflow bucket
	if (map->overflow_buckets[index])
	{
		dm_map_link_list* head = map->overflow_buckets[index];

		if (strcmp(head->item->key, key) == 0) return true;

		// run through the overflow buckets
		if (head->next)
		{
			dm_map_link_list* runner = head;
			while (runner->next->next)
			{
				runner = runner->next;

				if (strcmp(runner->item->key, key) == 0) return true;
			}
		}
	}
	
	// not in the map
	return false;
}

void dm_map_resize(dm_map_t* map)
{
	dm_map_t* new_map = dm_map_create(map->type_size, map->capacity * DM_MAP_RESIZE_FACTOR);

	for (int i = 0; i < map->capacity; i++)
	{
		if (map->items[i]) dm_map_insert(new_map, map->items[i]->key, map->items[i]->value);
	}

	dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
	dm_free(map->overflow_buckets, sizeof(dm_map_link_list*) * map->capacity, DM_MEM_MAP);
	*map = *new_map;

	// also need to adjust memory database to refelct
	dm_mem_db_adjust(sizeof(dm_map_t), DM_MEM_MAP, DM_MEM_ADJUST_SUBTRACT);
}

dm_map_item* dm_map_create_item(char* key, void* value, size_t type_size)
{
	dm_map_item* item = dm_alloc(sizeof(dm_map_item), DM_MEM_MAP);
	item->key = strdup(key);
	item->value = value;

	return item;
}

void dm_map_destroy_item(dm_map_item* item, dm_map_t* map)
{
	dm_free(item->key, strlen(item->key) + 1, DM_MEM_MAP);
	dm_free(item, sizeof(dm_map_item), DM_MEM_MAP);
}

void dm_map_handle_collision(dm_map_t* map, uint32_t index, dm_map_item* item)
{
	dm_map_link_list* head = map->overflow_buckets[index];

	if (!head)
	{
		head = dm_map_list_create(item);
		map->overflow_buckets[index] = head;
	}
	else
	{
		map->overflow_buckets[index] = dm_map_list_insert(head, item);
	}
}

dm_map_link_list* dm_map_list_insert(dm_map_link_list* list, dm_map_item* item)
{
	if (!list)
	{
		dm_map_link_list* head = dm_map_list_create(item);
		list = head;
	}
	else if (!list->next)
	{
		dm_map_link_list* node = dm_map_list_create(item);
		list->next = node;
	}
	else
	{
		// find end of list to append to
		dm_map_link_list* runner = list;
		while (runner->next->next) runner = runner->next;

		// attach the new node
		dm_map_link_list* node = dm_map_list_create(item);
		runner->next = node;
	}

	return list;
}

dm_map_link_list* dm_map_list_create(dm_map_item* item)
{
	dm_map_link_list* node = (dm_map_link_list*)dm_alloc(sizeof(dm_map_link_list), DM_MEM_MAP);
	node->item = item;
	return node;
}