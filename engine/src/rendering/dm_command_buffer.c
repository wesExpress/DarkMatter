#include "dm_command_buffer.h"
#include "core/dm_logger.h"
#include "core/dm_mem.h"

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, size_t data_size, dm_list* render_commands);

void dm_renderer_begin_renderpass_impl(dm_render_pipeline* pipeline);
void dm_renderer_end_rederpass_impl(dm_render_pipeline* pipeline);
bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_set_viewport_impl(dm_viewport viewport, dm_render_pipeline* pipeline);
void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline);

void dm_renderer_draw_arrays_impl(dm_render_pipeline* pipeline, int first, size_t count);
void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pipeline* pipeline);
void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pipeline* pipeline);

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size);
bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot);

void dm_render_command_begin_renderpass(dm_render_pipeline* pipeline)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, NULL, 0, pipeline->render_commands);
}

void dm_render_command_end_renderpass(dm_render_pipeline* pipeline)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, NULL, 0, pipeline->render_commands);
}

void dm_render_command_bind_pipeline(dm_render_pipeline* pipeline)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, NULL, 0, pipeline->render_commands);
}

void dm_render_command_set_viewport(dm_render_pipeline* pipeline)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, &pipeline->viewport, sizeof(dm_viewport), pipeline->render_commands);
}

void dm_render_command_clear(dm_color* color, dm_render_pipeline* pipeline)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, color, sizeof(dm_color), pipeline->render_commands);
}

void dm_render_command_update_buffer(dm_buffer* buffer, void* data, size_t data_size, dm_render_pipeline* pipeline)
{
	dm_buffer_update_packet* update_packet = dm_alloc(sizeof(dm_buffer_update_packet), DM_MEM_RENDER_COMMAND);
	update_packet->buffer = buffer;
	update_packet->data_size = data_size;
	update_packet->data = dm_alloc(data_size, DM_MEM_RENDER_COMMAND);
	dm_memcpy(update_packet->data, data, data_size);

	dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_BUFFER, update_packet, sizeof(dm_buffer_update_packet), pipeline->render_commands);
}

void dm_render_command_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_render_pipeline* pipeline)
{
	dm_buffer_bind_packet* packet = dm_alloc(sizeof(dm_buffer_bind_packet), DM_MEM_RENDER_COMMAND);
	packet->buffer = buffer;
	packet->slot = slot;
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_BUFFER, packet, sizeof(dm_buffer_bind_packet) + sizeof(dm_buffer), pipeline->render_commands);
}

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_render_pipeline* pipeline)
{
	uint32_t draw_commands[] = { start, count };
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_ARRAYS, &draw_commands, sizeof(draw_commands), pipeline->render_commands);
}

void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pipeline* pipeline)
{
	uint32_t draw_commands[] = { num_indices, index_offset, vertex_offset };
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, &draw_commands, sizeof(draw_commands), pipeline->render_commands);
}

void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pipeline* pipeline)
{
	uint32_t draw_commands[] = { num_indices, num_insts, index_offset, vertex_offset, inst_offset };
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INSTANCED, &draw_commands, sizeof(draw_commands), pipeline->render_commands);
}

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, size_t data_size, dm_list* render_commands)
{
	dm_render_command command = { .data = data, .type = command_type };
	
	dm_list_append(render_commands, &command);
}

void dm_renderer_clear_command_buffer(dm_list* render_commands)
{
	for(uint32_t i=0; i<render_commands->count;i++)
	{
		dm_render_command* command = dm_list_at(render_commands, i);
		switch (command->type)
		{
		case DM_RENDER_COMMAND_BIND_BUFFER:
		{
			dm_buffer_bind_packet* packet = command->data;
			dm_free(packet, sizeof(dm_buffer_bind_packet), DM_MEM_RENDER_COMMAND);
		} break;
		case DM_RENDER_COMMAND_UPDATE_BUFFER:
		{
			dm_buffer_update_packet* update_packet = command->data;
			dm_free(update_packet->data, update_packet->data_size, DM_MEM_RENDER_COMMAND);
			dm_free(update_packet, sizeof(dm_buffer_update_packet), DM_MEM_RENDER_COMMAND);
		} break;
		default:
			break;
		}
	}
	dm_list_clear(render_commands, 0);
}

bool dm_renderer_submit_command_buffer(dm_list* render_commands, dm_render_pipeline* pipeline)
{
	for(uint32_t i=0; i<render_commands->count; i++)
	{
		dm_render_command* command = dm_list_at(render_commands, i);

		switch (command->type)
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
			if (!dm_renderer_bind_pipeline_impl(pipeline)) return false;
		} break;
		case DM_RENDER_COMMAND_UPDATE_BUFFER:
		{
			dm_buffer_update_packet* update_packet = command->data;
			if (!dm_renderer_update_buffer_impl(update_packet->buffer, update_packet->data, update_packet->data_size)) return false;
		} break;
		case DM_RENDER_COMMAND_BIND_BUFFER:
		{
			dm_buffer_bind_packet* packet = command->data;
			if (!dm_renderer_bind_buffer_impl(packet->buffer, packet->slot)) return false;
		} break;
		case DM_RENDER_COMMAND_DRAW_ARRAYS:
		{
			uint32_t* draw_commands = (uint32_t*)command->data;
			dm_renderer_draw_arrays_impl(pipeline, draw_commands[0], draw_commands[1]);
		}
		case DM_RENDER_COMMAND_DRAW_INDEXED:
		{
			uint32_t* draw_commands = (uint32_t*)command->data;
			dm_renderer_draw_indexed_impl(draw_commands[0], draw_commands[1], draw_commands[2], pipeline);
		} break;
		case DM_RENDER_COMMAND_DRAW_INSTANCED:
		{
			uint32_t* draw_commands = (uint32_t*)command->data;
			dm_renderer_draw_instanced_impl(draw_commands[0], draw_commands[1], draw_commands[2], draw_commands[3], draw_commands[4], pipeline);
		} break;
		default:
			DM_LOG_ERROR("Uknown render command. Shouldn't be here...");
			return false;
		}
	}

	return true;
}