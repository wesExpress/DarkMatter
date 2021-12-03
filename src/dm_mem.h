#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>

/*
* wrapper for alloc. checks the block is not NULL before returning
* 
* @param size - size in bytes
*/
void* dm_alloc(size_t size);

#endif