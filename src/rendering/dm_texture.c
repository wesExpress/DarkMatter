#include "dm_texture.h"
#include "structures/dm_map.h"
#include "dm_render_types.h"
#include <stdio.h>

bool dm_textures_load(const char** paths, int num_paths)
{
	//dm_map(dm_texture*) map;
	//dm_map_init(&map, dm_texture*);
	//dm_map_destroy(&map);

	dm_map_t* map = dm_map_create(sizeof(int), DM_MAP_DEFAULT_SIZE);
	for (int i = 0; i < 5; i++)
	{
		char buffer[512];
		sprintf(buffer, "%d", i);
		dm_map_insert(map, buffer, &i);
	}
	dm_map_destroy(map);

	return true;
}