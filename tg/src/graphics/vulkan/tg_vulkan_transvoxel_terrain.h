#ifndef TG_VULKAN_TRANSVOXEL_TERRAIN_H
#define TG_VULKAN_TRANSVOXEL_TERRAIN_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_entity.h"
#include "tg_transvoxel_terrain.h"



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



typedef struct tg_transvoxel_vertex
{
	v3     primary_position;
	v3     normal;
	v3     secondary_position;
	i32    border_mask;
} tg_transvoxel_vertex;

// TODO: i want all of this to be an entity and have it multiple meshes
typedef struct tg_transvoxel_block
{
	tg_entity                entity;
	tg_entity                p_transition_entities[6];
	
	i32                      transition_mask;

	u16                      vertex_count;
	u16                      index_count;
	u16                      p_transition_vertex_counts[6];
	u16                      p_transition_index_counts[6];
	
	tg_transvoxel_vertex*    p_vertices;
	u16*                     p_indices;
	tg_transvoxel_vertex*    pp_transition_vertices[6];
	u16*                     pp_transition_indices[6];
} tg_transvoxel_block;



typedef struct tg_transvoxel_node tg_transvoxel_node;
typedef struct tg_transvoxel_node
{
	tg_transvoxel_block    node;
	tg_transvoxel_node*    pp_children[8];
} tg_transvoxel_node;

typedef struct tg_transvoxel_octree
{
	void*                 p_compressed_voxel_map;
	v3i                   min_coordinates;
	tg_transvoxel_node    root;
} tg_transvoxel_octree;



typedef struct tg_transvoxel_terrain
{
	tg_camera_h             h_camera;
	tg_transvoxel_octree    p_octrees[9]; // TODO: one octree per chunk/block
} tg_transvoxel_terrain;

#endif

#endif
