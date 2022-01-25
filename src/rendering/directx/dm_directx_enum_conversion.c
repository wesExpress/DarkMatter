#include "dm_directx_enum_conversion.h"

#ifdef DM_DIRECTX

#include "core/dm_logger.h"

D3D11_CULL_MODE dm_cull_to_directx_cull(dm_cull_mode dm_mode)
{
	switch (dm_mode)
	{
	case DM_CULL_FRONT_BACK:
	case DM_CULL_FRONT: return D3D11_CULL_FRONT;
	case DM_CULL_BACK: return D3D11_CULL_BACK;
	default:
		DM_LOG_FATAL("Unknown cull mode!");
		return D3D11_CULL_NONE;
	}
}

D3D11_PRIMITIVE_TOPOLOGY dm_toplogy_to_directx_topology(dm_primitive_topology dm_top)
{
	switch (dm_top)
	{
	case DM_TOPOLOGY_POINT_LIST: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case DM_TOPOLOGY_LINE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case DM_TOPOLOGY_LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case DM_TOPOLOGY_TRIANGLE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case DM_TOPOLOGY_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default:
		DM_LOG_FATAL("Unknown primitive topology!");
		return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

D3D11_USAGE dm_buffer_usage_to_directx(dm_buffer_usage usage)
{
	switch (usage)
	{
	case DM_BUFFER_USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
	case DM_BUFFER_USAGE_STATIC: return D3D11_USAGE_IMMUTABLE;
	case DM_BUFFER_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
	default:
		DM_LOG_FATAL("Unknown buffer usage!");
		return D3D11_USAGE_STAGING+1;
	}
}

D3D11_BIND_FLAG dm_buffer_type_to_directx(dm_buffer_type type)
{
	switch (type)
	{
	case DM_BUFFER_TYPE_VERTEX: return D3D11_BIND_VERTEX_BUFFER;
	case DM_BUFFER_TYPE_INDEX: return D3D11_BIND_INDEX_BUFFER;
	case DM_BUFFER_TYPE_CONSTANT: return D3D11_BIND_CONSTANT_BUFFER;
	default:
		DM_LOG_FATAL("Unknown buffer type!");
		return D3D11_BIND_VIDEO_ENCODER+1;
	}
}

D3D11_CPU_ACCESS_FLAG dm_buffer_cpu_access_to_directx(dm_buffer_cpu_access access)
{
	switch (access)
	{
	case DM_BUFFER_CPU_READ: return D3D11_CPU_ACCESS_READ;
	case DM_BUFFER_CPU_WRITE: return D3D11_CPU_ACCESS_WRITE;
	default:
		DM_LOG_FATAL("Unknown cpu access!");
		return D3D11_CPU_ACCESS_READ+1;
	}
}

DXGI_FORMAT dm_vertex_t_to_directx_format(dm_vertex_attrib_desc desc)
{
	switch (desc.data_t)
	{
	case DM_VERTEX_DATA_T_BYTE:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R8_SINT;
		case 2: return DXGI_FORMAT_R8G8_SINT;
		case 4: return DXGI_FORMAT_R8G8B8A8_SINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_UBYTE:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R8_UINT;
		case 2: return DXGI_FORMAT_R8G8_UINT;
		case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_SHORT:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R16_SINT;
		case 2: return DXGI_FORMAT_R16G16_SINT;
		case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_USHORT:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R16_UINT;
		case 2: return DXGI_FORMAT_R16G16_UINT;
		case 4: return DXGI_FORMAT_R16G16B16A16_UINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_INT:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R32_SINT;
		case 2: return DXGI_FORMAT_R32G32_SINT;
		case 3: return DXGI_FORMAT_R32G32B32_SINT;
		case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_UINT:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R32_UINT;
		case 2: return DXGI_FORMAT_R32G32_UINT;
		case 3: return DXGI_FORMAT_R32G32B32_UINT;
		case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
		default:
			break;
		}
	}
	case DM_VERTEX_DATA_T_DOUBLE:
	case DM_VERTEX_DATA_T_FLOAT:
	{
		switch (desc.count)
		{
		case 1: return DXGI_FORMAT_R32_FLOAT;
		case 2: return DXGI_FORMAT_R32G32_FLOAT;
		case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:
			break;
		}
	}
	default:
		break;
	}

	DM_LOG_FATAL("Unknown vertex format type!");
	return DXGI_FORMAT_UNKNOWN;
}

dm_comp_to_directx_comp(dm_comparison dm_comp)
{
	switch (dm_comp)
	{
	case DM_COMPARISON_ALWAYS: return D3D11_COMPARISON_ALWAYS;
	case DM_COMPARISON_NEVER: return D3D11_COMPARISON_NEVER;
	case DM_COMPARISON_EQUAL: return D3D11_COMPARISON_EQUAL;
	case DM_COMPARISON_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
	case DM_COMPARISON_LESS: return D3D11_COMPARISON_LESS;
	case DM_COMPARISON_LEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
	case DM_COMPARISON_GREATER: return D3D11_COMPARISON_GREATER;
	case DM_COMPARISON_GEQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
	default:
		DM_LOG_FATAL("Unknown comparison function!");
		return D3D11_COMPARISON_ALWAYS + 1;
	}
}

DXGI_FORMAT dm_image_fmt_to_directx_fmt(dm_texture_format dm_fmt)
{
	switch (dm_fmt)
	{
	case DM_TEXTURE_FORMAT_RGB: 
	case DM_TEXTURE_FORMAT_RGBA: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	default:
		DM_LOG_FATAL("Unknown texture format!");
		return DXGI_FORMAT_UNKNOWN;
	}
}

#endif