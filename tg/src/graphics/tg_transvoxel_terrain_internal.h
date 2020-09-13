#ifndef TG_VULKAN_TRANSVOXEL_TERRAIN_H
#define TG_VULKAN_TRANSVOXEL_TERRAIN_H

#include "tg_transvoxel_terrain.h"

tg_mesh_h tg_transvoxel_create_regular_mesh(v3i min_coords, v3i node_coords_offset_rel_to_octree, u8 lod, i8* p_voxel_map);

#endif
