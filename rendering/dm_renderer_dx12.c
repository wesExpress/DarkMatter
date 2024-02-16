#include "dm.h"

bool dm_renderer_backend_init(dm_context* context)
{
    return false;
}

void dm_renderer_backend_shutdown(dm_context* context)
{
}

bool dm_renderer_backend_begin_frame(dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_end_frame(dm_context* context)
{
    return false;
}

void dm_renderer_backend_resize(uint32_t width, uint32_t height, dm_renderer* renderer)
{
}

void* dm_renderer_backend_get_internal_texture_ptr(dm_render_handle handle, dm_renderer* renderer)
{
    return NULL;
}

bool dm_renderer_backend_create_buffer(dm_buffer_desc desc, void* data, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_uniform(size_t size, dm_uniform_stage stage, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_shader_and_pipeline(dm_shader_desc shader_desc, dm_pipeline_desc pipe_desc, dm_vertex_attrib_desc* attrib_descs, uint32_t attrib_count, dm_render_handle* shader_handle, dm_render_handle* pipe_handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_dynamic_texture(uint32_t width, uint32_t height, uint32_t num_channels, const void* data, const char* name, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_renderer_backend_create_renderpass(dm_renderpass_desc desc, dm_render_handle* handle, dm_renderer* renderer)
{
    return false;
}

void dm_render_command_backend_clear(float r, float g, float b, float a, dm_renderer* renderer)
{
}

void dm_render_command_backend_set_viewport(uint32_t width, uint32_t height, dm_renderer* renderer)
{
}

bool dm_render_command_backend_bind_pipeline(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_set_primitive_topology(dm_primitive_topology topology, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_shader(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_buffer(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_buffer(dm_render_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_uniform(dm_render_handle handle, dm_uniform_stage stage, uint32_t slot, uint32_t offset, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_uniform(dm_render_handle handle, void* data, size_t data_size, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_update_texture(dm_render_handle handle, uint32_t width, uint32_t height, void* data, size_t data_size, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_default_framebuffer(dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_framebuffer(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_bind_framebuffer_texture(dm_render_handle handle, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_begin_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_render_command_backend_end_renderpass(dm_render_handle handle, dm_renderer* renderer)
{
    return false;
}

void dm_render_command_backend_draw_arrays(uint32_t start, uint32_t count, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_renderer* renderer)
{
}

void dm_render_command_backend_toggle_wireframe(bool wireframe, dm_renderer* renderer)
{
}

// compute
bool dm_compute_backend_create_shader(dm_compute_shader_desc desc, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_create_buffer(size_t data_size, size_t elem_size, dm_compute_buffer_type type, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_create_uniform(size_t data_size, dm_compute_handle* handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_bind_buffer(dm_compute_handle handle, uint32_t offset, uint32_t slot, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_update_buffer(dm_compute_handle handle, void* data, size_t data_size, size_t offset, dm_renderer* renderer)
{
    return false;
}

void* dm_compute_backend_command_get_buffer_data(dm_compute_handle handle, dm_renderer* renderer)
{
    return NULL;
}

bool dm_compute_backend_command_bind_shader(dm_compute_handle handle, dm_renderer* renderer)
{
    return false;
}

bool dm_compute_backend_command_dispatch(uint32_t threads_per_group_x, uint32_t threads_per_group_y, uint32_t threads_per_group_z, uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z, dm_renderer* renderer)
{
    return false;
}