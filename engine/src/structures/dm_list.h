#ifndef __DM_LIST_H__
#define __DM_LIST_H__

#include "core/dm_defines.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct dm_list
{
    size_t capacity;
    size_t count;
    size_t element_size;
    void* data;
} dm_list;

DM_API dm_list* dm_list_create(size_t element_size, size_t capacity);
DM_API void dm_list_destroy(dm_list* list);

/*
append a value to end of list
*/
DM_API void dm_list_append(dm_list* list, void* value);

/*
insert a value into list at specified index
*/
DM_API void dm_list_insert(dm_list* list, void* value, uint32_t index);

/*
set a value at a specific index
*/
DM_API void dm_list_set(dm_list* list, void* value, uint32_t index);

/*
delete an element from end of list
*/
DM_API void dm_list_pop(dm_list* list);

/*
delete an element from list at a specified index
*/
DM_API void dm_list_pop_at(dm_list* list, uint32_t index);

/*
returns value from a list at specified index.
It is the user's responsibility to cast this to whatever it actually is!
*/
DM_API void* dm_list_at(dm_list* list, uint32_t index);

/*
clears a list and resets capacity to specified amount. If 0, then 16 elements
*/
DM_API void dm_list_clear(dm_list* list, size_t new_capacity);

DM_API bool dm_list_is_empty(dm_list* list);

#define dm_for_list_item(LIST, TYPE, ITEM)\
uint32_t i=0;\
TYPE* ITEM = dm_list_at(LIST, i);\
for(i=0, ITEM=dm_list_at(LIST,i); i<(LIST)->count; ITEM=dm_list_at(LIST,++i))\

#endif