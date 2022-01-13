#include "dm_directx_enum_conversion.h"

#ifdef DM_DIRECTX

#include "dm_logger.h"

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

#endif