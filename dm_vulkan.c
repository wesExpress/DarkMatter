#include "dm.h"

#ifdef DM_VULKAN
// === commands ===
bool dm_render_command_begin_frame(void* backend) { return true; }
bool dm_render_command_end_frame(void* backend) { return true; }
bool dm_render_command_begin_update_backend(void* backend) { return true; }
bool dm_render_command_end_update_backend(void* backend) { return true; }
bool dm_render_command_begin_render_pass_backend(dm_renderpass_handle handle, float r, float g, float b, float a, float depth, void* backend) { return true; }
bool dm_render_command_end_render_pass_backend(dm_renderpass_handle handle, void* backend) { return true; }
bool dm_render_command_bind_raster_pipeline_backend(dm_pipeline_handle handle, void* backend) { return true; }
bool dm_render_command_submit_resources_backend(dm_resource_handle* handles, uint16_t count, void* backend) { return true; }
bool dm_render_command_bind_vertex_buffer_backend(dm_resource_handle handle, uint8_t slot, size_t offset, void* backend) { return true; }
bool dm_render_command_bind_index_buffer_backend(dm_resource_handle handle, size_t offset, void* backend) { return true; }
bool dm_render_command_update_vertex_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_index_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_constant_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_storage_buffer_backend(dm_resource_handle handle, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_update_texture_backend(dm_resource_handle handle, uint16_t width, uint16_t height, void* data, size_t size, size_t offset, void* backend) { return true; }
bool dm_render_command_draw_instanced_backend(uint32_t instance_count, size_t instance_offset, uint32_t vertex_count, size_t vertex_offset, void* backend) { return true; }
bool dm_render_command_draw_instanced_indexed_backend(uint32_t instance_count, size_t instance_offset, uint32_t index_count, size_t index_offset, size_t vertex_offset, void* backend) { return true; }

#endif
