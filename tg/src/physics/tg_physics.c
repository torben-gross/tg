#include "physics/tg_physics.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"

f32 tg_distance_point_to_plane_d(v3 point, f32 plane_distance_to_origin, v3 plane_normal)
{
    TG_ASSERT(tgm_v3_magsqr(plane_normal) > 0.95f && tgm_v3_magsqr(plane_normal) < 1.05f); // We assume the normal to be normalized

    const f32 result = tgm_v3_dot(plane_normal, point) - plane_distance_to_origin;
    return result;
}

f32 tg_distance_point_to_plane_p(v3 point, v3 point_on_plane, v3 plane_normal)
{
    TG_ASSERT(tgm_v3_magsqr(plane_normal) > 0.95f && tgm_v3_magsqr(plane_normal) < 1.05f); // We assume the normal to be normalized

    const f32 l = tgm_v3_mag(point_on_plane);
    const v3  n = tgm_v3_divf(point_on_plane, l);
    const f32 d = l * tgm_v3_dot(n, plane_normal);

    const f32 result = tg_distance_point_to_plane_d(point, d, plane_normal);
    return result;
}



b32 tg_intersect_aabbs(v3 min0, v3 max0, v3 min1, v3 max1)
{
    const b32 bx0 = min0.x >= min1.x && min0.x <= max1.x;
    const b32 bx1 = max0.x >= min1.x && max0.x <= max1.x;
    const b32 by0 = min0.y >= min1.y && min0.y <= max1.y;
    const b32 by1 = max0.y >= min1.y && max0.y <= max1.y;
    const b32 bz0 = min0.z >= min1.z && min0.z <= max1.z;
    const b32 bz1 = max0.z >= min1.z && max0.z <= max1.z;
    const b32 bx = bx0 || bx1;
    const b32 by = by0 || by1;
    const b32 bz = bz0 || bz1;
    const b32 result = bx && by && bz;
    return result;
}

b32 tg_intersect_aabbsi(v3i min0, v3i max0, v3i min1, v3i max1)
{
    const b32 bx0 = min0.x >= min1.x && min0.x <= max1.x;
    const b32 bx1 = max0.x >= min1.x && max0.x <= max1.x;
    const b32 by0 = min0.y >= min1.y && min0.y <= max1.y;
    const b32 by1 = max0.y >= min1.y && max0.y <= max1.y;
    const b32 bz0 = min0.z >= min1.z && min0.z <= max1.z;
    const b32 bz1 = max0.z >= min1.z && max0.z <= max1.z;
    const b32 bx = bx0 || bx1;
    const b32 by = by0 || by1;
    const b32 bz = bz0 || bz1;
    const b32 result = bx && by && bz;
    return result;
}

b32 tg_intersect_aabb_obb(v3 min, v3 max, v3* p_obb_corners)
{
    TG_ASSERT(tgm_v3_less(min, max));
    TG_ASSERT(p_obb_corners != TG_NULL);

    // Order: x-, x+, y-, y+, z-, z+

    b32 separated;

    // AABB faces

    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].x < min.x;
        if (!separated)
        {
            goto x_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

x_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].x > max.x;
        if (!separated)
        {
            goto y_neg;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

y_neg:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].y < min.y;
        if (!separated)
        {
            goto y_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

y_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].y > max.y;
        if (!separated)
        {
            goto z_neg;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

z_neg:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].z < min.z;
        if (!separated)
        {
            goto z_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

z_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].z > max.z;
        if (!separated)
        {
            goto obb;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

    // OBB faces

obb:
    // TODO: remove this bs
    (void)(3 + 3);

    const v3 nxp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[1], p_obb_corners[0]));
    const v3 nyp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[2], p_obb_corners[0]));
    const v3 nzp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[4], p_obb_corners[0]));

    v3 p_obb_normals[6] = { 0 };
    p_obb_normals[0] = tgm_v3_neg(nxp);
    p_obb_normals[1] = nxp;
    p_obb_normals[2] = tgm_v3_neg(nyp);
    p_obb_normals[3] = nyp;
    p_obb_normals[4] = tgm_v3_neg(nzp);
    p_obb_normals[5] = nzp;

    // We have to find corners that lie on the respective plane. There are two corners, such that at least one of them lies on each plane.
    f32 p_obb_distances[6] = { 0 };
    const f32 l0 = tgm_v3_mag(p_obb_corners[0]);
    const f32 l1 = tgm_v3_mag(p_obb_corners[7]);
    const v3 n0 = l0 == 0.0f ? (v3) { 0 } : tgm_v3_divf(p_obb_corners[0], l0);
    const v3 n1 = l1 == 0.0f ? (v3) { 0 } : tgm_v3_divf(p_obb_corners[7], l1);
    p_obb_distances[0] = l0 * tgm_v3_dot(n0, p_obb_normals[0]);
    p_obb_distances[1] = l1 * tgm_v3_dot(n1, p_obb_normals[1]);
    p_obb_distances[2] = l0 * tgm_v3_dot(n0, p_obb_normals[2]);
    p_obb_distances[3] = l1 * tgm_v3_dot(n1, p_obb_normals[3]);
    p_obb_distances[4] = l0 * tgm_v3_dot(n0, p_obb_normals[4]);
    p_obb_distances[5] = l1 * tgm_v3_dot(n1, p_obb_normals[5]);

    v3 p_aabb_points[8] = { 0 };
    p_aabb_points[0] = (v3){ min.x, min.y, min.z };
    p_aabb_points[1] = (v3){ max.x, min.y, min.z };
    p_aabb_points[2] = (v3){ min.x, max.y, min.z };
    p_aabb_points[3] = (v3){ max.x, max.y, min.z };
    p_aabb_points[4] = (v3){ min.x, min.y, max.z };
    p_aabb_points[5] = (v3){ max.x, min.y, max.z };
    p_aabb_points[6] = (v3){ min.x, max.y, max.z };
    p_aabb_points[7] = (v3){ max.x, max.y, max.z };

    for (u32 plane_idx = 0; plane_idx < 6; plane_idx++)
    {
        const v3 normal = p_obb_normals[plane_idx];
        const f32 distance = p_obb_distances[plane_idx];

        separated = TG_TRUE;

        for (u32 point_idx = 0; point_idx < 8; point_idx++)
        {
            const v3 point = p_aabb_points[point_idx];
            const f32 dist = tgm_v3_dot(normal, point) - distance;
            separated &= dist > 0.0f;

            if (!separated)
            {
                break;
            }
        }

        if (separated)
        {
            return TG_FALSE;
        }
    }
    return TG_TRUE;
}

b32 tg_intersect_aabb_obb_ignore_contact(v3 min, v3 max, v3* p_obb_corners)
{
    TG_ASSERT(tgm_v3_less(min, max));
    TG_ASSERT(p_obb_corners != TG_NULL);

    // Order: x-, x+, y-, y+, z-, z+

    b32 separated;

    // AABB faces

    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].x <= min.x;
        if (!separated)
        {
            goto x_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

x_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].x >= max.x;
        if (!separated)
        {
            goto y_neg;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

y_neg:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].y <= min.y;
        if (!separated)
        {
            goto y_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

y_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].y >= max.y;
        if (!separated)
        {
            goto z_neg;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

z_neg:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].z <= min.z;
        if (!separated)
        {
            goto z_pos;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

z_pos:
    separated = TG_TRUE;
    for (u32 i = 0; i < 8; i++)
    {
        separated &= p_obb_corners[i].z >= max.z;
        if (!separated)
        {
            goto obb;
        }
    }
    if (separated)
    {
        return TG_FALSE;
    }

    // OBB faces

obb:
    // TODO: remove this bs
    (void)(3 + 3);

    const v3 nxp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[1], p_obb_corners[0]));
    const v3 nyp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[2], p_obb_corners[0]));
    const v3 nzp = tgm_v3_normalized(tgm_v3_sub(p_obb_corners[4], p_obb_corners[0]));

    v3 p_obb_normals[6] = { 0 };
    p_obb_normals[0] = tgm_v3_neg(nxp);
    p_obb_normals[1] = nxp;
    p_obb_normals[2] = tgm_v3_neg(nyp);
    p_obb_normals[3] = nyp;
    p_obb_normals[4] = tgm_v3_neg(nzp);
    p_obb_normals[5] = nzp;

    // We have to find corners that lie on the respective plane. There are two corners, such that at least one of them lies on each plane.
    f32 p_obb_distances[6] = { 0 };
    const f32 l0 = tgm_v3_mag(p_obb_corners[0]);
    const f32 l1 = tgm_v3_mag(p_obb_corners[7]);
    const v3 n0 = l0 == 0.0f ? (v3) { 0 } : tgm_v3_divf(p_obb_corners[0], l0);
    const v3 n1 = l1 == 0.0f ? (v3) { 0 } : tgm_v3_divf(p_obb_corners[7], l1);
    p_obb_distances[0] = l0 * tgm_v3_dot(n0, p_obb_normals[0]);
    p_obb_distances[1] = l1 * tgm_v3_dot(n1, p_obb_normals[1]);
    p_obb_distances[2] = l0 * tgm_v3_dot(n0, p_obb_normals[2]);
    p_obb_distances[3] = l1 * tgm_v3_dot(n1, p_obb_normals[3]);
    p_obb_distances[4] = l0 * tgm_v3_dot(n0, p_obb_normals[4]);
    p_obb_distances[5] = l1 * tgm_v3_dot(n1, p_obb_normals[5]);

    v3 p_aabb_points[8] = { 0 };
    p_aabb_points[0] = (v3){ min.x, min.y, min.z };
    p_aabb_points[1] = (v3){ max.x, min.y, min.z };
    p_aabb_points[2] = (v3){ min.x, max.y, min.z };
    p_aabb_points[3] = (v3){ max.x, max.y, min.z };
    p_aabb_points[4] = (v3){ min.x, min.y, max.z };
    p_aabb_points[5] = (v3){ max.x, min.y, max.z };
    p_aabb_points[6] = (v3){ min.x, max.y, max.z };
    p_aabb_points[7] = (v3){ max.x, max.y, max.z };

    for (u32 plane_idx = 0; plane_idx < 6; plane_idx++)
    {
        const v3 normal = p_obb_normals[plane_idx];
        const f32 distance = p_obb_distances[plane_idx];

        separated = TG_TRUE;

        for (u32 point_idx = 0; point_idx < 8; point_idx++)
        {
            const v3 point = p_aabb_points[point_idx];
            const f32 dist = tgm_v3_dot(normal, point) - distance;
            separated &= dist >= 0.0f;

            if (!separated)
            {
                break;
            }
        }

        if (separated)
        {
            return TG_FALSE;
        }
    }
    return TG_TRUE;
}

b32 tg_intersect_ray_aabb(v3 ray_origin, v3 ray_direction, v3 min, v3 max, TG_OUT f32* p_enter, TG_OUT f32* p_exit)
{
    const v3 v3_max = { TG_F32_MAX, TG_F32_MAX, TG_F32_MAX };
    const v3 vec0 = tgm_v3_div_zero_check(tgm_v3_sub(min, ray_origin), ray_direction, v3_max);
    const v3 vec1 = tgm_v3_div_zero_check(tgm_v3_sub(max, ray_origin), ray_direction, v3_max);
    const v3 n = tgm_v3_min(vec0, vec1);
    const v3 f = tgm_v3_max(vec0, vec1);
    *p_enter = TG_MAX(TG_MAX(n.x, n.y), n.z);
    *p_exit = TG_MIN(TG_MIN(f.x, f.y), f.z);
    const b32 result = *p_exit > 0.0f && *p_enter <= *p_exit;
    return result;
}

b32 tg_intersect_ray_plane(v3 ray_origin, v3 ray_direction, v3 plane_point, v3 plane_normal, TG_OUT tg_raycast_hit* p_hit)
{
    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON && tgm_v3_magsqr(plane_normal) > TG_F32_EPSILON);

    const f32 denom = tgm_v3_dot(plane_normal, ray_direction);
    b32 result = TG_FALSE;
    if (denom > TG_F32_EPSILON)
    {
        const v3 v = tgm_v3_sub(plane_point, ray_origin);
        float t = tgm_v3_dot(v, plane_normal) / denom;
        result = t >= 0;
        if (result && p_hit)
        {
            p_hit->distance = t;
            p_hit->hit = tgm_v3_add(ray_origin, tgm_v3_mulf(ray_direction, t));
            p_hit->normal = plane_normal;
        }
    }
    return result;
}

b32 tg_intersect_ray_triangle(v3 ray_origin, v3 ray_direction, v3 tri_p0, v3 tri_p1, v3 tri_p2, TG_OUT tg_raycast_hit* p_hit)
{
    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON);

    v3 barycentric_coords = { TG_F32_MAX,TG_F32_MAX,TG_F32_MAX };

    v3 v01 = tgm_v3_sub(tri_p1, tri_p0);
    v3 v02 = tgm_v3_sub(tri_p2, tri_p0);
    v3 h = tgm_v3_cross(ray_direction, v02);
    f32 a = tgm_v3_dot(v01, h);

    if (a >= TG_F32_EPSILON)
    {
        f32 f = 1.0f / a;

        v3 s = tgm_v3_sub(ray_origin, tri_p0);
        barycentric_coords.y = tgm_v3_dot(s, h) * f;

        if (barycentric_coords.y >= 0.0f && barycentric_coords.y <= 1.0f)
        {
            v3 q = tgm_v3_cross(s, v01);
            barycentric_coords.z = tgm_v3_dot(ray_direction, q) * f;

            if (barycentric_coords.z >= 0.0f && barycentric_coords.y + barycentric_coords.z <= 1.0f)
            {
                barycentric_coords.x = tgm_v3_dot(v02, q) * f;
                if (barycentric_coords.x < TG_F32_EPSILON)
                {
                    barycentric_coords.x = TG_F32_MAX;
                }
            }
        }
    }

    b32 result = TG_FALSE;
    if (barycentric_coords.y >= 0.0f && barycentric_coords.z >= 0.0f && barycentric_coords.y + barycentric_coords.z <= 1.0f)
    {
        const v3 v0 = tgm_v3_mulf(tri_p0, 1.0f - barycentric_coords.y - barycentric_coords.z);
        const v3 v1 = tgm_v3_mulf(tri_p1, barycentric_coords.y);
        const v3 v2 = tgm_v3_mulf(tri_p2, barycentric_coords.z);
        const v3 hit = tgm_v3_add(tgm_v3_add(v0, v1), v2);
        if (tgm_v3_dot(tgm_v3_sub(hit, ray_origin), ray_direction) > 0.0f)
        {
            result = TG_TRUE;
            if (p_hit)
            {
                p_hit->distance = tgm_v3_mag(tgm_v3_sub(hit, ray_origin));
                p_hit->hit = hit;
                p_hit->normal = tgm_v3_normalized(tgm_v3_cross(tgm_v3_sub(tri_p1, tri_p0), tgm_v3_sub(tri_p2, tri_p0)));
            }
        }
    }
    return result;
}
