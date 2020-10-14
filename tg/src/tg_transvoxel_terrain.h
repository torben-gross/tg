#ifndef TG_TRANSVOXEL_TERRAIN_H
#define TG_TRANSVOXEL_TERRAIN_H

#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"



#define TG_TERRAIN_NODE_CELL_STRIDE                    16
#define TG_TERRAIN_NODE_CELLS                          (TG_TERRAIN_NODE_CELL_STRIDE * TG_TERRAIN_NODE_CELL_STRIDE * TG_TERRAIN_NODE_CELL_STRIDE)

#define TG_TERRAIN_OCTREE_CELL_STRIDE                  256
#define TG_TERRAIN_OCTREE_NODES                        4681 // 8^0 + 8^1 + 8^2 + 8^3 + 8^4
#define TG_TERRAIN_OCTREE_NODES_CEIL                   4688

#define TG_TERRAIN_VOXEL_MAP_STRIDE                    257
#define TG_TERRAIN_VOXEL_MAP_VOXELS                    16974593 // 257^3
#define TG_TERRAIN_VOXEL_MAP_AT(voxel_map, x, y, z)    ((voxel_map)[(TG_TERRAIN_VOXEL_MAP_STRIDE * TG_TERRAIN_VOXEL_MAP_STRIDE) * (z) + TG_TERRAIN_VOXEL_MAP_STRIDE * (y) + (x)])
#define TG_TERRAIN_VOXEL_MAP_AT_V3I(voxel_map, v)      TG_TERRAIN_VOXEL_MAP_AT(voxel_map, (v).x, (v).y, (v).z)

#define TG_TERRAIN_MAX_LOD                             4
#define TG_TERRAIN_NODE_DESTROY_AFTER_SECONDS          4.0f

#define TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES            1
#define TG_TERRAIN_OCTREES                             (1 + 4 * TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES * (1 + TG_TERRAIN_VIEW_DISTANCE_IN_OCTREES))

#define TG_TERRAIN_DESTROY_RENDER_COMMAND_COUNT        1024



typedef enum tg_terrain_flag
{
	TG_TERRAIN_FLAG_CONSTRUCTED    = 0x1,
	TG_TERRAIN_FLAG_DESTROY        = 0x2,
	TG_TERRAIN_FLAG_UPDATE         = 0x4
} tg_terrain_flag;

typedef struct tg_terrain_octree_node
{
	u32                    flags;
	f32                    seconds_since_startup_on_destroy_mark;
	tg_render_command_h    h_block_render_command;
	tg_render_command_h    ph_transition_render_commands[6];
} tg_terrain_octree_node;

typedef struct tg_terrain_octree
{
	u32                       flags;
	v3i                       min_coords;
	i8*                       p_voxel_map;
	tg_terrain_octree_node    p_nodes[TG_TERRAIN_OCTREE_NODES];
	u8                        p_should_render_bitmap_temp[TG_TERRAIN_OCTREE_NODES_CEIL / 8];
	u8                        p_should_render_bitmap_stable[TG_TERRAIN_OCTREE_NODES_CEIL / 8];
} tg_terrain_octree;

typedef struct tg_terrain
{
	tg_camera*                   p_camera;
	v3                           last_camera_position;
	tg_material_h                h_material;
	tg_read_write_lock           render_read_write_lock;
	v3*                          p_position_buffer;
	v3*                          p_normal_buffer;
	b32                          is_thread_running;
	tg_thread_h                  h_thread;
	tg_terrain_octree            p_octrees[TG_TERRAIN_OCTREES];

	i32                          update_node_count;
	tg_event_h                   h_event;
	u8                           destroy_render_command_count;
	tg_render_command_h          ph_destroy_render_commands[TG_TERRAIN_DESTROY_RENDER_COMMAND_COUNT];
} tg_terrain;



tg_terrain*   tg_terrain_create(tg_camera* p_camera);
void          tg_terrain_destroy(tg_terrain* p_terrain);
void          tg_terrain_render(tg_terrain* p_terrain, tg_renderer_h h_renderer);
void          tg_terrain_shape(tg_terrain* p_terrain, v3 position, f32 radius_in_meters, i8 change_per_second);

#endif
