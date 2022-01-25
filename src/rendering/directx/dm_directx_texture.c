#include "dm_directx_texture.h"

#ifdef DM_DIRECTX

#include "core/dm_logger.h"
#include "core/dm_mem.h"
#include "dm_directx_enum_conversion.h"

bool dm_directx_create_texture(dm_texture* texture, dm_internal_renderer* renderer)
{
	HRESULT hr;

	texture->internal_texture = dm_alloc(sizeof(dm_internal_shader), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = texture->internal_texture;

	DXGI_FORMAT image_format = dm_image_fmt_to_directx_fmt(texture->desc.format);
	if (image_format == DXGI_FORMAT_UNKNOWN) return false;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = texture->desc.width;
	desc.Height = texture->desc.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Format = image_format;

	D3D11_SUBRESOURCE_DATA data = { 0 };
	data.pSysMem = texture->data;
	data.SysMemPitch = texture->desc.width * texture->desc.n_channels;

	DX_ERROR_CHECK(renderer->device->lpVtbl->CreateTexture2D(renderer->device, &desc, &data, &internal_texture->texture), "ID3D11Device::CreateTexture2D failed!");
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);

	DX_ERROR_CHECK(renderer->device->lpVtbl->CreateShaderResourceView(renderer->device, (ID3D11Resource*)internal_texture->texture, NULL, &internal_texture->view), "ID3D11Device::CreateShaderResourceView failed!");
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);

	return true;
}

void dm_directx_destroy_texture(dm_texture* texture)
{
	dm_internal_texture* internal_texture = texture->internal_texture;

	DX_RELEASE(internal_texture->texture);
	DX_RELEASE(internal_texture->view);

	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);

	dm_free(texture->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

#endif