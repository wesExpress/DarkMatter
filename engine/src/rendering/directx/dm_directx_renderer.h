#ifndef __DM_DIRECTX_RENDERER_H__
#define __DM_DIRECTX_RENDERER_H__

#include "core/dm_defines.h"

#ifdef DM_PLATFORM_WIN32

#include "platform/dm_platform_win32.h"

#include "rendering/dm_renderer.h"

#include <d3d11_1.h>
#include <dxgi.h>
#include <stdbool.h>

#define DX_ERROR_CHECK(HRCALL, ERROR_MSG) hr = HRCALL; if(hr!=S_OK){ DM_LOG_FATAL(ERROR_MSG); DM_LOG_FATAL(dm_get_win32_error_msg(hr)); return false; }
#define DX_RELEASE(OBJ) if(OBJ) { OBJ->lpVtbl->Release(OBJ); }

typedef struct dm_directx_buffer
{
	ID3D11Buffer* buffer;
} dm_directx_buffer;

typedef struct dm_directx_shader
{
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* input_layout;
} dm_directx_shader;

typedef struct dm_directx_texture
{
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* view;
} dm_directx_texture;

typedef struct dm_directx_renderer
{
	HWND hwnd;
	HINSTANCE h_instance;

	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swap_chain;

#if DM_DEBUG
	ID3D11Debug* debugger;
#endif
} dm_directx_renderer;

typedef struct dm_directx_pipeline
{
	ID3D11RenderTargetView* render_view;
	ID3D11Texture2D* render_back_buffer;
	ID3D11DepthStencilView* depth_stencil_view;
	ID3D11Texture2D* depth_stencil_back_buffer;
	ID3D11DepthStencilState* depth_stencil_state;
	dm_list* vertex_buffers;
} dm_directx_pipeline;

typedef struct dm_directx_render_pass
{
	ID3D11RasterizerState* rasterizer_state;
	ID3D11SamplerState* sample_state;
	D3D11_PRIMITIVE_TOPOLOGY topology;
} dm_directx_render_pass;

#endif

#endif