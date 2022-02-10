#include "dm_directx_buffer.h"

#ifdef DM_DIRECTX

#include "dm_directx_enum_conversion.h"
#include "core/dm_mem.h"
#include "core/dm_logger.h"

bool dm_directx_create_buffer(dm_buffer* buffer, void* data, dm_directx_renderer* renderer)
{
	HRESULT hr;

	buffer->internal_buffer = dm_alloc(sizeof(dm_directx_buffer), DM_MEM_RENDERER_BUFFER);
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;

	ID3D11Device* device = renderer->device;
	ID3D11DeviceContext* context = renderer->context;

	D3D11_USAGE usage = dm_buffer_usage_to_directx(buffer->desc.usage);
	if (usage == D3D11_USAGE_STAGING + 1) return false;
	D3D11_BIND_FLAG type = dm_buffer_type_to_directx(buffer->desc.type);
	if (type == D3D11_BIND_VIDEO_ENCODER + 1) return false;
	D3D11_CPU_ACCESS_FLAG cpu_access = dm_buffer_cpu_access_to_directx(buffer->desc.cpu_access);
	if (cpu_access == D3D11_CPU_ACCESS_READ + 1) return false;

	D3D11_BUFFER_DESC desc = { 0 };
	desc.Usage = usage;
	desc.BindFlags = type;
	desc.ByteWidth = buffer->desc.buffer_size;
	desc.StructureByteStride = buffer->desc.elem_size;
	if (usage == D3D11_USAGE_DYNAMIC) desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA sd = { 0 };
	sd.pSysMem = data;

	if (data)
	{
		DX_ERROR_CHECK(device->lpVtbl->CreateBuffer(device, &desc, &sd, &internal_buffer->buffer), "ID3D11Device::CreateBuffer failed!");
	}
	else
	{
		DX_ERROR_CHECK(device->lpVtbl->CreateBuffer(device, &desc, 0, &internal_buffer->buffer), "ID3D11Device::CreateBuffer failed!");
	}
	dm_mem_db_adjust(sizeof(ID3D11Buffer), DM_MEM_RENDERER_BUFFER, DM_MEM_ADJUST_ADD);

	return true;
}

void dm_directx_delete_buffer(dm_buffer* buffer)
{
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;

	DX_RELEASE(internal_buffer->buffer);

	dm_mem_db_adjust(sizeof(ID3D11Buffer), DM_MEM_RENDERER_BUFFER, DM_MEM_ADJUST_SUBTRACT);
	
	dm_free(buffer->internal_buffer, sizeof(dm_directx_buffer), DM_MEM_RENDERER_BUFFER);
}

void dm_directx_bind_buffer(dm_buffer* buffer, uint32_t slot, dm_directx_renderer* renderer)
{
	ID3D11DeviceContext* context = renderer->context;
	dm_directx_buffer* internal_buffer = buffer->internal_buffer;

	UINT stride = buffer->desc.elem_size;
	UINT offset = 0;

	switch (buffer->desc.type)
	{
	case DM_BUFFER_TYPE_VERTEX: 
		context->lpVtbl->IASetVertexBuffers(context, slot, 1, &internal_buffer->buffer, &stride, &offset);
		break;
	case DM_BUFFER_TYPE_INDEX: 
		context->lpVtbl->IASetIndexBuffer(context, internal_buffer->buffer, DXGI_FORMAT_R32_UINT, 0);
		break;
	case DM_BUFFER_TYPE_CONSTANT: 
		context->lpVtbl->VSSetConstantBuffers(context, slot, 1, &internal_buffer->buffer);
		break;
	default:
		DM_LOG_WARN("Trying to bind a buffer of unknown type! Shouldn't be here...");
		break;
	}
}

void dm_directx_bind_vertex_buffers(dm_list* buffers, dm_directx_renderer* renderer)
{
	ID3D11DeviceContext* context = renderer->context;
	
	ID3D11Buffer** buffer_list = dm_alloc(sizeof(ID3D11Buffer*) * buffers->count, DM_MEM_RENDERER_BUFFER);
	UINT* stride_list = dm_alloc(sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	UINT* offset_list = dm_alloc(sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);

	for (uint32_t i = 0; i < buffers->count; i++)
	{
		dm_buffer* buffer = dm_list_at(buffers, i);
		dm_directx_buffer* internal_buffer = buffer->internal_buffer;
		buffer_list[i] = internal_buffer->buffer;
		stride_list[i] = buffer->desc.elem_size;
		offset_list[i] = 0;
	}

	context->lpVtbl->IASetVertexBuffers(context, 0, buffers->count, buffer_list, stride_list, offset_list);

	dm_free(offset_list, sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	dm_free(stride_list, sizeof(UINT) * buffers->count, DM_MEM_RENDERER_BUFFER);
	dm_free(buffer_list, sizeof(ID3D11Buffer*) * buffers->count, DM_MEM_RENDERER_BUFFER);
}

#endif