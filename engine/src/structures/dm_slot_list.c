#include "dm_slot_list.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"

#define DM_SLOT_LIST_DEFAULT_CAPACITY 16
#define DM_SLOT_LIST_LOAD_FACTOR 0.75
#define DM_SLOT_LIST_RESIZE_FACTOR 2

dm_slot_list* dm_slot_list_create(size_t element_size, size_t capacity)
{
    dm_slot_list* list = dm_alloc(sizeof(dm_slot_list), DM_MEM_SLOT_LIST);
    if(capacity == 0) capacity = DM_SLOT_LIST_DEFAULT_CAPACITY;
    list->capacity = capacity;
    list->element_size = element_size;
    list->data = dm_alloc(list->capacity * list->element_size, DM_MEM_SLOT_LIST);
    
    return list;
}

void dm_slot_list_destroy(dm_slot_list* list)
{
    if(list)
    {
        dm_free(list->data, list->capacity * list->element_size, DM_MEM_SLOT_LIST);
        dm_free(list, sizeof(dm_slot_list), DM_MEM_SLOT_LIST);
    }
    else
    {
        DM_LOG_ERROR("Trying to delete a NULL slot list!");
    }
}

void dm_slot_list_insert(dm_slot_list* list, void* value, uint32_t* index)
{
    for (uint32_t i=0; i<list->capacity; i++)
    {
        void* item = (char*)list->data + i * list->element_size;
        
        if(!item)
        {
            *index = i;
        }
    }
    
    if(*index / list->capacity > DM_SLOT_LIST_LOAD_FACTOR)
    {
        dm_realloc(list->data, list->capacity * DM_SLOT_LIST_RESIZE_FACTOR);
        dm_mem_db_adjust(list->capacity * DM_SLOT_LIST_RESIZE_FACTOR - list->capacity, DM_MEM_SLOT_LIST, DM_MEM_ADJUST_ADD);
    }
    
    size_t offset = *index * list->element_size;
    dm_memcpy((char*)list->data + offset, value, list->element_size);
    list->count++;
}

void* dm_slot_list_at(dm_slot_list* list, uint32_t index)
{
    if(list && index < list->capacity)
    {
        return ((char*)list->data + index * list->element_size);
    }
    
    DM_LOG_ERROR("Trying to insert into either invalid slot list, or with an invalid index!");
    return NULL;
}

void dm_slot_list_clear(dm_slot_list* list, size_t new_capacity)
{
    if(list)
    {
        if (new_capacity == 0) new_capacity = DM_SLOT_LIST_DEFAULT_CAPACITY;
        dm_free(list->data, list->capacity * list->element_size, DM_MEM_SLOT_LIST);
        list->data = dm_alloc(new_capacity * list->element_size, DM_MEM_SLOT_LIST);
        list->capacity = new_capacity;
        list->count = 0;
    }
    else
    {
        DM_LOG_ERROR("Trying to clear NULL slot list!");
    }
}

bool dm_slot_list_is_empty(dm_slot_list* list)
{
    if(list)
    {
        return list->count == 0;
    }
    DM_LOG_ERROR("Invalid slot list!");
    return false;
}
