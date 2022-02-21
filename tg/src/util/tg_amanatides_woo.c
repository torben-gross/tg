#include "util/tg_amanatides_woo.h"

b32 tg_amanatides_woo(v3 ray_hit_on_grid, v3 ray_direction, v3 extent, const u32* p_voxel_grid, TG_OUT v3i* p_grid_voxel_id)
{
    TG_ASSERT(ray_hit_on_grid.x >= 0.0f && ray_hit_on_grid.x <= extent.x);
    TG_ASSERT(ray_hit_on_grid.y >= 0.0f && ray_hit_on_grid.y <= extent.y);
    TG_ASSERT(ray_hit_on_grid.z >= 0.0f && ray_hit_on_grid.z <= extent.z);

    b32 result = TG_FALSE;
    *p_grid_voxel_id = (v3i){ -1, -1, -1 };

    const v3 extent_minus_one = tgm_v3_sub(extent, (v3) { 1.0f, 1.0f, 1.0f });
    const v3 xyz = tgm_v3_min(tgm_v3_floor(ray_hit_on_grid), extent_minus_one);

    i32 x = (i32)xyz.x;
    i32 y = (i32)xyz.y;
    i32 z = (i32)xyz.z;

    i32 step_x = 0;
    i32 step_y = 0;
    i32 step_z = 0;

    f32 t_max_x = TG_F32_MAX;
    f32 t_max_y = TG_F32_MAX;
    f32 t_max_z = TG_F32_MAX;

    f32 t_delta_x = TG_F32_MAX;
    f32 t_delta_y = TG_F32_MAX;
    f32 t_delta_z = TG_F32_MAX;

    if (ray_direction.x > 0.0f)
    {
        step_x = 1;
        t_max_x = ((f32)(x + 1) - ray_hit_on_grid.x) / ray_direction.x;
        t_delta_x = 1.0f / ray_direction.x;
    }
    else if (ray_direction.x < 0.0f)
    {
        step_x = -1;
        t_max_x = (ray_hit_on_grid.x - (f32)x) / -ray_direction.x;
        t_delta_x = 1.0f / -ray_direction.x;
    }
    if (ray_direction.y > 0.0f)
    {
        step_y = 1;
        t_max_y = ((f32)(y + 1) - ray_hit_on_grid.y) / ray_direction.y;
        t_delta_y = 1.0f / ray_direction.y;
    }
    else if (ray_direction.y < 0.0f)
    {
        step_y = -1;
        t_max_y = (ray_hit_on_grid.y - (f32)y) / -ray_direction.y;
        t_delta_y = 1.0f / -ray_direction.y;
    }
    if (ray_direction.z > 0.0f)
    {
        step_z = 1;
        t_max_z = ((f32)(z + 1) - ray_hit_on_grid.z) / ray_direction.z;
        t_delta_z = 1.0f / ray_direction.z;
    }
    else if (ray_direction.z < 0.0f)
    {
        step_z = -1;
        t_max_z = (ray_hit_on_grid.z - (f32)z) / -ray_direction.z;
        t_delta_z = 1.0f / -ray_direction.z;
    }

    while (TG_TRUE)
    {
        const u32 vox_idx = (u32)extent.x * (u32)extent.y * z + (u32)extent.x * y + x;
        const u32 bits_idx = vox_idx / 32;
        const u32 bit_idx = vox_idx % 32;
        const u32 bits = p_voxel_grid[bits_idx];
        const u32 bit = bits & (1 << bit_idx);
        if (bit != 0)
        {
            *p_grid_voxel_id = (v3i){ x, y, z };
            result = TG_TRUE;
            break;
        }
        if (t_max_x < t_max_y)
        {
            if (t_max_x < t_max_z)
            {
                t_max_x += t_delta_x;
                x += step_x;
                if (x < 0 || x >= extent.x) break;
            }
            else
            {
                t_max_z += t_delta_z;
                z += step_z;
                if (z < 0 || z >= extent.z) break;
            }
        }
        else
        {
            if (t_max_y < t_max_z)
            {
                t_max_y += t_delta_y;
                y += step_y;
                if (y < 0 || y >= extent.y) break;
            }
            else
            {
                t_max_z += t_delta_z;
                z += step_z;
                if (z < 0 || z >= extent.z) break;
            }
        }
    }

    return result;
}
