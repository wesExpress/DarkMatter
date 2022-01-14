#include "dm_directx_shader.h"

#ifdef DM_DIRECTX

#include "platform/dm_platform_win32.h"
#include <d3dcompiler.h>

bool dm_directx_create_shader(dm_shader* shader, dm_vertex_layout layout, dm_render_pipeline* pipeline)
{
	HRESULT hr;

	dm_internal_pipeline* internal_pipe = (dm_internal_pipeline*)pipeline->interal_pipeline;

	shader->internal_shader = (dm_internal_shader*)dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
	dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;
	internal_shader->vertex_shader = (ID3D11VertexShader*)dm_alloc(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER);
	internal_shader->pixel_shader = (ID3D11PixelShader*)dm_alloc(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER);
	internal_shader->input_layout = (ID3D11InputLayout*)dm_alloc(sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER);

	ID3D11Device* device = internal_pipe->device;
	ID3D11DeviceContext* context = internal_pipe->context;

	// vertex shader
	wchar_t ws[100];
	swprintf(ws, 100, L"%hs", shader->vertex_desc.path);

	ID3DBlob* blob;

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	size_t size = blob->lpVtbl->GetBufferSize(blob);

	DX_ERROR_CHECK(device->lpVtbl->CreateVertexShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->vertex_shader), "ID3D11Device::CreateVertexShader failed!");

	dm_list(D3D11_INPUT_ELEMENT_DESC) desc;
	dm_list_init(&desc, D3D11_INPUT_ELEMENT_DESC);

	for (int i = 0; i < layout.num; i++)
	{
		dm_vertex_attrib_desc attrib_desc = layout.attributes[i];

		DXGI_FORMAT format = dm_vertex_t_to_directx_format(attrib_desc);
		if (format == DXGI_FORMAT_UNKNOWN) return false;

		D3D11_INPUT_ELEMENT_DESC element_desc = {
			.SemanticName = attrib_desc.name,
			.SemanticIndex = 0,
			.Format = format,
			.AlignedByteOffset = attrib_desc.offset,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		};

		// append the element_desc to the array
		dm_list_append(&desc, element_desc);
	}

	DX_ERROR_CHECK(device->lpVtbl->CreateInputLayout(device, desc.array, (UINT)layout.num, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), &internal_shader->input_layout), "ID3D11Device::CreateInputLayout failed!");

	// pixel shader
	swprintf(ws, 100, L"%hs", shader->pixel_desc.path);

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	DX_ERROR_CHECK(device->lpVtbl->CreatePixelShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->pixel_shader), "ID3D11Device::CreatePixelShader failed!");

	DX_RELEASE(blob);
	dm_list_destroy(&desc);

	return true;
}

void dm_directx_delete_shader(dm_shader* shader, dm_internal_pipeline* pipeline)
{
	dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

	DX_RELEASE(internal_shader->vertex_shader);
	DX_RELEASE(internal_shader->pixel_shader);
	DX_RELEASE(internal_shader->input_layout);

	dm_mem_db_adjust(-sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER);
	dm_mem_db_adjust(-sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER);
	dm_mem_db_adjust(-sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER);

	dm_free(shader->internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
}

#endif