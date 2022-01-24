#include "dm_list.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include "rendering/dm_render_types.h"
#include <math.h>

#define DM_LIST_RESIZE_FACTOR 2
#define DM_LIST_LOAD_FACTOR 0.75

typedef char* dm_void_to_arith;

typedef struct dm_list_header
{
	size_t capacity, count, element_size;
} dm_list_header;
#define DM_LIST_HEADER_OFFSET sizeof(dm_list_header)

bool dm_list_should_grow(dm_list_header* header);
bool dm_list_should_shrink(dm_list_header* header);
void dm_list_grow(dm_list_header* header);
void dm_list_shrink(dm_list_header* header);
void dm_list_resize(dm_list_header* header, size_t new_capacity, dm_mem_adjust_func adjust_func);
dm_list_header* dm_list_get_header(void* list);

void* dm_list_init(size_t element_size, size_t capacity)
{
	dm_list_header* header = dm_alloc(DM_LIST_HEADER_OFFSET + element_size * capacity, DM_MEM_LIST);
	header->capacity = capacity;
	header->element_size = element_size;

	return ((char*)header + DM_LIST_HEADER_OFFSET);
}

void dm_list_destroy(void* list)
{
	dm_list_header* header = dm_list_get_header(list);

	dm_free(header, DM_LIST_HEADER_OFFSET + header->capacity * header->element_size, DM_MEM_LIST);
}

void dm_list_append(void* list, const void* value)
{
	dm_list_header* header = dm_list_get_header(list);

	void* dest = (char*)header + DM_LIST_HEADER_OFFSET + header->count * header->element_size;
	dm_memcpy(dest, value, header->element_size);
	header->count++;
	dm_list_grow(header);

	list = ((char*)header + DM_LIST_HEADER_OFFSET);
}

void dm_list_insert(void* list, const void* value, uint32_t index)
{
	dm_list_header* header = dm_list_get_header(list);

	if (index < header->count)
	{
		// size in bytes to be moved (all elements after index)
		size_t block_size = (header->count - index) * header->element_size;

		// move all elements after index one index to the right
		void* dest = (char*)header + DM_LIST_HEADER_OFFSET + (index + 1) * header->element_size;
		void* src = (char*)header + DM_LIST_HEADER_OFFSET + index * header->element_size;
		dm_memmove(dest, src, block_size);
		// copy value into index
		dm_memcpy(src, value, header->element_size);
		header->count++;
		dm_list_grow(header);
		list = ((char*)header + DM_LIST_HEADER_OFFSET);
	}
	else
	{
		DM_LOG_ERROR("Trying to insert into list with count '%d' with invalid index '%d'", header->count, index);
	}
}

void dm_list_pop(void* list)
{
	dm_list_header* header = dm_list_get_header(list);

	if (header->count > 0)
	{
		header->count--;
		dm_list_shrink(header);
		list = ((char*)header + DM_LIST_HEADER_OFFSET);
	}
	else
	{
		DM_LOG_ERROR("Trying to pop from empty list!");
	}
}

void dm_list_pop_at(void* list, uint32_t index)
{
	dm_list_header* header = dm_list_get_header(list);

	if (index < header->count)
	{
		// size in bytes to be moved (all elements after index)
		size_t block_size = (header->count - index) * header->element_size;

		// move all elements after index one index to the right
		void* dest = (char*)header + DM_LIST_HEADER_OFFSET + index * header->element_size;
		void* src = (char*)header + DM_LIST_HEADER_OFFSET + (index + 1) * header->element_size;
		dm_memmove(dest, src, block_size);

		header->count--;
		dm_list_shrink(header);
		list = ((char*)header + DM_LIST_HEADER_OFFSET);
	}
	else
	{
		DM_LOG_ERROR("Trying to pop from list with count '%d' with invalid index '%d'", header->count, index);
	}
}

void dm_list_clear(void* list, size_t new_capacity)
{
	dm_list_header* header = dm_list_get_header(list);
	if(new_capacity==0) new_capacity = DM_LIST_DEFAULT_CAPACITY;
	size_t old_size = DM_LIST_HEADER_OFFSET + header->capacity * header->element_size;
	size_t new_size = DM_LIST_HEADER_OFFSET + new_capacity * header->element_size;
	size_t elem_size = header->element_size;

	if (header->count > 0)
	{
		dm_free(header, old_size, DM_MEM_LIST);
		header = dm_alloc(new_size, DM_MEM_LIST);
		header->capacity = new_capacity;
		header->element_size = elem_size;
		list = ((char*)header + DM_LIST_HEADER_OFFSET);
	}
	else
	{
		DM_LOG_ERROR("Trying to clear an empty list!");
	}
}

bool dm_list_should_grow(dm_list_header* header)
{
	return ((float)header->count / (float)header->capacity >= DM_LIST_LOAD_FACTOR);
}

bool dm_list_should_shrink(dm_list_header* header)
{
	return ((float)header->count / (float)header->capacity < DM_LIST_LOAD_FACTOR);
}

void dm_list_grow(dm_list_header* header)
{
	if (dm_list_should_grow(header))
	{
		size_t new_capacity = header->capacity * DM_LIST_RESIZE_FACTOR;
		dm_list_resize(header, new_capacity, DM_MEM_ADJUST_ADD);
	}
}

void dm_list_shrink(dm_list_header* header)
{
	if (dm_list_should_shrink(header))
	{
		size_t new_capacity = header->capacity / DM_LIST_RESIZE_FACTOR;
		dm_list_resize(header, new_capacity, DM_MEM_ADJUST_SUBTRACT);
	}
}

void dm_list_resize(dm_list_header* header, size_t new_capacity, dm_mem_adjust_func adjust_func)
{
	int64_t block_size = (header->capacity - new_capacity) * header->element_size;
	dm_mem_db_adjust(llabs(block_size), DM_MEM_LIST, adjust_func);
	dm_realloc(header, DM_LIST_HEADER_OFFSET + new_capacity * header->element_size);
	header->capacity = new_capacity;
}

dm_list_header* dm_list_get_header(void* list)
{
	return (dm_list_header*)((dm_void_to_arith)list - sizeof(dm_list_header));
}

bool dm_list_is_empty(void* list)
{
	dm_list_header* header = dm_list_get_header(list);

	return (header->count == 0);
}

size_t dm_list_get_count(void* list)
{
	dm_list_header* header = dm_list_get_header(list);

	return header->count;
}

size_t dm_list_get_capacity(void* list)
{
	dm_list_header* header = dm_list_get_header(list);

	return header->capacity;
}
