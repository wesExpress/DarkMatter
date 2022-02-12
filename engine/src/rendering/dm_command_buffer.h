#ifndef __DM_COMMAND_BUFFER_H__
#define __DM_COMMAND_BUFFER_H__

#include "dm_render_types.h"

// render commands
void dm_render_command_begin_render_pass(dm_render_pass* render_pass, dm_list* render_commands);
void dm_render_command_end_render_pass(dm_render_pass* render_pass, dm_list* render_commands);

void dm_render_command_bind_pipeline(dm_render_pipeline* pipeline, dm_list* render_commands);

void dm_render_command_set_viewport(dm_viewport* viewport, dm_list* render_commands);

void dm_render_command_clear(dm_color* color, dm_list* render_commands);

void dm_render_command_update_buffer(dm_buffer* buffer, void* data, size_t data_size, dm_list* render_commands);
void dm_render_command_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_list* render_commands);

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_render_pass* render_pass, dm_list* render_commands);
void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass, dm_list* render_commands);
void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass, dm_list* render_commands);

void dm_renderer_clear_command_buffer(dm_list* render_commands);
bool dm_renderer_submit_command_buffer(dm_list* render_commands, dm_render_pipeline* pipeline);

#endif