#include "dm_directx_shader.h"

#ifdef DM_DIRECTX

#include "platform/dm_platform_win32.h"
#include "dm_directx_enum_conversion.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"
#include <d3dcompiler.h>

bool dm_directx_create_input_element(dm_vertex_attrib_desc attrib_desc, D3D11_INPUT_ELEMENT_DESC* element_desc);

bool dm_directx_create_shader(dm_shader* shader, dm_vertex_layout layout, dm_internal_renderer* renderer, dm_render_pipeline* pipeline)
{
	HRESULT hr;

	dm_internal_pipeline* internal_pipe = pipeline->interal_pipeline;

	shader->internal_shader = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
	dm_internal_shader* internal_shader = shader->internal_shader;

	ID3D11Device* device = renderer->device;
	ID3D11DeviceContext* context = renderer->context;

	// vertex shader
	wchar_t ws[100];
	swprintf(ws, 100, L"%hs", shader->vertex_desc.path);

	ID3DBlob* blob;

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	DX_ERROR_CHECK(device->lpVtbl->CreateVertexShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->vertex_shader), "ID3D11Device::CreateVertexShader failed!");
	dm_mem_db_adjust(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);

	// input layout
	dm_list* desc = dm_list_create(sizeof(D3D11_INPUT_ELEMENT_DESC), 0);
	uint32_t count = 0;
	for (uint32_t i = 0; i < layout.num; i++)
	{
		dm_vertex_attrib_desc attrib_desc = layout.attributes[i];

		if (attrib_desc.data_t == DM_VERTEX_DATA_T_MATRIX)
		{
			for (uint32_t j = 0; j < attrib_desc.count; j++)
			{
				dm_vertex_attrib_desc sub_desc = attrib_desc;
				sub_desc.data_t = DM_VERTEX_DATA_T_FLOAT;
				sub_desc.offset = sub_desc.offset + sizeof(float) * j;
				
				D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
				if (!dm_directx_create_input_element(sub_desc, &element_desc)) return false;
				element_desc.SemanticIndex = j;
				element_desc.AlignedByteOffset = sizeof(dm_vec4) * j;

				dm_list_append(desc, &element_desc);
				count++;
			}
		}
		else
		{
			D3D11_INPUT_ELEMENT_DESC element_desc = { 0 };
			if (!dm_directx_create_input_element(attrib_desc, &element_desc)) return false;

			// append the element_desc to the array
			dm_list_append(desc, &element_desc);
			count++;
		}	
	}

	DX_ERROR_CHECK(device->lpVtbl->CreateInputLayout(device, desc->data, (UINT)count, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), &internal_shader->input_layout), "ID3D11Device::CreateInputLayout failed!");
	dm_mem_db_adjust(sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);

	// pixel shader
	swprintf(ws, 100, L"%hs", shader->pixel_desc.path);

	DX_ERROR_CHECK(D3DReadFileToBlob(ws, &blob), "D3DReadFileToBlob failed!");

	DX_ERROR_CHECK(device->lpVtbl->CreatePixelShader(device, blob->lpVtbl->GetBufferPointer(blob), blob->lpVtbl->GetBufferSize(blob), NULL, &internal_shader->pixel_shader), "ID3D11Device::CreatePixelShader failed!");
	dm_mem_db_adjust(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_ADD);

	DX_RELEASE(blob);
	dm_list_destroy(desc);

	return true;
}

void dm_directx_delete_shader(dm_shader* shader, dm_internal_pipeline* pipeline)
{
	dm_internal_shader* internal_shader = (dm_internal_shader*)shader->internal_shader;

	DX_RELEASE(internal_shader->vertex_shader);
	DX_RELEASE(internal_shader->pixel_shader);
	DX_RELEASE(internal_shader->input_layout);

	dm_mem_db_adjust(sizeof(ID3D11VertexShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11PixelShader), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11InputLayout), DM_MEM_RENDERER_SHADER, DM_MEM_ADJUST_SUBTRACT);

	dm_free(shader->internal_shader, sizeof(dm_internal_shader), DM_MEM_RENDERER_SHADER);
}

bool dm_directx_create_input_element(dm_vertex_attrib_desc attrib_desc, D3D11_INPUT_ELEMENT_DESC* element_desc)
{
	DXGI_FORMAT format = dm_vertex_t_to_directx_format(attrib_desc);
	if (format == DXGI_FORMAT_UNKNOWN) return false;
	D3D11_INPUT_CLASSIFICATION input_class = dm_vertex_class_to_directx_class(attrib_desc.attrib_class);
	if (input_class == DM_VERTEX_ATTRIB_CLASS_UNKNOWN) return false;

	element_desc->SemanticName = attrib_desc.name;
	element_desc->Format = format;
	element_desc->AlignedByteOffset = attrib_desc.offset;
	element_desc->InputSlotClass = input_class;
	if (input_class == D3D11_INPUT_PER_INSTANCE_DATA)
	{
		element_desc->InstanceDataStepRate = 1;
		element_desc->InputSlot = 1;
	}

	return true;
}

#endif