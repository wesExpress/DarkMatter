#include "dm_mem.h"

void* dm_alloc(size_t size)
{
	void* temp = malloc(size);
	if (!temp) return NULL;
	return temp;
}