#ifndef __DM_SLOT_LIST_H__
#define __DM_SLOT_LIST_H__

#include "core/dm_defines.h"
#include "structures/dm_list.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct dm_slot_list
{
    size_t capacity;
    size_t count;
    size_t element_size;
    uint16_t* indices;
    void* data;
} dm_slot_list;

DM_API dm_slot_list* dm_slot_list_create(size_t element_size, size_t capacity);
DM_API void dm_slot_list_destroy(dm_slot_list* list);

/*
finds first open slot and inserts value, changes index to found index
*/
DM_API void dm_slot_list_insert(dm_slot_list* list, void* value, uint32_t* index);

/*
*/
DM_API void dm_slot_list_delete(dm_slot_list* list, uint32_t index);

/*
returns value from a slot list at specified index.
*/
DM_API void* dm_slot_list_at(dm_slot_list* list, uint32_t index);

/*
clears a list and resets capacity to specified amount. If 0, then 16 elements
*/
DM_API void dm_slot_list_clear(dm_slot_list* list, size_t new_capacity);

DM_API bool dm_slot_list_is_empty(dm_slot_list* list);

#define dm_for_slot_list_item(LIST, TYPE, ITEM)\
uint32_t i=0;\
TYPE* ITEM = dm_list_at(LIST, i);\
for(i=0, ITEM=dm_list_at(LIST,i); i<(LIST)->count; ITEM=dm_list_at(LIST,++i))\


#endif 
