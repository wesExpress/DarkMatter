#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>
#include <stdbool.h>

typedef enum dm_mem_tag
{
	DM_MEM_ENGINE,
	DM_MEM_PLATFORM,
	DM_MEM_APPLICATION,
	DM_MEM_INPUT,
	DM_MEM_STRING,
	DM_MEM_LIST,
    DM_MEM_BYTE_BUFFER,
	DM_MEM_MAP,
	DM_MEM_RENDERER,
	DM_MEM_RENDER_COMMAND,
	DM_MEM_RENDER_PIPELINE,
	DM_MEM_RENDER_PASS,
	DM_MEM_RENDERER_BUFFER,
	DM_MEM_RENDERER_SHADER,
	DM_MEM_RENDERER_UNIFORM,
	DM_MEM_RENDERER_TEXTURE,
	DM_MEM_UNKNOWN
} dm_mem_tag;

typedef enum dm_mem_adjust_func
{
	DM_MEM_ADJUST_ADD,
	DM_MEM_ADJUST_SUBTRACT,
	DM_MEM_ADJUST_MULTIPLY,
	DM_MEM_ADJUST_DIVIDE,
	DM_MEM_ADJUST_UNKNOWN
} dm_mem_adjust_func;

/*
* wrapper for alloc. checks the block is not NULL before returning
* 
* @param size - size in bytes
* @param tag - memory tag
*/
void* dm_alloc(size_t size, dm_mem_tag tag);

/*
* wrapper for calloc
*/
void* dm_calloc(size_t count, size_t size, dm_mem_tag tag);

/*
* wrapper for realloc. reallocate the size of a memory block.
* if you care about mem tracking, make sure to call dm_mem_db_adjust before this!!!
* 
* @param block - memory block to reallocate
* @param new_size - size in bytes
*/
void* dm_realloc(void* block, size_t new_size);

/*
* wrapper for free. frees block of memory. must have been created with dm_alloc!
* 
* @param block - memory block to free
* @param size - size in bytes
* @param tag - memory tag
*/
void dm_free(void* block, size_t size, dm_mem_tag tag);

/*
* wrapper for memzero. sets block of memory to zero
* 
* @param block - memory block to set to zero
* @param size - size in bytes
*/
void* dm_memzero(void* block, size_t size);

/*
* wrapper for memcpy. copies data from one location to another.
* 
* @param dest - destination memory block
* @param src - src memory block
* @param size - size in bytes to copy
*/
void* dm_memcpy(void* dest, const void* src, size_t size);

/*
* wrapper for memmove. moves block of memory to new location.
* 
* @param dest - destination memory block
* @param src - source memory block
* @param size - size in bytes
*/
void dm_memmove(void* dest, const void* src, size_t size);

/*
* useful for memory tracking. Should be called before realloc
* 
* @param size - size in bytes
* @param tag - memory tag
*/
void dm_mem_db_adjust(size_t size, dm_mem_tag tag, dm_mem_adjust_func adjust_func);

// memory tracking printing functions
void dm_mem_track();

// to be called at the end of the application to check if you've freed all allocated memory
void dm_mem_all_freed();

#endif