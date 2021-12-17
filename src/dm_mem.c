#include "dm_mem.h"
#include "platform/dm_platform.h"

void* dm_alloc(size_t size)
{
	return dm_platform_alloc(size);
}

void* dm_realloc(void* block, size_t new_size)
{
	return dm_platform_realloc(block, new_size);
}

void dm_free(void* block)
{
	dm_platform_free(block);
}

void* dm_memzero(void* block, size_t size)
{
	return dm_platform_memzero(block, size);
}

void* dm_memcpy(void* dest, const void* src, size_t size)
{
	return dm_platform_memcpy(dest, src, size);
}

void dm_memmove(void* dest, const void* src, size_t size)
{
	dm_platform_memmove(dest, src, size);
}