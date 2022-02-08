#include "dm_geometry.h"

#include "dm_renderer.h"

bool dm_geometry_load_primitives()
{
	dm_vertex_t cube_vertices[] = {
		// front face
		{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f} }, // 0
		{ { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f} },
		{ { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f} },
		{ {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f} },
		// bacxk face
		{ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 4
		{ {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
		{ {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
		{ { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
		// top face
		{ {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f} }, // 8
		{ { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f} },
		{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
		{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
		// bottom face
		{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 12
		{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
		{ { 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f} },
		{ {-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f} },
		// left face
		{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} }, // 16
		{ {-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f} },
		{ {-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f} },
		{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} },
		// right face
		{ { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f} }, // 20
		{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
		{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
		{ { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f} }
	};

	dm_index_t cube_indices[] = {
		 0,  1,  2,    2,  3,  0,
		 4,  5,  6,    6,  7,  4,
		 8,  9, 10,   10, 11,  8,
		12, 13, 14,   14, 15, 12,
		16, 17, 18,   18, 19, 16,
		20, 21, 22,   22, 23, 20
	};

	uint32_t num_vertices = sizeof(cube_vertices) / sizeof(dm_vertex_t);
	uint32_t num_indices = sizeof(cube_indices) / sizeof(dm_index_t);

	dm_renderer_submit_vertex_data(cube_vertices, cube_indices, num_vertices, num_indices, "cube");

	return true;
}