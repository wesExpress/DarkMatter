#include "dm_command_buffer.h"
#include "core/dm_logger.h"

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline);
void dm_renderer_end_rederpass_impl(dm_render_pipeline* pipeline);
bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline);
void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline);

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count);
void dm_renderer_draw_indexed_impl(dm_render_pipeline* pipeline);

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size);
bool dm_renderer_bind_buffer_impl(dm_buffer* buffer);

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, dm_list* render_commands)
{
	dm_render_command command = { .command = command_type, .data = data };
	dm_list_append(render_commands, &command);
}

void dm_renderer_clear_command_buffer(dm_list* render_commands)
{
	dm_list_clear(render_commands, 0);
}

bool dm_renderer_submit_command_buffer(dm_list* render_commands, dm_render_pipeline* pipeline)
{
	for(uint32_t i=0; i<render_commands->count; i++)
	{
		dm_render_command* command = dm_list_at(render_commands, i);

		switch (command->command)
		{
		// TODO flesh out
		case DM_RENDER_COMMAND_BEGIN_RENDER_PASS:
		{
			dm_renderer_begin_renderpass_impl(pipeline);
		} break;
		case DM_RENDER_COMMAND_END_RENDER_PASS:
		{
			dm_renderer_end_rederpass_impl(pipeline);
		} break;
		case DM_RENDER_COMMAND_SET_VIEWPORT:
		{
			dm_renderer_set_viewport_impl(pipeline->viewport, pipeline);
		} break;
		case DM_RENDER_COMMAND_CLEAR:
		{
			dm_renderer_clear_impl((dm_color*)command->data, pipeline);
		} break;
		case DM_RENDER_COMMAND_BIND_PIPELINE:
		{
			if (!dm_renderer_bind_pipeline_impl((dm_render_pipeline*)command->data)) return false;
		} break;
		case DM_RENDER_COMMAND_UPDATE_BUFFER:
		{
			dm_buffer_update_packet* update_packet = command->data;
			if (!dm_renderer_update_buffer_impl(update_packet->buffer, update_packet->data, update_packet->data_size)) return false;
		}
		case DM_RENDER_COMMAND_BIND_BUFFER:
		{
			dm_buffer* buffer = command->data;
			if (!dm_renderer_bind_buffer_impl((dm_buffer*)command->data)) return false;
		} break;
		case DM_RENDER_COMMAND_DRAW_ARRAYS:
		{
			dm_renderer_draw_arrays_impl(pipeline, 0, 36);
		}
		case DM_RENDER_COMMAND_DRAW_INDEXED:
		{
			dm_renderer_draw_indexed_impl(pipeline);
		} break;
		case DM_RENDER_COMMAND_DRAW_INSTANCED:
		{} break;
		default:
			DM_LOG_ERROR("Uknown render command. Shouldn't be here...");
			return false;
		}
	}

	return true;
}