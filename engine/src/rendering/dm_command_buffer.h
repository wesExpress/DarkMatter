#ifndef __DM_COMMAND_BUFFER_H__
#define __DM_COMMAND_BUFFER_H__

#include "dm_render_types.h"

void dm_render_command_init();
void dm_render_command_shutdown();

// render commands
void dm_render_command_set_viewport(dm_viewport viewport);
void dm_render_command_clear(dm_color color);

void dm_render_command_begin_renderpass(dm_render_pass* render_pass);
void dm_render_command_end_renderpass(dm_render_pass* render_pass);

void dm_render_command_update_buffer(dm_buffer* buffer, void* data, size_t data_size);
void dm_render_command_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_render_pass* render_pass);

void dm_render_command_bind_texture(dm_image* image, uint32_t slot, dm_render_pass* render_pass);

void dm_render_command_bind_uniforms(uint32_t slot, dm_render_pass* render_pass);

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_render_pass* render_pass);
void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass);
void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass);

void dm_renderer_clear_command_buffer();
bool dm_renderer_submit_command_buffer();

#endif