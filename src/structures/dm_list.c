#include "dm_list.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"

#define DM_LIST_DEFAULT_CAPACITY 16
#define DM_LIST_LOAD_FACTOR 0.75
#define DM_LIST_RESIZE_FACTOR 2

bool dm_list_should_grow(dm_list* list);
bool dm_list_should_shrink(dm_list* list);
void dm_list_grow(dm_list* list);
void dm_list_shrink(dm_list* list);
void dm_list_resize(dm_list* list, size_t new_size);

dm_list* dm_list_create(size_t element_size, size_t capacity)
{
    dm_list* list = dm_alloc(sizeof(dm_list), DM_MEM_LIST);
    if(capacity==0) capacity = DM_LIST_DEFAULT_CAPACITY;
    list->capacity = capacity;
    list->element_size = element_size;
    list->data = dm_alloc(list->capacity * list->element_size, DM_MEM_LIST);

    return list;
}

void dm_list_destroy(dm_list* list)
{
    dm_free(list->data, list->capacity * list->element_size, DM_MEM_LIST);
    dm_free(list, sizeof(dm_list), DM_MEM_LIST);
}

void dm_list_append(dm_list* list, void* value)
{
    size_t offset = list->count * list->element_size;
    dm_memcpy((char*)list->data + offset, value, list->element_size);
    list->count++;
    if(dm_list_should_grow(list)) dm_list_grow(list);
}

void dm_list_insert(dm_list* list, void* value, uint32_t index)
{
    size_t block_size = (list->count - index) * list->element_size;

    void* dest = (char*)list->data + (index + 1) * list->element_size;
    void* src = (char*)list->data + index * list->element_size;
    dm_memmove(dest, src, block_size);
    dm_memcpy(src, value, list->element_size);
    list->count++;
    if(dm_list_should_grow(list)) dm_list_grow(list);
}

void dm_list_pop(dm_list* list)
{
    list->count--;
    if(dm_list_should_shrink(list)) dm_list_shrink(list);
}

void dm_list_pop_at(dm_list* list, uint32_t index)
{
    size_t block_size = (list->count - index) * list->element_size;

    void* dest = (char*)list->data + index * list->element_size;
    void* src = (char*)list->data + (index + 1) * list->element_size;
    dm_memmove(dest, src, block_size);
    list->count--;
    if(dm_list_should_shrink(list)) dm_list_shrink(list);
}

bool dm_list_should_grow(dm_list* list)
{
    return ((float)list->count / (float)list->capacity >= DM_LIST_LOAD_FACTOR);
}

bool dm_list_should_shrink(dm_list* list)
{
    return ((float)list->count / (float)list->capacity < DM_LIST_LOAD_FACTOR);
}

void dm_list_grow(dm_list* list)
{
    size_t new_capacity = list->capacity * DM_LIST_RESIZE_FACTOR;
    size_t new_size = sizeof(dm_list) + new_capacity * list->element_size;
    dm_list_resize(list, new_size);
    list->capacity = new_capacity;
}

void dm_list_shrink(dm_list* list)
{
    size_t new_capacity = list->capacity / DM_LIST_RESIZE_FACTOR;
    size_t new_size = sizeof(dm_list) + new_capacity * list->element_size;
    dm_list_resize(list, new_size);
    list->capacity = new_capacity;
}

void dm_list_resize(dm_list* list, size_t new_size)
{
    int64_t block_size = (list->capacity - new_size) * list->element_size;
    dm_mem_db_adjust(llabs(block_size), DM_MEM_LIST, DM_MEM_ADJUST_ADD);
    list->data = dm_realloc(list->data, new_size);
}

void* dm_list_at(dm_list* list, uint32_t index)
{
    return ((char*)list->data + index * list->element_size);
}

void dm_list_clear(dm_list* list, size_t new_capacity)
{
    if (new_capacity==0) new_capacity = DM_LIST_DEFAULT_CAPACITY;
    dm_free(list->data, list->capacity * list->element_size, DM_MEM_LIST);
    list->data = dm_alloc(new_capacity * list->element_size, DM_MEM_LIST);
    list->capacity = new_capacity;
    list->count = 0;
}

bool dm_list_is_empty(dm_list* list)
{
    return list->count==0;
}