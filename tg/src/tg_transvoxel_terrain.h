#ifndef TG_TRANSVOXEL_TERRAIN_H
#define TG_TRANSVOXEL_TERRAIN_H

#include "tg_entity.h"



#define TG_TRANSVOXEL_PADDING_PER_SIDE                      1

#define TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE                  16
#define TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE_WITH_PADDING     18
#define TG_TRANSVOXEL_CELLS_PER_BLOCK                       4096 // 16^3
#define TG_TRANSVOXEL_CELLS_PER_BLOCK_WITH_PADDING          5832 // 18^3

#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE                 17
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_SIDE_WITH_PADDING    19
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK                      4913 // 17^3
#define TG_TRANSVOXEL_VOXELS_PER_BLOCK_WITH_PADDING         6859 // 19^3



#define TG_TRANSVOXEL_MAX_TRIANGLES_PER_BLOCK               (5 * TG_TRANSVOXEL_CELLS_PER_BLOCK)
#define TG_TRANSVOXEL_MAX_VERTICES_PER_BLOCK                (15 * TG_TRANSVOXEL_CELLS_PER_BLOCK)

#define TG_TRANSVOXEL_OCTREE_STRIDE_IN_CELLS                256 // 16 * 16
#define TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS               257 // 16 * 16 + 1

#define TG_TRANSVOXEL_VOXEL_MAP_STRIDE                      TG_TRANSVOXEL_OCTREE_STRIDE_IN_VOXELS // 257
#define TG_TRANSVOXEL_VOXEL_MAP_PADDING                     TG_TRANSVOXEL_CELLS_PER_BLOCK_SIDE // 16
#define TG_TRANSVOXEL_VOXEL_MAP_STRIDE_WITH_PADDING         289 // 257 + 2 * 16
#define TG_TRANSVOXEL_VOXEL_MAP_VOXELS                      24137569 // 289^3
#define TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, x, y, z)      ((voxel_map)[83521 * (z) + 289 * (y) + (x)]) // 289 * 289 * z + 289 * y + x
#define TG_TRANSVOXEL_VOXEL_MAP_AT_V3I(voxel_map, v)        TG_TRANSVOXEL_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)

#define TG_TRANSVOXEL_VIEW_DISTANCE_IN_OCTREES              1



typedef struct tg_terrain_block
{
	tg_material_h          h_material;

	u32                    transition_mask;
	tg_uniform_buffer_h    h_transition_uniform_buffer;
	
	tg_mesh_h              h_block_mesh;
	tg_render_command_h    h_block_render_command;

	tg_mesh_h              ph_transition_meshes[6];
	tg_render_command_h    ph_transition_render_commands[6];
} tg_terrain_block;

typedef struct tg_terrain_entity_node tg_terrain_entity_node;
typedef struct tg_terrain_entity_node
{
	u32                        flags;
	tg_terrain_block           block;
	tg_terrain_entity_node*    pp_children[8];
} tg_terrain_entity_node;

typedef struct tg_terrain_entity_octree
{
	void*                      p_voxel_map;
	v3i                        min_coordinates;
	tg_terrain_entity_node*    p_root;
} tg_terrain_entity_octree;

typedef struct tg_terrain_entity_data
{
	tg_entity                   entity;
	tg_camera_h                 h_camera;
	tg_terrain_entity_octree    p_octrees[9];
} tg_terrain_entity_data;



tg_entity tg_transvoxel_terrain_create(tg_camera_h h_camera);
void tg_transvoxel_terrain_destroy(tg_entity* p_entity);

#endif
