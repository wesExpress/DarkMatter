#include "dm_command_buffer.h"
#include "core/dm_logger.h"
#include "core/dm_mem.h"

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, dm_list* render_commands);

bool dm_renderer_begin_renderpass_impl(dm_render_pass* render_pass);
void dm_renderer_end_rederpass_impl(dm_render_pass* render_pass);
bool dm_renderer_bind_pipeline_impl(dm_render_pipeline* pipeline);
void dm_renderer_set_viewport_impl(dm_viewport viewport);
void dm_renderer_clear_impl(dm_color* clear_color, dm_render_pipeline* pipeline);

void dm_renderer_draw_arrays_impl(uint32_t start, uint32_t count, dm_render_pass* render_pass);
void dm_renderer_draw_indexed_impl(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass);
void dm_renderer_draw_instanced_impl(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass);

bool dm_renderer_update_buffer_impl(dm_buffer* buffer, void* data, size_t data_size);
bool dm_renderer_bind_buffer_impl(dm_buffer* buffer, uint32_t slot);
bool dm_renderer_bind_texture_impl(dm_image* image, uint32_t slot);
bool dm_renderer_bind_uniform_impl(dm_uniform* uniform);

typedef struct dm_draw_command
{
	uint32_t* commands;
	dm_render_pass* render_pass;
} dm_draw_command;

typedef struct dm_texture_command
{
    dm_image* image;
    uint32_t slot;
} dm_texture_command;

void dm_render_command_begin_renderpass(dm_render_pass* render_pass, dm_list* render_commands)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_BEGIN_RENDER_PASS, render_pass, render_commands);
}

void dm_render_command_end_renderpass(dm_render_pass* render_pass, dm_list* render_commands)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_END_RENDER_PASS, render_pass, render_commands);
}

void dm_render_command_bind_pipeline(dm_render_pipeline* pipeline, dm_list* render_commands)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_PIPELINE, NULL, render_commands);
}

void dm_render_command_set_viewport(dm_viewport* viewport, dm_list* render_commands)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_SET_VIEWPORT, viewport, render_commands);
}

void dm_render_command_clear(dm_color* color, dm_list* render_commands)
{
	dm_renderer_submit_command(DM_RENDER_COMMAND_CLEAR, color, render_commands);
}

void dm_render_command_update_buffer(dm_buffer* buffer, void* data, size_t data_size, dm_list* render_commands)
{
	dm_buffer_update_packet* update_packet = dm_alloc(sizeof(dm_buffer_update_packet), DM_MEM_RENDER_COMMAND);
	update_packet->buffer = buffer;
	update_packet->data_size = data_size;
	update_packet->data = dm_alloc(data_size, DM_MEM_RENDER_COMMAND);
	dm_memcpy(update_packet->data, data, data_size);
    
	dm_renderer_submit_command(DM_RENDER_COMMAND_UPDATE_BUFFER, update_packet, render_commands);
}

void dm_render_command_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_list* render_commands)
{
	dm_buffer_bind_packet* packet = dm_alloc(sizeof(dm_buffer_bind_packet), DM_MEM_RENDER_COMMAND);
	packet->buffer = buffer;
	packet->slot = slot;
	dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_BUFFER, packet, render_commands);
}

void dm_render_command_bind_texture(dm_image* image, uint32_t slot, dm_list* render_commands)
{
    dm_texture_command* texture_command = dm_alloc(sizeof(dm_texture_command), DM_MEM_RENDER_COMMAND);
    texture_command->image = image;
    texture_command->slot = slot;
    dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_TEXTURE, texture_command, render_commands);
}

void dm_render_command_bind_uniform(dm_uniform* uniform, dm_list* render_commands)
{
    dm_renderer_submit_command(DM_RENDER_COMMAND_BIND_UNIFORM, uniform, render_commands);
}

void dm_render_command_draw_arrays(uint32_t start, uint32_t count, dm_render_pass* render_pass, dm_list* render_commands)
{
	dm_draw_command* draw_command = dm_alloc(sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
    
	draw_command->commands = dm_alloc(sizeof(uint32_t)*2, DM_MEM_RENDER_COMMAND);
	draw_command->commands[0] = start;
	draw_command->commands[1] = count;
    
	draw_command->render_pass = render_pass;
    
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_ARRAYS, draw_command, render_commands);
}

void dm_render_command_draw_indexed(uint32_t num_indices, uint32_t index_offset, uint32_t vertex_offset, dm_render_pass* render_pass, dm_list* render_commands)
{
	dm_draw_command* draw_command = dm_alloc(sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
    
	draw_command->commands = dm_alloc(sizeof(uint32_t) * 3, DM_MEM_RENDER_COMMAND);
	draw_command->commands[0] = num_indices;
	draw_command->commands[1] = index_offset;
	draw_command->commands[2] = vertex_offset;
    
	draw_command->render_pass = render_pass;
    
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INDEXED, draw_command, render_commands);
}

void dm_render_command_draw_instanced(uint32_t num_indices, uint32_t num_insts, uint32_t index_offset, uint32_t vertex_offset, uint32_t inst_offset, dm_render_pass* render_pass, dm_list* render_commands)
{
	dm_draw_command* draw_command = dm_alloc(sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
    
	draw_command->commands = dm_alloc(sizeof(uint32_t) * 5, DM_MEM_RENDER_COMMAND);
	draw_command->commands[0] = num_indices;
	draw_command->commands[1] = num_insts;
	draw_command->commands[2] = index_offset;
	draw_command->commands[3] = vertex_offset;
	draw_command->commands[4] = inst_offset;
    
	draw_command->render_pass = render_pass;
    
	dm_renderer_submit_command(DM_RENDER_COMMAND_DRAW_INSTANCED, draw_command, render_commands);
}

void dm_renderer_submit_command(dm_render_command_type command_type, void* data, dm_list* render_commands)
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
            case DM_RENDER_COMMAND_BIND_TEXTURE:
            {
                dm_texture_command* texture_command = command->data;
                dm_free(texture_command, sizeof(dm_texture_command), DM_MEM_RENDER_COMMAND);
            } break;
            case DM_RENDER_COMMAND_UPDATE_BUFFER:
            {
                dm_buffer_update_packet* update_packet = command->data;
                dm_free(update_packet->data, update_packet->data_size, DM_MEM_RENDER_COMMAND);
                dm_free(update_packet, sizeof(dm_buffer_update_packet), DM_MEM_RENDER_COMMAND);
            } break;
            case DM_RENDER_COMMAND_DRAW_ARRAYS:
            {
                dm_draw_command* draw_command = command->data;
                
                dm_free(draw_command->commands, sizeof(uint32_t) * 2, DM_MEM_RENDER_COMMAND);
                dm_free(command->data, sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
            } break;
            case DM_RENDER_COMMAND_DRAW_INDEXED:
            {
                dm_draw_command* draw_command = command->data;
                
                dm_free(draw_command->commands, sizeof(uint32_t) * 3, DM_MEM_RENDER_COMMAND);
                dm_free(command->data, sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
            } break;
            case DM_RENDER_COMMAND_DRAW_INSTANCED:
            {
                dm_draw_command* draw_command = command->data;
                
                dm_free(draw_command->commands, sizeof(uint32_t) * 5, DM_MEM_RENDER_COMMAND);
                dm_free(command->data, sizeof(dm_draw_command), DM_MEM_RENDER_COMMAND);
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
                if (!dm_renderer_begin_renderpass_impl((dm_render_pass*)command->data)) return false;
            } break;
            case DM_RENDER_COMMAND_END_RENDER_PASS:
            {
                dm_renderer_end_rederpass_impl((dm_render_pass*)command->data);
            } break;
            case DM_RENDER_COMMAND_SET_VIEWPORT:
            {
                dm_renderer_set_viewport_impl(*(dm_viewport*)command->data);
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
            case DM_RENDER_COMMAND_BIND_TEXTURE:
            {
                dm_texture_command* texture_command = command->data;
                if(!dm_renderer_bind_texture_impl(texture_command->image, texture_command->slot)) return false;
            } break;
            case DM_RENDER_COMMAND_BIND_UNIFORM:
            {
                if(!dm_renderer_bind_uniform_impl((dm_uniform*)command->data)) return false;
            } break;
            case DM_RENDER_COMMAND_DRAW_ARRAYS:
            {
                dm_draw_command* draw_command = command->data;
                dm_renderer_draw_arrays_impl(draw_command->commands[0], draw_command->commands[1], draw_command->render_pass);
            } break;
            case DM_RENDER_COMMAND_DRAW_INDEXED:
            {
                dm_draw_command* draw_command = command->data;
                dm_renderer_draw_indexed_impl(draw_command->commands[0], draw_command->commands[1], draw_command->commands[2], draw_command->render_pass);
            } break;
            case DM_RENDER_COMMAND_DRAW_INSTANCED:
            {
                dm_draw_command* draw_command = command->data;
                dm_renderer_draw_instanced_impl(draw_command->commands[0], draw_command->commands[1], draw_command->commands[2], draw_command->commands[3], draw_command->commands[4], draw_command->render_pass);
            } break;
            default:
			DM_LOG_ERROR("Uknown render command. Shouldn't be here...");
			return false;
		}
	}
    
	return true;
}