#define NK_IMPLEMENTATION
#include "dm.h"

#include <float.h>

extern void dm_platform_clipboard_copy(const char* text, int len);
extern void dm_platform_clipboard_paste(void (*callback)(char*,int,void*), void* edit);

void dm_imgui_nuklear_copy(nk_handle usr, const char* text, int len);
void dm_imgui_nuklear_paste(nk_handle usr, struct nk_text_edit* edit);
void dm_imgui_nuklear_paste_callback(char* text, int len, void* edit);

//
bool dm_imgui_init(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = &imgui_ctx->internal_context;
    
    if(!dm_renderer_create_dynamic_vertex_buffer(NULL, DM_IMGUI_MAX_VERTICES, sizeof(dm_imgui_vertex), &imgui_ctx->vb, context)) return false;
    if(!dm_renderer_create_dynamic_index_buffer(NULL, DM_IMGUI_MAX_INDICES, sizeof(dm_nk_element_t), &imgui_ctx->ib, context)) return false;
    if(!dm_renderer_create_uniform(sizeof(dm_imgui_uni), DM_UNIFORM_STAGE_VERTEX, &imgui_ctx->uni, context)) return false;
    
    dm_vertex_attrib_desc attrib_descs[] = {
        { .name="POSITION", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, pos), .count=4, .index=0, .normalized=false },
        { .name="TEXCOORD", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, tex_coords), .count=2, .index=0, .normalized=false },
        { .name="COLOR", .data_t=DM_VERTEX_DATA_T_UBYTE_NORM, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, color), .count=4, .index=0, .normalized=false }
    };
    
    // pipeline desc
    dm_pipeline_desc pipeline_desc = { 0 };
    pipeline_desc.cull_mode          = DM_CULL_NONE;
    pipeline_desc.winding_order      = DM_WINDING_CLOCK;
    pipeline_desc.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;
    
    pipeline_desc.depth = true;
    pipeline_desc.depth_comp = DM_COMPARISON_ALWAYS;
    
    pipeline_desc.blend = true;
    pipeline_desc.blend_eq           = DM_BLEND_EQUATION_ADD;
    pipeline_desc.blend_src_f        = DM_BLEND_FUNC_SRC_ALPHA;
    pipeline_desc.blend_dest_f       = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.blend_src_alpha_f  = DM_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.blend_dest_alpha_f = DM_BLEND_FUNC_ZERO;
    pipeline_desc.blend_alpha_eq     = DM_BLEND_EQUATION_ADD;
    
    pipeline_desc.sampler_comp = DM_COMPARISON_ALWAYS;
    pipeline_desc.sampler_filter = DM_FILTER_LINEAR;
    pipeline_desc.u_mode = pipeline_desc.v_mode = pipeline_desc.w_mode = DM_TEXTURE_MODE_EDGE;
    pipeline_desc.max_lod = FLT_MAX;
    
    dm_shader_desc shader_desc = { 0 };
#ifdef DM_VULKAN
    strcpy(shader_desc.vertex, "assets/shaders/dm_imgui_vertex.spv");
    strcpy(shader_desc.pixel,  "assets/shaders/dm_imgui_pixel.spv");
#elif defined(DM_OPENGL)
    strcpy(shader_desc.vertex, "assets/shaders/dm_imgui_vertex.glsl");
    strcpy(shader_desc.pixel,  "assets/shaders/dm_imgui_pixel.glsl");
    
    shader_desc.vb_count = 1;
    shader_desc.vb[0] = imgui_ctx->vb;
#elif defined(DM_DIRECTX)
    strcpy(shader_desc.vertex, "assets/shaders/dm_imgui_vertex.fxc");
    strcpy(shader_desc.pixel,  "assets/shaders/dm_imgui_pixel.fxc");
#else
    strcpy(shader_desc.vertex, "vertex_main");
    strcpy(shader_desc.pixel,  "fragment_main");
    strcpy(shader_desc.master, "assets/shaders/dm_imgui.metallib");
#endif
    
    if(!dm_renderer_create_shader_and_pipeline(shader_desc, pipeline_desc, attrib_descs, DM_ARRAY_LEN(attrib_descs), &imgui_ctx->shader, &imgui_ctx->pipe, context)) return false;
    
    // nuklear stuff
    nk_init_default(&imgui_nk_ctx->ctx, 0);
    imgui_nk_ctx->ctx.clip.copy = dm_imgui_nuklear_copy;
    imgui_nk_ctx->ctx.clip.paste = dm_imgui_nuklear_paste;
    
    imgui_nk_ctx->max_vertex_buffer = DM_IMGUI_MAX_VERTICES;
    imgui_nk_ctx->max_index_buffer = DM_IMGUI_MAX_INDICES;
    
    nk_buffer_init_default(&imgui_nk_ctx->cmds);
    
    nk_font_atlas_init_default(&imgui_nk_ctx->atlas);
    nk_font_atlas_begin(&imgui_nk_ctx->atlas);
    
    struct nk_font* chicago = nk_font_atlas_add_from_file(&imgui_nk_ctx->atlas, "assets/fonts/Chicago.ttf", 12, 0);
    
    uint32_t w, h;
    const void* image = nk_font_atlas_bake(&imgui_nk_ctx->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    
    if(!dm_renderer_create_texture_from_data(w,h, 4, image, "imgui_font", &imgui_ctx->font_texture, context)) return false;
    
    nk_font_atlas_end(&imgui_nk_ctx->atlas, nk_handle_ptr(dm_renderer_get_internal_texture_ptr(imgui_ctx->font_texture, context)), &imgui_nk_ctx->tex_null);
    if(imgui_nk_ctx->atlas.default_font) nk_style_set_font(&imgui_nk_ctx->ctx, &imgui_nk_ctx->atlas.default_font->handle);
    else nk_style_set_font(&imgui_nk_ctx->ctx, &chicago->handle);
    
    // style
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
    table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
    table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
    table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
    nk_style_from_table(&imgui_nk_ctx->ctx, table);
    
    return true;
}

void dm_imgui_shutdown(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = &imgui_ctx->internal_context;
    
    nk_font_atlas_clear(&imgui_nk_ctx->atlas);
    nk_buffer_free(&imgui_nk_ctx->cmds);
    nk_free(&imgui_nk_ctx->ctx);
}

// clipboard
void dm_imgui_nuklear_copy(nk_handle usr, const char* text, int len)
{
    dm_platform_clipboard_copy(text, len);
}

void dm_imgui_nuklear_paste(nk_handle usr, struct nk_text_edit* edit)
{
    dm_platform_clipboard_paste(dm_imgui_nuklear_paste_callback, edit);
}

void dm_imgui_nuklear_paste_callback(char* text, int len, void* edit)
{
    nk_textedit_paste(0, text, len);
}

// input
void dm_imgui_input_begin(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = &imgui_ctx->internal_context;
    
    nk_input_begin(&imgui_nk_ctx->ctx);
}

void dm_imgui_input_end(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = &imgui_ctx->internal_context;
    
    nk_input_end(&imgui_nk_ctx->ctx);
}

void dm_imgui_input_event(dm_event e, dm_context* context)
{
    dm_imgui_nuklear_context* imgui_nk_ctx = &context->imgui_context.internal_context;
    
    switch(e.type)
    {
        case DM_EVENT_KEY_UP:
        case DM_EVENT_KEY_DOWN:
        {
            const int  down = e.type==DM_EVENT_KEY_DOWN ? 1 : 0;
            const bool ctrl = dm_input_is_key_pressed(DM_KEY_LCTRL, context) || dm_input_is_key_pressed(DM_KEY_RCTRL, context);
            
            switch(e.key)
            {
                case DM_KEY_LSHIFT:
                case DM_KEY_RSHIFT:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_SHIFT, down);
                break;
                
                case DM_KEY_DELETE:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_DEL, down);
                break;
                
                case DM_KEY_ENTER:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_ENTER, down);
                break;
                
                case DM_KEY_TAB:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TAB, down);
                break;
                
                case DM_KEY_LEFT:
                if(ctrl) nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_WORD_LEFT, down);
                else     nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_LEFT, down);
                break;
                
                case DM_KEY_RIGHT:
                if(ctrl) nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                else     nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_RIGHT, down);
                break;
                
                case DM_KEY_BACKSPACE:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_BACKSPACE, down);
                break;
                
                case DM_KEY_HOME:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_START, down);
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_SCROLL_START, down);
                break;
                
                case DM_KEY_END:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_END, down);
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_SCROLL_END, down);
                break;
                
                case DM_KEY_PAGEDOWN:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_SCROLL_DOWN, down);
                break;
                
                case DM_KEY_PAGEUP:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_SCROLL_UP, down);
                break;
                
                case DM_KEY_C:
                if(ctrl) nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_COPY, down);
                break;
                
                case DM_KEY_V:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_PASTE, down);
                break;
                
                case DM_KEY_X:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_CUT, down);
                break;
                
                case DM_KEY_Z:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_UNDO, down);
                break;
                
                case DM_KEY_R:
                nk_input_key(&imgui_nk_ctx->ctx, NK_KEY_TEXT_REDO, down);
                break;
            }
        }break;
        
        case DM_EVENT_MOUSEBUTTON_DOWN:
        {
            int x,y;
            dm_input_get_mouse_pos(&x,&y, context);
            
            switch(e.button)
            {
                case DM_MOUSEBUTTON_L:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_LEFT, x,y, 1);
                break;
                
                case DM_MOUSEBUTTON_R:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_RIGHT, x,y, 1);
                break;
                
                case DM_MOUSEBUTTON_M:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_MIDDLE, x,y, 1);
                break;
                
                case DM_MOUSEBUTTON_DOUBLE:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_DOUBLE, x,y, 1);
                break;
            }
        } break;
        
        case DM_EVENT_MOUSEBUTTON_UP:
        {
            int x,y;
            dm_input_get_mouse_pos(&x,&y, context);
            
            switch(e.button)
            {
                case DM_MOUSEBUTTON_L:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_DOUBLE, x,y, 0);
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_LEFT, x,y, 0);
                break;
                
                case DM_MOUSEBUTTON_R:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_RIGHT, x,y, 0);
                break;
                
                case DM_MOUSEBUTTON_M:
                nk_input_button(&imgui_nk_ctx->ctx, NK_BUTTON_MIDDLE, x,y, 0);
                break;
            }
        } break;
        
        case DM_EVENT_MOUSE_MOVE:
        {
            uint32_t x,y;
            dm_input_get_mouse_pos(&x,&y,context);
            nk_input_motion(&imgui_nk_ctx->ctx, x,y);
        } break;
        
        case DM_EVENT_MOUSE_SCROLL:
        nk_input_scroll(&imgui_nk_ctx->ctx, nk_vec2(0,e.delta));
        break;
    }
}

void dm_imgui_render(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = &imgui_ctx->internal_context;
    
    // early out if we don't have any commands
    //if(imgui_nk_ctx->cmds.calls==0) return;
    
    dm_render_command_bind_shader(imgui_ctx->shader, context);
    dm_render_command_bind_pipeline(imgui_ctx->pipe, context);
    
    dm_imgui_uni uni = { 0 };
    dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, uni.proj);
#ifdef DM_DIRECTX
    dm_mat4_transpose(uni.proj, uni.proj);
#endif
    dm_render_command_bind_uniform(imgui_ctx->uni, 0, DM_UNIFORM_STAGE_VERTEX, 0, context);
    dm_render_command_update_uniform(imgui_ctx->uni, &uni, sizeof(uni), context);
    
    struct nk_convert_config config = { 0 };
    NK_STORAGE const struct nk_draw_vertex_layout_element vertex_layout[] = {
        { NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    NK_OFFSETOF(dm_imgui_vertex, pos) },
        { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    NK_OFFSETOF(dm_imgui_vertex, tex_coords) },
        { NK_VERTEX_COLOR,    NK_FORMAT_R8G8B8A8, NK_OFFSETOF(dm_imgui_vertex, color) },
        { NK_VERTEX_LAYOUT_END }
    };
    
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(dm_imgui_vertex);
    config.vertex_alignment = NK_ALIGNOF(dm_imgui_vertex);
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.tex_null = imgui_nk_ctx->tex_null;
    
    struct nk_buffer vbuf, ibuf;
    nk_buffer_init_fixed(&vbuf, imgui_nk_ctx->vertices, (size_t)imgui_nk_ctx->max_vertex_buffer);
    nk_buffer_init_fixed(&ibuf, imgui_nk_ctx->indices, (size_t)imgui_nk_ctx->max_index_buffer);
    nk_convert(&imgui_nk_ctx->ctx, &imgui_nk_ctx->cmds, &vbuf, &ibuf, &config);
    
    dm_nk_element_t test[100];
    dm_memcpy(test, ibuf.memory.ptr, sizeof(test));
    dm_memcpy(test, imgui_nk_ctx->indices, sizeof(test));
    size_t d = sizeof(dm_nk_element_t);
    
    dm_render_command_bind_buffer(imgui_ctx->vb, 0, context);
    dm_render_command_update_buffer(imgui_ctx->vb, imgui_nk_ctx->vertices, DM_IMGUI_MAX_VERTICES, 0, context);
    dm_render_command_bind_buffer(imgui_ctx->ib, 0, context);
    dm_render_command_update_buffer(imgui_ctx->ib, imgui_nk_ctx->indices, DM_IMGUI_MAX_INDICES, 0, context);
    
    const struct nk_draw_command* cmd;
    uint32_t offset = 0;
    nk_draw_foreach(cmd, &imgui_nk_ctx->ctx, &imgui_nk_ctx->cmds)
    {
        if(!cmd->elem_count) continue;
        
        dm_render_command_bind_texture(imgui_ctx->font_texture, 0, context);
        dm_render_command_set_scissor_rects((uint32_t)cmd->clip_rect.x, (uint32_t)(cmd->clip_rect.x + cmd->clip_rect.w), (uint32_t)cmd->clip_rect.y, (uint32_t)(cmd->clip_rect.y + cmd->clip_rect.h), context);
        dm_render_command_draw_indexed(cmd->elem_count, offset, 0, context);
        offset += cmd->elem_count;
    }
    
    nk_clear(&imgui_nk_ctx->ctx);
    nk_buffer_clear(&imgui_nk_ctx->cmds);
}