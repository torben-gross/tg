#include "physics/tg_physics.h"

#include "graphics/tg_graphics.h"



b32 tg__traverse(v3 ray_origin, v3 ray_direction, const tg_kd_tree* p_kd_tree, const tg_kd_node* p_node, tg_bounds bounds, v3* p_hit)
{
    if (p_node->flags == 0)
    {
        f32 dsqr = TG_F32_MAX;
        for (u32 i = p_node->leaf.first_index_offset; i < p_node->leaf.first_index_offset + p_node->leaf.index_count; i += 3)
        {
            u32 i0 = p_kd_tree->p_indices[i];
            u32 i1 = p_kd_tree->p_indices[i + 1];
            u32 i2 = p_kd_tree->p_indices[i + 2];
            const v3 p0 = TG_MESH_POSITIONS(*p_kd_tree->p_mesh)[i0];
            const v3 p1 = TG_MESH_POSITIONS(*p_kd_tree->p_mesh)[i1];
            const v3 p2 = TG_MESH_POSITIONS(*p_kd_tree->p_mesh)[i2];
            v3 hit = { 0 };
            const b32 result = tg_intersect_ray_triangle(ray_origin, ray_direction, p0, p1, p2, &hit);
            const f32 ds = tgm_v3_magsqr(tgm_v3_sub(hit, ray_origin));
            if (result && ds < dsqr)
            {
                tg_intersect_ray_triangle(ray_origin, ray_direction, p0, p1, p2, &hit);
                dsqr = ds;
                if (p_hit)
                {
                    *p_hit = hit;
                }
            }
        }
        return dsqr < TG_F32_MAX;
    }

    if (!tg_intersect_ray_box(ray_origin, ray_direction, bounds.min, bounds.max, TG_NULL))
    {
        return TG_FALSE;
    }

    u32 split_axis = 0;
    u32 i = p_node->flags;
    while (i >>= 1) split_axis++;
    
    const tg_kd_node* p_near = &p_kd_tree->p_nodes[p_node->node.p_child_indices[0]];
    const tg_kd_node* p_far = &p_kd_tree->p_nodes[p_node->node.p_child_indices[1]];

    tg_bounds bounds_near = bounds;
    tg_bounds bounds_far = bounds;
    bounds_near.max.p_data[split_axis] = p_node->node.split_position;
    bounds_far.min.p_data[split_axis] = p_node->node.split_position;

    if (ray_origin.p_data[split_axis] > p_node->node.split_position)
    {
        const tg_kd_node* p_temp = p_near;
        p_near = p_far;
        p_far = p_temp;

        const tg_bounds temp = bounds_near;
        bounds_near = bounds_far;
        bounds_far = temp;
    }

    v3 hit = { 0 };
    b32 result = tg__traverse(ray_origin, ray_direction, p_kd_tree, p_near, bounds_near, &hit);
    if (result && p_hit)
    {
        *p_hit = hit;
    }
    if (!result || tgm_f32_abs(p_node->node.split_position - ray_origin.p_data[split_axis]) < tgm_f32_abs(hit.p_data[split_axis] - ray_origin.p_data[split_axis]))
    {
        if (tg__traverse(ray_origin, ray_direction, p_kd_tree, p_far, bounds_far, &hit))
        {
            result |= TG_TRUE;
            if (p_hit)
            {
                *p_hit = hit;
            }
        }
    }
    return result;
}



// source: https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c

#define TG_RIGHT	 0
#define TG_LEFT	     1
#define TG_MIDDLE	 2

b32 tg_intersect_ray_box(v3 ray_origin, v3 ray_direction, v3 min, v3 max, v3* p_hit)
{
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
            *p_hit = ray_origin;
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

    for (u8 i = 0; i < 3; i++)
    {
        if (which_plane != i)
        {
            const f32 f = ray_origin.p_data[i] + max_t[which_plane] * ray_direction.p_data[i];
            if (f < min.p_data[i] || f > max.p_data[i])
            {
                return TG_FALSE;
            }
            else if (p_hit)
            {
                p_hit->p_data[i] = f;
            }
        }
        else if (p_hit)
        {
            p_hit->p_data[i] = candidate_plane[i];
        }
    }
    return TG_TRUE;
}

#undef TG_MIDDLE
#undef TG_LEFT
#undef TG_RIGHT

b32 tg_intersect_ray_plane(v3 ray_origin, v3 ray_direction, v3 plane_point, v3 plane_normal, f32* p_distance)
{
    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON && tgm_v3_magsqr(plane_normal) > TG_F32_EPSILON);

    const f32 denom = tgm_v3_dot(plane_normal, ray_direction);
    b32 result = TG_FALSE;
    if (denom > TG_F32_EPSILON)
    {
        const v3 v = tgm_v3_sub(plane_point, ray_origin);
        float t = tgm_v3_dot(v, plane_normal) / denom;
        result = t >= 0;
        if (result && p_distance)
        {
            *p_distance = t;
        }
    }
    return result;
}

b32 tg_intersect_ray_triangle(v3 ray_origin, v3 ray_direction, v3 tri_p0, v3 tri_p1, v3 tri_p2, v3* p_hit)
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
    if (barycentric_coords.y >= 0.0f && barycentric_coords.z >= 0.0f && barycentric_coords.y + barycentric_coords.z <= 1.0f && p_hit)
    {
        const v3 v0 = tgm_v3_mulf(tri_p0, 1.0f - barycentric_coords.y - barycentric_coords.z);
        const v3 v1 = tgm_v3_mulf(tri_p1, barycentric_coords.y);
        const v3 v2 = tgm_v3_mulf(tri_p2, barycentric_coords.z);
        const v3 hit = tgm_v3_add(tgm_v3_add(v0, v1), v2);
        if (tgm_v3_dot(tgm_v3_sub(hit, ray_origin), ray_direction) > 0.0f)
        {
            result = TG_TRUE;
            *p_hit = hit;
        }
    }
    return result;
}

b32 tg_raycast_kd_tree(v3 ray_origin, v3 ray_direction, const tg_kd_tree* p_kd_tree, v3* p_hit)
{
    TG_ASSERT(tgm_v3_magsqr(ray_direction) > TG_F32_EPSILON && p_kd_tree && p_kd_tree->node_count != 0);

    const b32 result = tg__traverse(ray_origin, ray_direction, p_kd_tree, p_kd_tree->p_nodes, p_kd_tree->p_mesh->bounds, p_hit);
    return result;
}
