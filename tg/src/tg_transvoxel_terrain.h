#ifndef TG_TRANSVOXEL_TERRAIN_H
#define TG_TRANSVOXEL_TERRAIN_H

#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"



typedef struct tg_terrain_block
{
	u32                    transition_mask;
	
	tg_mesh_h              h_block_mesh;
	tg_render_command_h    h_block_render_command;

	tg_mesh_h              ph_transition_meshes[6];
	tg_render_command_h    ph_transition_render_commands[6];
} tg_terrain_block;

typedef struct tg_terrain_octree_node tg_terrain_octree_node;
typedef struct tg_terrain_octree_node
{
	u32                        flags;
	tg_terrain_block           block;
	tg_terrain_octree_node*    pp_children[8];
} tg_terrain_octree_node;

typedef struct tg_terrain_octree
{
	char*                      p_voxel_map;
	v3i                        min_coordinates;
	tg_terrain_octree_node*    p_root;
} tg_terrain_octree;

typedef struct tg_terrain
{
	tg_camera_h           h_camera;
	tg_material_h         h_material;
	tg_terrain_octree*    pp_octrees[9];
} tg_terrain;



tg_terrain    tg_terrain_create(tg_camera_h h_camera);
void          tg_terrain_destroy(tg_terrain* p_terrain);
void          tg_terrain_update(tg_terrain* p_terrain, f32 delta_ms);
void          tg_terrain_render(tg_terrain* p_terrain, tg_camera_h h_camera);

#endif
