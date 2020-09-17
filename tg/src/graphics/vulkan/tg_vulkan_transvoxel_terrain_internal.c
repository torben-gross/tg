#include "graphics/tg_transvoxel_terrain_internal.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_graphics_vulkan.h"

tg_mesh_h tg_transvoxel_create_regular_mesh(v3i min_coords, v3i node_coords_offset_rel_to_octree, u8 lod, i8* p_voxel_map)
{
	TG_INVALID_CODEPATH();
	return TG_NULL;
}

#endif
