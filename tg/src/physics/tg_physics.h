#ifndef TG_PHYSICS_H
#define TG_PHYSICS_H

#include "math/tg_math.h"
#include "physics/tg_kd_tree.h"
#include "tg_common.h"

typedef struct tg_raycast_hit
{
	f32    distance;
	v3     hit;
	v3     normal;
} tg_raycast_hit;

b32    tg_intersect_ray_box(v3 ray_origin, v3 ray_direction, v3 min, v3 max, tg_raycast_hit* p_hit);
b32    tg_intersect_ray_plane(v3 ray_origin, v3 ray_direction, v3 plane_point, v3 plane_normal, tg_raycast_hit* p_hit);
b32    tg_intersect_ray_triangle(v3 ray_origin, v3 ray_direction, v3 tri_p0, v3 tri_p1, v3 tri_p2, tg_raycast_hit* p_hit);
b32    tg_raycast_kd_tree(v3 ray_origin, v3 ray_direction, const tg_kd_tree* p_kd_tree, tg_raycast_hit* p_hit);

#endif
