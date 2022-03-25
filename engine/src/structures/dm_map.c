#include "dm_map.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "core/dm_string.h"
#include "core/dm_hash.h"

#include <stdint.h>
#include <string.h>

#define DM_MAP_DEFAULT_CAPACITY 16
#define DM_MAP_RESIZE_FACTOR 2
#define DM_MAP_LOAD_FACTOR 0.75

// forward declare "private functions"
void dm_map_resize(dm_map* map);
dm_map_item* dm_map_create_item(const char* key, void* value, size_t type_size);
void dm_map_destroy_item(dm_map_item* item, size_t type_size);

// just add the characters together
dm_hash dm_map_simple_hash_function(dm_map* map, char* key)
{
	uint32_t hash = 0;
	for (int i = 0; key[i]; i++)
	{
		hash += key[i];
	}
    
	return hash % map->capacity;
}

// FNV-1a algorithm
dm_hash dm_map_hash(const char* key, dm_map* map)
{
	dm_hash hash = dm_hash_fnv1a(key);
	return hash % map->capacity;
}

dm_map* dm_map_create(size_t type_size, size_t capacity)
{
	dm_map* map = dm_alloc(sizeof(dm_map), DM_MEM_MAP);
    
	if (capacity == 0)capacity = DM_MAP_DEFAULT_CAPACITY;
	map->capacity = capacity;
	map->type_size = type_size;
    
	map->items = dm_alloc(sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
    
    map->begin = NULL;
    map->end = NULL;
    
	return map;
}

void dm_map_destroy(dm_map* map)
{
	if(map)
	{
		for (int i = 0; i < map->capacity; i++)
		{
			// items
			if (map->items[i]) dm_map_destroy_item(map->items[i], map->type_size);
		}
		dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
		dm_free(map, sizeof(dm_map), DM_MEM_MAP);
	}
	else
	{
		DM_LOG_ERROR("Trying to destroy NULL map!");
	}
}

void dm_map_list_destroy(dm_map* map)
{
	if(map)
	{
		for(uint32_t i=0;i<map->capacity;i++)
		{
			if(map->items[i])
			{
				dm_list* list = map->items[i]->value;
				dm_list_destroy(list);
                
				dm_strdel(map->items[i]->key);
				dm_free(map->items[i], sizeof(dm_map_item), DM_MEM_MAP);
			}
		}
		dm_free(map->items, sizeof(dm_map_item*) * map->capacity, DM_MEM_MAP);
		dm_free(map, sizeof(dm_map), DM_MEM_MAP);
	}
	else DM_LOG_ERROR("Trying to destroy NULL map!");
}

void dm_map_insert(dm_map* map, const char* key, void* value)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);
		dm_hash hash = index;
        
		while(map->items[index])
		{
			if (map->items[index])
			{
				if (strcmp(map->items[index]->key, key) == 0)
				{
					map->items[index]->value = value;
					return;
				}
			}
            if(index >= map->capacity) index=0;
            index++;
        }
        
        // insert element
        dm_map_item* item = dm_map_create_item(key, value, map->type_size);
        
        // start of map chain
        if(!map->begin) map->begin = item;
        
        // link items into chain for iteration
        if(map->end)
        {
            map->end->next = item;
            item->prev = map->end;
        }
        map->end = item;
        
        map->items[index] = item;
        map->count++;
        if(( (float)map->count / (float)map->capacity) >= DM_MAP_LOAD_FACTOR) dm_map_resize(map);
		
	}
	else DM_LOG_ERROR("Trying to insert into NULL map!");
}

void dm_map_insert_list(dm_map* map, const char* key, dm_list* list)
{
	if(map)
	{
		if(list)
		{
			uint32_t index = dm_map_hash(key, map);
			dm_hash hash = index;
			bool replace = false;
            
			while(map->items[index])
			{
				if(map->items[index] && strcmp(map->items[index]->key, key) == 0)
				{
					dm_list* old_list = map->items[index]->value;
					dm_list_destroy(old_list);
					
					map->items[index]->value = dm_list_create(list->element_size, list->capacity);
					dm_list* new_list = map->items[index]->value;
					new_list->count = list->count;
					dm_memcpy(new_list->data, list->data, list->capacity * list->element_size);
					
					replace = true;
                    
					break;
				}
				else
				{
					if(index >= map->capacity) index=0;
					index++;
				}
			}
            
			if(!replace)
			{
				map->items[index] = dm_alloc(sizeof(dm_map_item), DM_MEM_MAP);
				map->items[index]->key = dm_strdup(key);
				map->items[index]->value = dm_list_create(list->element_size, list->capacity);
				
				dm_list* map_list = map->items[index]->value;
				map_list->count = list->count;
				dm_memcpy(map_list->data, list->data, list->capacity * list->element_size);
                
				map->count++;
				if(( (float)map->count / (float)map->capacity) >= DM_MAP_LOAD_FACTOR) dm_map_resize(map);
			}
		}
		else DM_LOG_ERROR("Trying to insert NULL list into map!");
	}
	else DM_LOG_ERROR("Trying to insert list into NULL map!");
}

void dm_map_delete_elem(dm_map* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);
        
		while(map->items[index])
		{
			if (strcmp(map->items[index]->key, key) == 0)
			{
                dm_map_item* item = map->items[index];
                
                // re-link the chain
                if(item == map->begin) map->begin = item->next;
                if(item == map->end) map->end = item->prev;
                if(item->prev) item->prev->next = item->next;
                if(item->next) item->next->prev = item->prev;
                
				dm_map_destroy_item(map->items[index], map->type_size);
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

void* dm_map_get(dm_map* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);
        
		while(map->items[index])
		{
			if (map->items[index])
			{
				if (strcmp(map->items[index]->key, key) == 0) return map->items[index]->value;
			}
			
			index++;
			if(index>=map->capacity) index=0;
		}
	}
	else DM_LOG_ERROR("Map is NULL! Returning NULL...");
    
	// not found;
	return NULL;
}

bool dm_map_exists(dm_map* map, const char* key)
{
	if(map)
	{
		uint32_t index = dm_map_hash(key, map);
        
		while(map->items[index])
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

void dm_map_resize(dm_map* map)
{
	dm_map* new_map = dm_map_create(map->type_size, map->capacity * DM_MAP_RESIZE_FACTOR);
    
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
    
    item->next = NULL;
    item->prev = NULL;
    
	return item;
}

void dm_map_destroy_item(dm_map_item* item, size_t type_size)
{
	dm_strdel(item->key);
	dm_free(item->value, type_size, DM_MEM_MAP);
	dm_free(item, sizeof(dm_map_item), DM_MEM_MAP);
}