#include "dm_mem.h"
#include "platform/dm_platform.h"
#include "dm_logger.h"
#include "core/dm_logger.h"
#include <string.h>
#include <stdio.h>

 typedef struct dm_mem_db
{
	size_t allocs[DM_MEM_UNKNOWN];
	size_t total;
} dm_mem_db;

 static const char* mem_tag_str[] = {
	 "ENGINE         ",
	 "PLATFORM       ",
	 "APPLICATION    ",
	 "INPUT          ",
	 "STRING         ",
	 "LIST           ",
	 "MAP            ",
	 "RENDERER       ",
	 "RENDER COMMAND ",
	 "RENDER PIPELINE",
	 "BUFFER         ",
	 "SHADER         ",
	 "UNIFORM        ",
	 "TEXTURE        ",
	 "UNKNOWN        "
 };

dm_mem_db mem_db = { 0 };

void* dm_alloc(size_t size, dm_mem_tag tag)
{
	mem_db.total += size;
	mem_db.allocs[tag] += size;

	return dm_platform_alloc(size);
}

void* dm_calloc(size_t count, size_t size, dm_mem_tag tag)
{
	mem_db.total += size * count;
	mem_db.allocs[tag] += size * count;

	return dm_platform_calloc(count, size);
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

void dm_mem_db_adjust(size_t size, dm_mem_tag tag, dm_mem_adjust_func adjust_func)
{
	switch (adjust_func)
	{
	case DM_MEM_ADJUST_ADD:
		mem_db.total += size;
		mem_db.allocs[tag] += size;
		break;
	case DM_MEM_ADJUST_SUBTRACT:
		mem_db.total -= size;
		mem_db.allocs[tag] -= size;
		break;
	case DM_MEM_ADJUST_MULTIPLY:
		mem_db.total += (mem_db.allocs[tag] * size);
		mem_db.allocs[tag] *= size;
		break;
	case DM_MEM_ADJUST_DIVIDE:
		mem_db.total += (mem_db.allocs[tag] / size);
		mem_db.allocs[tag] *= size;
		break;
	default: break;
	}
	
}

void dm_mem_track()
{
	const size_t gb = 1024 * 1024 * 1024;
	const size_t mb = 1024 * 1024;
	const size_t kb = 1024;

	char buffer[1024] = "System memory usage: \n";
	size_t offset = strlen(buffer);

	for (int i = 0; i < DM_MEM_UNKNOWN; i++)
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

	DM_LOG_INFO("%s", buffer);
}

void dm_mem_all_freed()
{
	if (mem_db.total > 0)
	{
		DM_LOG_WARN("You have a memory leak somewhere! Total allocated memory is not 0!");
		DM_LOG_WARN("You should... ");

		for (int i = 0; i < DM_MEM_UNKNOWN; i++)
		{
			char buffer[512];

			if (mem_db.allocs[i] > 0)
			{
				switch (i)
				{
				case DM_MEM_ENGINE:
					strcpy(buffer, "engine");
					break;
				case DM_MEM_PLATFORM:
					strcpy(buffer, "platform");
					break;
				case DM_MEM_STRING:
					strcpy(buffer, "string");
					break;
				case DM_MEM_LIST:
					strcpy(buffer, "list");
					break;
				case DM_MEM_MAP:
					strcpy(buffer, "map");
					break;
				case DM_MEM_RENDERER:
					strcpy(buffer, "renderer");
					break;
				case DM_MEM_RENDER_COMMAND:
					strcpy(buffer, "render command");
					break;
				case DM_MEM_RENDER_PIPELINE:
					strcpy(buffer, "render pipeline");
					break;
				case DM_MEM_RENDERER_BUFFER:
					strcpy(buffer, "buffer");
					break;
				case DM_MEM_RENDERER_SHADER:
					strcpy(buffer, "shader");
					break;
				case DM_MEM_RENDERER_UNIFORM:
					strcpy(buffer, "uniform");
					break;
				case DM_MEM_RENDERER_TEXTURE:
					strcpy(buffer, "texture");
					break;
				default:
					break;
				}

				DM_LOG_WARN("Check %s allocations...", buffer);
			}
		}
	}
}