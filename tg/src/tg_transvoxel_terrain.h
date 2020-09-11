#ifndef TG_TRANSVOXEL_TERRAIN_H
#define TG_TRANSVOXEL_TERRAIN_H

#include "graphics/tg_graphics.h"



#define TG_TERRAIN_CELLS_PER_BLOCK_SIDE                16
#define TG_TERRAIN_CELLS_PER_BLOCK                     4096 // 16^3
#define TG_TERRAIN_VOXELS_PER_BLOCK_SIDE               17
#define TG_TERRAIN_OCTREE_STRIDE_IN_CELLS              256 // 16 * 16
#define TG_TERRAIN_VOXEL_MAP_STRIDE                    257
#define TG_TERRAIN_VOXEL_MAP_VOXELS                    16974593 // 257^3
#define TG_TERRAIN_VOXEL_MAP_AT(voxel_map, x, y, z)    ((voxel_map)[66049 * (z) + 257 * (y) + (x)]) // 257 * 257 * z + 257 * y + x
#define TG_TERRAIN_VOXEL_MAP_AT_V3I(voxel_map, v)      TG_TERRAIN_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)
#define TG_TERRAIN_MAX_LOD                             4 // 5 - 1
#define TG_TERRAIN_NODES_PER_OCTREE                    4681 // 8^0 + 8^1 + 8^2 + 8^3 + 8^4
#define TG_TERRAIN_NODES_PER_OCTREE_CEIL               4688

#define TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES            0
#define TG_TERRAIN_OCTREES                             9 // (1 + 2 * 1)^2



typedef struct tg_terrain_octree_node
{
	//tg_mesh_h              h_block_mesh; // TODO: create getter for mesh, so i dont have to save it in here for destruction
	//tg_mesh_h              ph_transition_meshes[6]; // TODO: create getter for mesh, so i dont have to save it in here for destruction
	tg_render_command_h    h_block_render_command;
	tg_render_command_h    ph_transition_render_commands[6];
} tg_terrain_octree_node;

typedef struct tg_terrain_octree
{
	v3i                       min_coords;
	tg_terrain_octree_node    p_nodes[TG_TERRAIN_NODES_PER_OCTREE];
	u8                        p_should_split_bitmap[TG_TERRAIN_NODES_PER_OCTREE_CEIL / 8];
} tg_terrain_octree;

typedef struct tg_terrain
{
	tg_camera*                p_camera;
	tg_material_h             h_material;
	tg_terrain_octree         p_octrees[TG_TERRAIN_OCTREES];
} tg_terrain;



tg_terrain*   tg_terrain_create(tg_camera* p_camera);
void          tg_terrain_destroy(tg_terrain* p_terrain);
void          tg_terrain_update(tg_terrain* p_terrain);
void          tg_terrain_render(tg_terrain* p_terrain, tg_renderer_h h_renderer);

#endif
