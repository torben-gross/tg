#include "physics/tg_physics.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"

// source: https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c
b32 tg_intersect_ray_box(v3 ray_origin, v3 ray_direction, v3 min, v3 max, tg_raycast_hit* p_hit)
{
    #define TG_RIGHT	 0
    #define TG_LEFT	     1
    #define TG_MIDDLE	 2

    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON);

    b32 result = TG_TRUE;
    u8 quadrant[3];
    f32 candidate_plane[3];

    for (u8 i = 0; i < 3; i++)
    {
        if (ray_origin.p_data[i] < min.p_data[i])
        {
            quadrant[i] = TG_LEFT;
            candidate_plane[i] = min.p_data[i];
            result = TG_FALSE;
        }
        else if (ray_origin.p_data[i] > max.p_data[i])
        {
            quadrant[i] = TG_RIGHT;
            candidate_plane[i] = max.p_data[i];
            result = TG_FALSE;
        }
        else
        {
            quadrant[i] = TG_MIDDLE;
        }
    }

    if (result)
    {
        if (p_hit)
        {
            p_hit->distance = 0.0f;
            p_hit->hit = ray_origin;
            const v3 center = tgm_v3_divf(tgm_v3_add(min, max), 2.0f);
            const v3 delta = tgm_v3_sub(ray_origin, center);
            v3 normal = { 0 };
            if (tgm_f32_abs(delta.x) >= tgm_f32_abs(delta.y) && tgm_f32_abs(delta.x) >= tgm_f32_abs(delta.z))
            {
                normal.x = 1.0f;
            }
            else if (tgm_f32_abs(delta.y) >= tgm_f32_abs(delta.x) && tgm_f32_abs(delta.y) >= tgm_f32_abs(delta.z))
            {
                normal.y = 1.0f;
            }
            else
            {
                normal.z = 1.0f;
            }
            p_hit->normal = tgm_v3_normalized(normal);
        }
        return TG_TRUE;
    }

    f32 max_t[3];
    for (u8 i = 0; i < 3; i++)
    {
        if (quadrant[i] != TG_MIDDLE && ray_direction.p_data[i] != 0.0f)
        {
            max_t[i] = (candidate_plane[i] - ray_origin.p_data[i]) / ray_direction.p_data[i];
        }
        else
        {
            max_t[i] = -1.0f;
        }
    }

    u8 which_plane = 0;
    for (u8 i = 1; i < 3; i++)
    {
        if (max_t[which_plane] < max_t[i])
        {
            which_plane = i;
        }
    }

    if (max_t[which_plane] < 0.0f)
    {
        return TG_FALSE;
    }

    v3 hit = { 0 };
    for (u8 i = 0; i < 3; i++)
    {
        if (which_plane != i)
        {
            const f32 f = ray_origin.p_data[i] + max_t[which_plane] * ray_direction.p_data[i];
            if (f < min.p_data[i] || f > max.p_data[i])
            {
                return TG_FALSE;
            }
            else
            {
                hit.p_data[i] = f;
            }
        }
        else
        {
            hit.p_data[i] = candidate_plane[i];
        }
    }

    if (p_hit)
    {
        p_hit->distance = tgm_v3_mag(tgm_v3_sub(hit, ray_origin));
        p_hit->hit = hit;
        const v3 center = tgm_v3_divf(tgm_v3_add(min, max), 2.0f);
        const v3 delta = tgm_v3_sub(hit, center);
        v3 normal = { 0 };
        if (tgm_f32_abs(delta.x) >= tgm_f32_abs(delta.y) && tgm_f32_abs(delta.x) >= tgm_f32_abs(delta.z))
        {
            normal.x = 1.0f;
        }
        else if (tgm_f32_abs(delta.y) >= tgm_f32_abs(delta.x) && tgm_f32_abs(delta.y) >= tgm_f32_abs(delta.z))
        {
            normal.y = 1.0f;
        }
        else
        {
            normal.z = 1.0f;
        }
        p_hit->normal = tgm_v3_normalized(normal);
    }

    return TG_TRUE;

    #undef TG_MIDDLE
    #undef TG_LEFT
    #undef TG_RIGHT
}

b32 tg_intersect_ray_plane(v3 ray_origin, v3 ray_direction, v3 plane_point, v3 plane_normal, tg_raycast_hit* p_hit)
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

b32 tg_intersect_ray_triangle(v3 ray_origin, v3 ray_direction, v3 tri_p0, v3 tri_p1, v3 tri_p2, tg_raycast_hit* p_hit)
{
    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON);

    v3 barycentric_coords = V3(TG_F32_MAX);

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
