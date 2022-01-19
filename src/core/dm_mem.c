#include "dm_mem.h"
#include "platform/dm_platform.h"
#include <string.h>
#include <stdio.h>

 typedef struct dm_mem_db
{
	size_t allocs[DM_MEM_RENDERER_UNKNOWN];
	size_t total;
} dm_mem_db;

 static const char* mem_tag_str[] = {
	 "ENGINE         ",
	 "PLATFORM       ",
	 "STRING         ",
	 "LIST           ",
	 "MAP            ",
	 "RENDERER       ",
	 "RENDER PIPELINE",
	 "BUFFER         ",
	 "SHADER         ",
	 "UNKNOWN        "
 };

dm_mem_db mem_db = { 0 };

void* dm_alloc(size_t size, dm_mem_tag tag)
{
	mem_db.total += size;
	mem_db.allocs[tag] += size;

	return dm_platform_alloc(size);
}

void* dm_realloc(void* block, size_t new_size)
{
	return dm_platform_realloc(block, new_size);
}

void dm_free(void* block, size_t size, dm_mem_tag tag)
{
	mem_db.total -= size;
	mem_db.allocs[tag] -= size;

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

void dm_mem_db_adjust(size_t size, dm_mem_tag tag)
{
	mem_db.total += size;
	mem_db.allocs[tag] += size;
}

char* dm_mem_track()
{
	const size_t gb = 1024 * 1024 * 1024;
	const size_t mb = 1024 * 1024;
	const size_t kb = 1024;

	char buffer[1024] = "System memory usage: \n";
	size_t offset = strlen(buffer);

	for (int i = 0; i < DM_MEM_RENDERER_UNKNOWN; i++)
	{
		char unit[4] = "XiB";
		float amount = 1.0f;

		if (mem_db.allocs[i] >= gb)
		{
			unit[0] = 'G';
			amount = mem_db.allocs[i] / (float)gb;
		}
		else if (mem_db.allocs[i] >= mb)
		{
			unit[0] = 'M';
			amount = mem_db.allocs[i] / (float)mb;
		}
		else if (mem_db.allocs[i] >= kb)
		{
			unit[0] = 'K';
			amount = mem_db.allocs[i] / (float)kb;
		}
		else 
		{
			unit[0] = 'B';
			unit[1] = 0;
			amount = mem_db.allocs[i];
		}

		int len = snprintf(buffer + offset, 1024, "  %s: %.2f%s\n", mem_tag_str[i], amount, unit);
		offset += len;
	}

#if __WIN32__ || _WIN32 || WIN32
	char* out_str = _strdup(buffer);
#else
	char* out_str = strdup(buffer);
#endif
	return out_str;
}