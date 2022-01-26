#ifndef __DM_LIST_H__
#define __DM_LIST_H__

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

dm_list* dm_list_create(size_t element_size, size_t capacity);
void dm_list_destroy(dm_list* list);

/*
append a value to end of list
*/
void dm_list_append(dm_list* list, void* value);

/*
insert a value into list at specified index
*/
void dm_list_insert(dm_list* list, void* value, uint32_t index);

/*
delete an element from end of list
*/
void dm_list_pop(dm_list* list);

/*
delete an element from list at a specified index
*/
void dm_list_pop_at(dm_list* list, uint32_t index);

/*
returns value from a list at specified index.
It is the user's responsibility to cast this to whatever it actually is!
*/
void* dm_list_at(dm_list* list, uint32_t index);

/*
clears a list and resets capacity to specified amount. If 0, then 16 elements
*/
void dm_list_clear(dm_list* list, size_t new_capacity);

bool dm_list_is_empty(dm_list* list);

#endif