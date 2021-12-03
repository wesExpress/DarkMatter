#include "mem.h"

void* mem_alloc(size_t size)
{
	void* temp = malloc(size);
	if (!temp) return NULL;
	return temp;
}