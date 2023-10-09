#include "dm.h"
#include <float.h>
#include <assert.h>

bool dm_imgui_init(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    
    if(!dm_renderer_create_dynamic_vertex_buffer(NULL, sizeof(imgui_ctx->vertices), sizeof(dm_imgui_vertex), &imgui_ctx->vb, context)) return false;
    if(!dm_renderer_create_dynamic_index_buffer(NULL, sizeof(imgui_ctx->vertices), &imgui_ctx->ib, context)) return false;
    if(!dm_renderer_create_uniform(sizeof(dm_imgui_uni), DM_UNIFORM_STAGE_VERTEX, &imgui_ctx->uni, context)) return false;
    
    if(!dm_renderer_load_font("assets/fonts/Chicago.ttf", &imgui_ctx->font, context)) return false;
    
    dm_vertex_attrib_desc attrib_descs[] = {
        { .name="POSITION", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, pos), .count=3, .index=0, .normalized=false },
        { .name="TEXCOORD", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, tex_coords), .count=2, .index=0, .normalized=false },
        { .name="COLOR", .data_t=DM_VERTEX_DATA_T_FLOAT, .attrib_class=DM_VERTEX_ATTRIB_CLASS_VERTEX, .stride=sizeof(dm_imgui_vertex), .offset=offsetof(dm_imgui_vertex, color), .count=4, .index=0, .normalized=false }
    };
    
    // pipeline desc
    dm_pipeline_desc pipeline_desc = { 0 };
    pipeline_desc.cull_mode          = DM_CULL_BACK;
    pipeline_desc.winding_order      = DM_WINDING_COUNTER_CLOCK;
    pipeline_desc.primitive_topology = DM_TOPOLOGY_TRIANGLE_LIST;
    
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
    
    // set context state
    imgui_ctx->state = DM_IMGUI_CONTEXT_STATE_ENDED;
    
    // z step
    imgui_ctx->vertex_z_step = 1.0f / DM_IMGUI_MAX_VERTICES;
    
    // set style
    dm_imgui_style style = { 0 };
    
    style.default_window_r = 0.1f;
    style.default_window_g = 0.1f;
    style.default_window_b = 0.15f;
    style.default_window_a = 1;
    
    style.active_window_r = 0.2f;
    style.active_window_g = 0.2f;
    style.active_window_b = 0.25f;
    style.active_window_a = 1;
    
    style.hot_window_r = 0.3f;
    style.hot_window_g = 0.3f;
    style.hot_window_b = 0.35f;
    style.hot_window_a = 1;
    
    style.default_title_bar_r = 0.05f;
    style.default_title_bar_g = 0.05f;
    style.default_title_bar_b = 0.1f;
    style.default_title_bar_a = 1;
    
    style.active_title_bar_r = 0.1f;
    style.active_title_bar_g = 0.1f;
    style.active_title_bar_b = 0.15f;
    style.active_title_bar_a = 1;
    
    style.hot_title_bar_r = 0.2f;
    style.hot_title_bar_g = 0.2f;
    style.hot_title_bar_b = 0.25f;
    style.hot_title_bar_a = 1;
    
    style.title_bar_height = 36;
    
    style.vertical_padding = 5.0f;
    style.horizontal_padding = 5.0f;
    
    style.button_height = 36;
    
    dm_imgui_set_style(style, context);
    
    return true;
}

void dm_imgui_render(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    
    if(imgui_ctx->vertex_count==0) return;
    
    dm_render_command_bind_shader(imgui_ctx->shader, context);
    dm_render_command_bind_pipeline(imgui_ctx->pipe, context);
    
    dm_render_command_bind_buffer(imgui_ctx->vb, 0, context);
    dm_render_command_update_buffer(imgui_ctx->vb, imgui_ctx->vertices, sizeof(dm_imgui_vertex) * imgui_ctx->vertex_count, 0, context);
    
    dm_imgui_uni uni = { 0 };
    dm_mat_ortho(0,(float)context->renderer.width, (float)context->renderer.height,0, -1,1, uni.proj);
#ifdef DM_DIRECTX
    dm_mat4_transpose(uni.proj, uni.proj);
#endif
    dm_render_command_bind_uniform(imgui_ctx->uni, 0, DM_UNIFORM_STAGE_VERTEX, 0, context);
    dm_render_command_update_uniform(imgui_ctx->uni, &uni, sizeof(uni), context);
    
    dm_render_command_draw_arrays(0, imgui_ctx->vertex_count, context);
    
    // reset vertex count
    imgui_ctx->vertex_count = 0;
}

void dm_imgui_set_style(dm_imgui_style style, dm_context* context)
{
    context->imgui_context.style = style;
}

/************
imgui window 
**************/
dm_imgui_window* dm_imgui_get_window(const char* title, dm_imgui_context* imgui_ctx)
{
    dm_hash hash = dm_hash_fnv1a(title);
    uint32_t index = hash % DM_IMGUI_MAX_WINDOWS;
    
    // check existing windows
    dm_imgui_window* window = &imgui_ctx->windows[index];
    
    if(strcmp(window->title, title)==0) return window;
    
    index++;
    if(index>DM_IMGUI_MAX_WINDOWS) index=0;
    window = &imgui_ctx->windows[index];
    
    while(window->state != DM_IMGUI_WINDOW_STATE_INVALID)
    {
        if(strcmp(window->title, title)==0) return window;
        index++;
        if(index>DM_IMGUI_MAX_WINDOWS) index=0;
        window = &imgui_ctx->windows[index];
    }
    
    // this window does not exist
    strcpy(window->title, title);
    window->state = DM_IMGUI_WINDOW_STATE_CREATED;
    
    return window;
}

bool dm_imgui_region_hit(const float x, const float y, const float width, const float height, dm_context* context)
{
    uint32_t mouse_x, mouse_y;
    dm_input_get_mouse_pos(&mouse_x, &mouse_y, context);
    
    const bool in_x = (mouse_x > x) && (mouse_x < x + width);
    const bool in_y = (mouse_y > y) && (mouse_y < y + height);
    
    return in_x && in_y;
}

bool dm_imgui_window_hit(dm_imgui_window* window, dm_context* context)
{
    return dm_imgui_region_hit(window->x, window->y, window->w, window->h, context);
}

/*********
rendering
***********/
void dm_imgui_draw_rect(float x, float y, float width, float height, float r, float g, float b, float a, dm_imgui_context* imgui_ctx)
{
    if(imgui_ctx->vertex_count > DM_IMGUI_MAX_VERTICES - 6) return;
    
    const float z = -1.0f + (imgui_ctx->vertex_count+1) * imgui_ctx->vertex_z_step;
    
    dm_imgui_vertex v0 = { 0 };
    v0.pos[0] = x;
    v0.pos[1] = y;
    //v0.pos[2] = z;
    v0.tex_coords[0] = 0;
    v0.tex_coords[1] = 0;
    v0.color[0] = r;
    v0.color[1] = g;
    v0.color[2] = b;
    v0.color[3] = a;
    
    dm_imgui_vertex v1 = { 0 };
    v1.pos[0] = x + width;
    v1.pos[1] = y;
    //v1.pos[2] = z;
    v1.tex_coords[0] = 1;
    v1.tex_coords[1] = 0;
    v1.color[0] = r;
    v1.color[1] = g;
    v1.color[2] = b;
    v1.color[3] = a;
    
    dm_imgui_vertex v2 = { 0 };
    v2.pos[0] = x + width;
    v2.pos[1] = y + height;
    //v2.pos[2] = z;
    v2.tex_coords[0] = 1;
    v2.tex_coords[1] = 1;
    v2.color[0] = r;
    v2.color[1] = g;
    v2.color[2] = b;
    v2.color[3] = a;
    
    dm_imgui_vertex v3 = { 0 };
    v3.pos[0] = x;
    v3.pos[1] = y + height;
    //v3.pos[2] = z;
    v3.tex_coords[0] = 0;
    v3.tex_coords[1] = 1;
    v3.color[0] = r;
    v3.color[1] = g;
    v3.color[2] = b;
    v3.color[3] = a;
    
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v0;
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v2;
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v1;
    
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v2;
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v0;
    imgui_ctx->vertices[imgui_ctx->vertex_count++] = v3;
}

bool dm_imgui_begin(const char* title, float x, float y, float width, float height, dm_imgui_window_flag flags, dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    
    if(imgui_ctx->state!=DM_IMGUI_CONTEXT_STATE_ENDED) assert(false);
    imgui_ctx->state = DM_IMGUI_CONTEXT_STATE_BEGUN;
    
    // get window
    dm_imgui_window* window = dm_imgui_get_window(title, imgui_ctx);
    
    if(window->state==DM_IMGUI_WINDOW_STATE_CREATED)
    {
        window->x = x;
        window->y = y;
        window->w = width;
        window->h = height;
    }
    
    window->state = DM_IMGUI_WINDOW_STATE_ACTIVE;
    imgui_ctx->active_window = window;
    
    if(dm_imgui_window_hit(window, context))
    {
        if(flags & DM_IMGUI_WINDOW_FLAG_MOVABLE)
        {
            if(dm_imgui_region_hit(window->x,window->y,window->w,imgui_ctx->style.title_bar_height, context) && dm_input_is_mousebutton_pressed(DM_MOUSEBUTTON_L, context))
            {
                imgui_ctx->hot_window = window;
                window->x += dm_input_get_mouse_delta_x(context);
                window->y += dm_input_get_mouse_delta_y(context);
            }
        }
    }
    
    if(imgui_ctx->active_window == window) 
    {
        dm_imgui_draw_rect(window->x,window->y+imgui_ctx->style.title_bar_height,window->w,window->h, imgui_ctx->style.active_window_r, imgui_ctx->style.active_window_g, imgui_ctx->style.active_window_b, imgui_ctx->style.active_window_a, imgui_ctx);
        dm_imgui_draw_rect(window->x,window->y,window->w,imgui_ctx->style.title_bar_height, imgui_ctx->style.active_title_bar_r, imgui_ctx->style.active_title_bar_g, imgui_ctx->style.active_title_bar_b, imgui_ctx->style.active_title_bar_a, imgui_ctx);
    }
    else if(imgui_ctx->hot_window == window) 
    {
        dm_imgui_draw_rect(window->x,window->y+imgui_ctx->style.title_bar_height,window->w,window->h, imgui_ctx->style.hot_window_r, imgui_ctx->style.hot_window_g, imgui_ctx->style.hot_window_b, imgui_ctx->style.hot_window_a, imgui_ctx);
        dm_imgui_draw_rect(window->x,window->y,window->w,imgui_ctx->style.title_bar_height, imgui_ctx->style.hot_title_bar_r, imgui_ctx->style.hot_title_bar_g, imgui_ctx->style.hot_title_bar_b, imgui_ctx->style.hot_title_bar_a, imgui_ctx);
    }
    else
    {
        dm_imgui_draw_rect(window->x,window->y+imgui_ctx->style.title_bar_height,window->w,window->h, imgui_ctx->style.default_window_r, imgui_ctx->style.default_window_g, imgui_ctx->style.default_window_b, imgui_ctx->style.default_window_a, imgui_ctx);
        dm_imgui_draw_rect(window->x,window->y,window->w,imgui_ctx->style.title_bar_height, imgui_ctx->style.default_title_bar_r, imgui_ctx->style.default_title_bar_g, imgui_ctx->style.default_title_bar_b, imgui_ctx->style.default_title_bar_a, imgui_ctx);
    }
    
    imgui_ctx->active_window->y_offset = imgui_ctx->style.vertical_padding + imgui_ctx->style.title_bar_height;
    imgui_ctx->active_window->x_offset = imgui_ctx->style.horizontal_padding;
    
    return true;
}

void dm_imgui_end(dm_context* context)
{
    assert(context->imgui_context.state==DM_IMGUI_CONTEXT_STATE_BEGUN);
    context->imgui_context.state = DM_IMGUI_CONTEXT_STATE_ENDED;
    
    context->imgui_context.active_window->y_offset = 0;
    context->imgui_context.active_window->x_offset = 0;
    
    context->imgui_context.active_window = NULL;
    context->imgui_context.hot_window = NULL;
}

bool dm_imgui_button(const char* title, dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    assert(imgui_ctx->active_window);
    
    float width = imgui_ctx->active_window->w - 2.0f * imgui_ctx->style.horizontal_padding;
    float x = imgui_ctx->active_window->x + imgui_ctx->style.horizontal_padding;
    float y = imgui_ctx->active_window->y + imgui_ctx->active_window->y_offset;
    
    dm_imgui_draw_rect(x,y, width, imgui_ctx->style.button_height, 1,1,1,1, imgui_ctx);
    
    imgui_ctx->active_window->y_offset += imgui_ctx->style.button_height + imgui_ctx->style.vertical_padding;
    
    return true;
}
