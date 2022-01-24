#ifndef __DM_LIST_H__
#define __DM_LIST_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define DM_LIST_DEFAULT_COUNT 16
#define dm_list(TYPE) TYPE*

void* dm_list_init(size_t element_size, size_t capacity);
void dm_list_destroy(void* list);

void dm_list_append(void* list, void* value);
void dm_list_insert(void* list, void* value, uint32_t index);

void dm_list_pop(void* list);
void dm_list_pop_at(void* list, uint32_t index);

void dm_list_clear(void* list);

bool dm_list_is_empty(void* list);

size_t dm_list_get_count(void* list);
size_t dm_list_get_capacity(void* list);

#endif