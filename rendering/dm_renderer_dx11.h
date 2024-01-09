#ifndef DM_RENDERER_DX11_H
#define DM_RENDERER_DX11_H

#include "dm.h"

#ifdef DM_DIRECTX

#define COBJMACROS
#include <d3d11_1.h>
#include <dxgi.h>

typedef struct dm_dx11_buffer_t
{
	ID3D11Buffer* buffer;
    D3D11_BIND_FLAG type;
    size_t stride;
} dm_dx11_buffer;

typedef struct dm_dx11_compute_buffer_t
{
    ID3D11Buffer*              buffer;
    ID3D11Buffer*              output_buffer;
    
    union
    {
        ID3D11ShaderResourceView*  resource_view;
        ID3D11UnorderedAccessView* access_view;
    };
    
    dm_compute_buffer_type type;
} dm_dx11_compute_buffer;

typedef struct dm_dx11_shader_t
{
    ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader*  pixel_shader;
	ID3D11InputLayout*  input_layout;
} dm_dx11_shader;

typedef struct dm_dx11_compute_shader_t
{
    ID3D11ComputeShader* shader;
} dm_dx11_compute_shader;

typedef struct dm_dx11_texture_t
{
	ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* view;
    
    // staging texture
    ID3D11Texture2D* staging;
    
    uint32_t width, height;
    bool is_dynamic;
} dm_dx11_texture;

typedef struct dm_dx11_framebuffer_t
{
    ID3D11Texture2D*          render_target;
    ID3D11ShaderResourceView* shader_view;
    ID3D11RenderTargetView*   view;
    ID3D11DepthStencilState*  depth_stencil_buffer;
} dm_dx11_framebuffer;

typedef enum dm_dx11_pipeline_flag_t
{
    DM_DX11_PIPELINE_FLAG_WIREFRAME = 1 << 0,
    DM_DX11_PIPELINE_FLAG_BLEND     = 1 << 1,
    DM_DX11_PIPELINE_FLAG_DEPTH     = 1 << 2,
    DM_DX11_PIPELINE_FLAG_STENCIL   = 1 << 3,
    DM_DX11_PIPELINE_FLAG_UNKNOWN   = 1 << 4
} dm_dx11_pipeline_flag;

typedef struct dm_dx11_pipeline_t
{
	ID3D11DepthStencilState* depth_stencil_state;
    ID3D11RasterizerState*   rasterizer_state;
    ID3D11RasterizerState*   wireframe_state;
    ID3D11BlendState*        blend_state;
    ID3D11SamplerState*      sample_state;
    D3D11_PRIMITIVE_TOPOLOGY default_topology;
    bool wireframe, blend, depth, stencil;
} dm_dx11_pipeline;

// directx renderer
typedef enum dm_dx11_resource_t
{
    DM_DX11_RESOURCE_BUFFER,
    DM_DX11_RESOURCE_SHADER,
    DM_DX11_RESOURCE_TEXTURE,
    DM_DX11_RESOURCE_FRAMEBUFFER,
    DM_DX11_RESOURCE_PIPELINE,
    DM_DX11_RESOURCE_UNKNOWN
} dm_dx11_resource;

typedef struct dm_dx11_renderer
{
    ID3D11Device*           device;
    ID3D11DeviceContext*    context;
    IDXGISwapChain*         swap_chain;
    ID3D11RenderTargetView* render_view;
    ID3D11Texture2D*        render_back_buffer;
    ID3D11DepthStencilView* depth_stencil_view;
    ID3D11Texture2D*        depth_stencil_back_buffer;
    
    HWND hwnd;
	HINSTANCE h_instance;
    
    dm_dx11_buffer         buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_compute_buffer compute_buffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_shader         shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_compute_shader compute_shaders[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_texture        textures[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_framebuffer    framebuffers[DM_RENDERER_MAX_RESOURCE_COUNT];
    dm_dx11_pipeline       pipelines[DM_RENDERER_MAX_RESOURCE_COUNT];
    
    uint32_t buffer_count, compute_buffer_count, shader_count, compute_count, texture_count, framebuffer_count, pipeline_count;
    
    uint32_t active_pipeline, active_shader;
    
#ifdef DM_DEBUG
    ID3D11Debug* debugger;
#endif
} dm_dx11_renderer;

#endif //DM_RENDERER_DX11_H

#endif