#include "dm.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "Nuklear/nuklear.h"

#include <float.h>

typedef struct dm_imgui_vertex_t
{
    float   pos[2];
    float   tex_coords[2];
    uint8_t color[4];
} dm_imgui_vertex;

typedef struct dm_imgui_uni_t
{
    float proj[4 * 4];
} dm_imgui_uni;

typedef uint16_t dm_nk_element_t;

#define DM_IMGUI_MAX_VERTICES 512 * 1024
#define DM_IMGUI_MAX_INDICES  128 * 1024

#define DM_IMGUI_VERTEX_LEN DM_IMGUI_MAX_VERTICES / sizeof(dm_imgui_vertex)
#define DM_IMGUI_INDEX_LEN  DM_IMGUI_MAX_INDICES / sizeof(dm_nk_element_t)

typedef struct dm_imgui_nuklear_context_t
{
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_buffer cmds;
    
    dm_imgui_vertex vertices[DM_IMGUI_VERTEX_LEN];
    dm_nk_element_t indices[DM_IMGUI_INDEX_LEN];
    
    struct nk_draw_null_texture tex_null;
    uint32_t max_vertex_buffer;
    uint32_t max_index_buffer;
} dm_imgui_nuklear_context;

extern void dm_platform_clipboard_copy(const char* text, int len);
extern void dm_platform_clipboard_paste(void (*callback)(char*,int,void*), void* edit);

void dm_imgui_nuklear_copy(nk_handle usr, const char* text, int len);
void dm_imgui_nuklear_paste(nk_handle usr, struct nk_text_edit* edit);
void dm_imgui_nuklear_paste_callback(char* text, int len, void* edit);

//
bool dm_imgui_init(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    imgui_ctx->internal_context = dm_alloc(sizeof(dm_imgui_nuklear_context));
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    
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
    
    struct nk_font* chicago = nk_font_atlas_add_from_file(&imgui_nk_ctx->atlas, "assets/Chicago.ttf", 13, 0);
    
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
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    
    nk_font_atlas_clear(&imgui_nk_ctx->atlas);
    nk_buffer_free(&imgui_nk_ctx->cmds);
    nk_free(&imgui_nk_ctx->ctx);
    
    dm_free(imgui_ctx->internal_context);
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
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    
    nk_input_begin(&imgui_nk_ctx->ctx);
}

void dm_imgui_input_end(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    
    nk_input_end(&imgui_nk_ctx->ctx);
}

void dm_imgui_input_event(dm_event e, dm_context* context)
{
    dm_imgui_nuklear_context* imgui_nk_ctx = context->imgui_context.internal_context;
    
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
            int x,y;
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
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    
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

void dm_imgui_test(dm_context* context)
{
    dm_imgui_context* imgui_ctx = &context->imgui_context;
    dm_imgui_nuklear_context* imgui_nk_ctx = imgui_ctx->internal_context;
    struct nk_context* ctx = &imgui_nk_ctx->ctx;
    
    struct nk_colorf bg;
    
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                 NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;
        
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed\n");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            bg = nk_color_picker(ctx, bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
            bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
            bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
            bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }
    }
    nk_end(ctx);
    
    // calculator
    if (nk_begin(ctx, "Calculator", nk_rect(10, 10, 180, 250),
                 NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE))
    {
        static int set = 0, prev = 0, op = 0;
        static const char numbers[] = "789456123";
        static const char ops[] = "+-*/";
        static double a = 0, b = 0;
        static double *current = &a;
        
        size_t i = 0;
        int solve = 0;
        {int len; char buffer[256];
            nk_layout_row_dynamic(ctx, 35, 1);
            len = snprintf(buffer, 256, "%.2f", *current);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, buffer, &len, 255, nk_filter_float);
            buffer[len] = 0;
            *current = atof(buffer);}
        
        nk_layout_row_dynamic(ctx, 35, 4);
        for (i = 0; i < 16; ++i) {
            if (i >= 12 && i < 15) {
                if (i > 12) continue;
                if (nk_button_label(ctx, "C")) {
                    a = b = op = 0; current = &a; set = 0;
                } if (nk_button_label(ctx, "0")) {
                    *current = *current*10.0f; set = 0;
                } if (nk_button_label(ctx, "=")) {
                    solve = 1; prev = op; op = 0;
                }
            } else if (((i+1) % 4)) {
                if (nk_button_text(ctx, &numbers[(i/4)*3+i%4], 1)) {
                    *current = *current * 10.0f + numbers[(i/4)*3+i%4] - '0';
                    set = 0;
                }
            } else if (nk_button_text(ctx, &ops[i/4], 1)) {
                if (!set) {
                    if (current != &b) {
                        current = &b;
                    } else {
                        prev = op;
                        solve = 1;
                    }
                }
                op = ops[i/4];
                set = 1;
            }
        }
        if (solve) {
            if (prev == '+') a = a + b;
            if (prev == '-') a = a - b;
            if (prev == '*') a = a * b;
            if (prev == '/') a = a / b;
            current = &a;
            if (set) current = &b;
            b = 0; set = 0;
        }
    }
    nk_end(ctx);
}