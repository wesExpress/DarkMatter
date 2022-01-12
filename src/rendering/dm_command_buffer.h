#ifndef __DM_COMMAND_BUFFER_H__
#define __DM_COMMAND_BUFFER_H__

#include "dm_render_types.h"

// render commands
void dm_renderer_submit_command(dm_render_command_type command, void* data, dm_command_buffer* command_buffer);
void dm_renderer_clear_command_buffer(dm_command_buffer* command_buffer);
void dm_renderer_destroy_command_buffer(dm_command_buffer* command_buffer);
bool dm_renderer_submit_command_buffer(dm_command_buffer* command_buffer, dm_render_pipeline* pipeline);

#endif