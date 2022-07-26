#ifndef __DM_BYTE_BUFFER_H__
#define __DM_BYTE_BUFFER_H__

/*
inspired and borrowed heavily from gunslinger
https://github.com/MrFrenik/gunslinger/blob/master/gs.h
*/

#include <stdbool.h>
#include <stdint.h>

typedef struct dm_byte_buffer
{
    size_t size;
    size_t capacity;
    void* data;
} dm_byte_buffer;

dm_byte_buffer* dm_byte_buffer_create();
void dm_byte_buffer_destroy(dm_byte_buffer* buffer);

void dm_byte_buffer_push(dm_byte_buffer* buffer, void* data, size_t data_size);
void* dm_byte_buffer_pop(dm_byte_buffer* buffer, size_t data_size);

void dm_byte_buffer_clear(dm_byte_buffer* buffer);

#endif 
