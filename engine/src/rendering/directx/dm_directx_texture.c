#include "dm_directx_texture.h"

#ifdef DM_DIRECTX

#include "core/dm_logger.h"
#include "core/dm_mem.h"
#include "dm_directx_enum_conversion.h"

bool dm_directx_create_texture(dm_image* image, dm_internal_renderer* renderer)
{
	HRESULT hr;

	image->internal_texture = dm_alloc(sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
	dm_internal_texture* internal_texture = image->internal_texture;

	DXGI_FORMAT image_format = dm_image_fmt_to_directx_fmt(image->desc.format);
	if (image_format == DXGI_FORMAT_UNKNOWN) return false;

	D3D11_TEXTURE2D_DESC tex_desc = { 0 };
	tex_desc.Width = image->desc.width;
	tex_desc.Height = image->desc.height;
	tex_desc.ArraySize = 1;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.Format = image_format;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	DX_ERROR_CHECK(renderer->device->lpVtbl->CreateTexture2D(renderer->device, &tex_desc, NULL, &internal_texture->texture), "ID3D11Device::CreateTexture2D failed!");
	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);

	renderer->context->lpVtbl->UpdateSubresource(renderer->context, (ID3D11Resource*)internal_texture->texture, 0, NULL, image->data, image->desc.width * 4, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = { 0 };
	view_desc.Format = tex_desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels = -1;

	DX_ERROR_CHECK(renderer->device->lpVtbl->CreateShaderResourceView(renderer->device, (ID3D11Resource*)internal_texture->texture, &view_desc, &internal_texture->view), "ID3D11Device::CreateShaderResourceView failed!");
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_ADD);

	renderer->context->lpVtbl->GenerateMips(renderer->context, internal_texture->view);

	return true;
}

void dm_directx_destroy_texture(dm_image* image)
{
	dm_internal_texture* internal_texture = image->internal_texture;

	DX_RELEASE(internal_texture->texture);
	DX_RELEASE(internal_texture->view);

	dm_mem_db_adjust(sizeof(ID3D11Texture2D), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);
	dm_mem_db_adjust(sizeof(ID3D11ShaderResourceView), DM_MEM_RENDERER_TEXTURE, DM_MEM_ADJUST_SUBTRACT);

	dm_free(image->internal_texture, sizeof(dm_internal_texture), DM_MEM_RENDERER_TEXTURE);
}

void dm_directx_bind_texture(dm_image* image, uint32_t slot, dm_internal_renderer* renderer)
{
	dm_internal_texture* internal_texture = image->internal_texture;
	
	renderer->context->lpVtbl->PSSetShaderResources(renderer->context, slot, 1, &internal_texture->view);
}

#endif