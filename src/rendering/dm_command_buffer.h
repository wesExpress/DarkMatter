#ifndef __DM_COMMAND_BUFFER_H__
#define __DM_COMMAND_BUFFER_H__

#include "dm_render_types.h"

// render commands
void dm_renderer_submit_command(dm_render_command_type command, void* data, dm_list* render_commands);
void dm_renderer_clear_command_buffer(dm_list* render_commands);
bool dm_renderer_submit_command_buffer(dm_list* render_commands, dm_render_pipeline* pipeline);

#endif