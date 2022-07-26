#include "dm_byte_buffer.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"

#define DM_BYTE_BUFFER_DEFAULT_CAPACITY 64
#define DM_BYTE_BUFFER_LOAD_FACTOR 0.75
#define DM_BYTE_BUFFER_RESIZE_FACTOR 2

dm_byte_buffer* dm_byte_buffer_create()
{
    dm_byte_buffer* buffer = dm_alloc(sizeof(dm_byte_buffer), DM_MEM_BYTE_BUFFER);
    buffer->size = 0;
    buffer->capacity = DM_BYTE_BUFFER_DEFAULT_CAPACITY;
    buffer->data = dm_alloc(buffer->capacity, DM_MEM_BYTE_BUFFER);
    
    return buffer;
}

void dm_byte_buffer_destroy(dm_byte_buffer* buffer)
{
    dm_free(buffer->data, buffer->capacity, DM_MEM_BYTE_BUFFER);
    dm_free(buffer, sizeof(dm_byte_buffer), DM_MEM_BYTE_BUFFER);
}

void dm_byte_buffer_push(dm_byte_buffer* buffer, void* data, size_t data_size)
{
    while((buffer->size + data_size) / buffer->capacity > DM_BYTE_BUFFER_LOAD_FACTOR)
    {
        buffer->data = dm_realloc(buffer->data, buffer->capacity * DM_BYTE_BUFFER_RESIZE_FACTOR);
        dm_mem_db_adjust(buffer->capacity * DM_BYTE_BUFFER_RESIZE_FACTOR - buffer->capacity, DM_MEM_BYTE_BUFFER, DM_MEM_ADJUST_ADD);
        buffer->capacity *= DM_BYTE_BUFFER_RESIZE_FACTOR;
    }
    
    dm_memcpy((char*)buffer->data + buffer->size, data, data_size);
    buffer->size += data_size;
}

void* dm_byte_buffer_pop(dm_byte_buffer* buffer, size_t data_size)
{
    void* val = (char*)buffer->data + buffer->size - data_size;
    
    buffer->size -= data_size;
    if(buffer->size == 0) dm_byte_buffer_clear(buffer);
    
    return val;
}

void dm_byte_buffer_clear(dm_byte_buffer* buffer)
{
    if(buffer)
    {
        buffer->data = dm_realloc(buffer->data, DM_BYTE_BUFFER_DEFAULT_CAPACITY);
        dm_mem_db_adjust(buffer->capacity - DM_BYTE_BUFFER_DEFAULT_CAPACITY, DM_MEM_BYTE_BUFFER, DM_MEM_ADJUST_SUBTRACT);
        buffer->capacity = DM_BYTE_BUFFER_DEFAULT_CAPACITY;
        buffer->size = 0;
    }
    else
    {
        DM_LOG_ERROR("Trying to clear a NULL byte buffer!");
    }
}