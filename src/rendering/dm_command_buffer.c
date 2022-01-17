#include "dm_command_buffer.h"

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline);
void dm_renderer_end_rederpass_impl();
bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline);
void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline);

void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline);

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, dm_command_buffer* command_buffer)
{
	dm_render_command command = { .command = command_type, .data = data };
	dm_list_append(&command_buffer->commands, command);
}

void dm_renderer_clear_command_buffer(dm_command_buffer* command_buffer)
{
	dm_list_clear(&command_buffer->commands);
}

void dm_renderer_destroy_command_buffer(dm_command_buffer* command_buffer)
{
	dm_list_destroy(&command_buffer->commands);
}

bool dm_renderer_submit_command_buffer(dm_command_buffer* command_buffer, dm_render_pipeline* pipeline)
{
	dm_list_for_range(command_buffer->commands)
	{
		dm_render_command command = command_buffer->commands.array[i];

		switch (command.command)
		{
		// TODO flesh out
		case DM_RENDER_COMMAND_BEGIN_RENDER_PASS:
		{
			dm_renderer_begin_renderpass_impl(pipeline);
		} break;
		case DM_RENDER_COMMAND_END_RENDER_PASS:
		{
			dm_renderer_end_rederpass_impl();
		} break;
		case DM_RENDER_COMMAND_SET_VIEWPORT:
		{
			dm_renderer_set_viewport_impl(pipeline->viewport, pipeline);
		} break;
		case DM_RENDER_COMMAND_CLEAR:
		{
			dm_renderer_clear_impl((dm_color*)command.data, pipeline);
		} break;
		case DM_RENDER_COMMAND_BIND_PIPELINE:
		{
			if (!dm_renderer_bind_pipeline_impl((dm_render_pipeline*)command.data)) return false;
		} break;
		case DM_RENDER_COMMAND_DRAW_INDEXED:
		{
			dm_renderer_draw_indexed_impl(pipeline);
		} break;
		case DM_RENDER_COMMAND_DRAW_INSTANCED:
		{} break;
		default:
			DM_LOG_ERROR("Uknown render command. Shouldn't be here...");
			break;
		}
	}

	return true;
}