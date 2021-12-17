#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>

/*
* wrapper for alloc. checks the block is not NULL before returning
* 
* @param size - size in bytes
*/
void* dm_alloc(size_t size);

/*
* wrapper for realloc. reallocate the size of a memory block.
* 
* @param block - memory block to reallocate
* @param size - size in bytes
*/
void* dm_realloc(void* block, size_t new_size);

/*
* wrapper for free. frees block of memory. must have been created with dm_alloc!
* 
* @param block - memory block to free
*/
void dm_free(void* block);

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

#endif