#ifndef TG_PHYSICS_H
#define TG_PHYSICS_H

#include "math/tg_math.h"
#include "tg_common.h"

typedef struct tg_raycast_hit
{
	f32    distance;
	v3     hit;
	v3     normal;
} tg_raycast_hit;

f32    tg_distance_point_to_plane_d(v3 point, f32 plane_distance_to_origin, v3 plane_normal); // 'plane_distance_to_origin' is negative, if normal points towards origin
f32    tg_distance_point_to_plane_p(v3 point, v3 point_on_plane, v3 plane_normal);

b32    tg_intersect_aabbs(v3 min0, v3 max0, v3 min1, v3 max1);
b32    tg_intersect_aabbsi(v3i min0, v3i max0, v3i min1, v3i max1);
b32    tg_intersect_aabb_obb(v3 min, v3 max, v3* p_obb_corners);
b32    tg_intersect_aabb_obb_ignore_contact(v3 min, v3 max, v3* p_obb_corners); // Ignores shapes, that don't overlap but simply are in perfect contact
b32    tg_intersect_ray_box(v3 ray_origin, v3 ray_direction, v3 min, v3 max, tg_raycast_hit* p_hit);
b32    tg_intersect_ray_plane(v3 ray_origin, v3 ray_direction, v3 plane_point, v3 plane_normal, tg_raycast_hit* p_hit);
b32    tg_intersect_ray_triangle(v3 ray_origin, v3 ray_direction, v3 tri_p0, v3 tri_p1, v3 tri_p2, tg_raycast_hit* p_hit);

#endif
