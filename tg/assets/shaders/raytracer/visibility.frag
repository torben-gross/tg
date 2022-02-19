#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"



struct tg_instance_data
{
    m4     s_mat;
    m4     r_mat;
    m4     t_mat;
    u32    grid_w;
    u32    grid_h;
    u32    grid_d;
    u32    first_voxel_id;
};

struct tg_box
{
    v3    min;
    v3    max;
};

struct tg_ray
{
    v3    o;
    v3    d;
    v3    invd;
};



layout(location = 0) flat in uint    v_instance_id;
layout(location = 1)      in v3      v_position;

layout(std430, set = 0, binding = 0) buffer tg_instance_data_ssbo
{
    tg_instance_data    instance_data[];
};

layout(set = 0, binding = 2) uniform tg_raytracer_data_ubo
{
    v4     camera;
    v4     ray00;
    v4     ray10;
    v4     ray01;
    v4     ray11;
    f32    near;
    f32    far;
};

layout(set = 0, binding = 3) buffer tg_voxel_data_ssbo
{
    u32    voxel_data[];
};

layout(set = 0, binding = 4) buffer tg_visibility_buffer_ssbo
{
    u32    visibility_buffer_w;
    u32    visibility_buffer_h;
    u64    visibility_buffer_data[];
};



v3 tg_min_component_wise(v3 a, v3 b)
{
    return v3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

v3 tg_max_component_wise(v3 a, v3 b)
{
    return v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

// Source: https://gamedev.net/forums/topic/682750-problem-with-raybox-intersection-in-glsl/5313495/
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool tg_intersect_ray_box(tg_ray r, tg_box b, out f32 t)
{
    v3 vector1 = (b.min - r.o) * r.invd;
    v3 vector2 = (b.max - r.o) * r.invd;
    v3 n = tg_min_component_wise(vector1, vector2);
    v3 f = tg_max_component_wise(vector1, vector2);
    f32 enter = max(max(n.x, n.y), n.z);
    f32 exit = min(min(f.x, f.y), f.z);
    
    t = enter;
    return exit > 0.0 && enter < exit;
}



void main()
{
    tg_instance_data i = instance_data[v_instance_id];
    
    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);



    // Instead of transforming the box, the ray is transformed with its inverse
    m4 ray_origin_mat = inverse(i.t_mat * i.r_mat);
    v3 ray_origin = (ray_origin_mat * vec4(camera.xyz, 1.0)).xyz;

    m4 ray_direction_mat = inverse(i.r_mat);
    v3 ray_direction_v3 = mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx);
    v3 ray_direction = normalize((ray_direction_mat * v4(ray_direction_v3, 0.0)).xyz);
    
    v3 ray_inv_direction = v3(1.0) / ray_direction;
    
    tg_ray r = tg_ray(ray_origin, ray_direction, ray_inv_direction);



    v3 box_max = v3(f32(i.grid_w) / 2.0, f32(i.grid_h) / 2.0, f32(i.grid_d) / 2.0);
    v3 box_min = -box_max;
    tg_box b = tg_box(box_min, box_max);

    f32 d = 1.0;
    u32 voxel_id = 0;
    f32 t_box;
    if (tg_intersect_ray_box(r, b, t_box))
    {
        // supercover
        // https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
        
        v3 hit = t_box > 0.0 ? r.o + t_box * r.d : r.o;
        v3 xyz = clamp(floor(hit), box_min, box_max - v3(1.0));

        hit += box_max;
        xyz += box_max;

        i32 x = i32(xyz.x);
        i32 y = i32(xyz.y);
        i32 z = i32(xyz.z);

        i32 step_x;
        i32 step_y;
        i32 step_z;

        f32 t_max_x = TG_F32_MAX;
        f32 t_max_y = TG_F32_MAX;
        f32 t_max_z = TG_F32_MAX;

        f32 t_delta_x = TG_F32_MAX;
        f32 t_delta_y = TG_F32_MAX;
        f32 t_delta_z = TG_F32_MAX;

        if (r.d.x > 0.0)
        {
            step_x = 1;
            t_max_x = t_box + (f32(x + 1) - hit.x) / r.d.x;
            t_delta_x = 1.0 / r.d.x;
        }
        else
        {
            step_x = -1;
            t_max_x = t_box + (hit.x - f32(x)) / -r.d.x;
            t_delta_x = 1.0 / -r.d.x;
        }
        if (r.d.y > 0.0)
        {
            step_y = 1;
            t_max_y = t_box + (f32(y + 1) - hit.y) / r.d.y;
            t_delta_y = 1.0 / r.d.y;
        }
        else
        {
            step_y = -1;
            t_max_y = t_box + (hit.y - f32(y)) / -r.d.y;
            t_delta_y = 1.0 / -r.d.y;
        }
        if (r.d.z > 0.0)
        {
            step_z = 1;
            t_max_z = t_box + (f32(z + 1) - hit.z) / r.d.z;
            t_delta_z = 1.0 / r.d.z;
        }
        else
        {
            step_z = -1;
            t_max_z = t_box + (hit.z - f32(z)) / -r.d.z;
            t_delta_z = 1.0 / -r.d.z;
        }

        while (true)
        {
            u32 vox_id    = i.grid_w * i.grid_h * z + i.grid_w * y + x;
            u32 vox_idx   = i.first_voxel_id + vox_id;
            u32 bits_idx  = vox_idx / 32;
            u32 bit_idx   = vox_idx % 32;
            u32 bits      = voxel_data[bits_idx];
            u32 bit       = bits & (1 << bit_idx);
            if (bit != 0)
            {
                v3 voxel_min = box_min + v3(f32(x    ), f32(y    ), f32(z    ));
                v3 voxel_max = box_min + v3(f32(x + 1), f32(y + 1), f32(z + 1));
                tg_box voxel = tg_box(voxel_min, voxel_max);
                f32 t_voxel;
                tg_intersect_ray_box(r, voxel, t_voxel); // TODO: We should in this case adjust the function to potentially return enter AND exit, because if we are inside of the voxel, we receive a negative 't_voxel', which results in wrong depth. Might be irrelevant...
                d = min(1.0, t_voxel / far);
                voxel_id = vox_id;
                break;
            }
            if (t_max_x < t_max_y)
            {
                if (t_max_x < t_max_z)
                {
                    t_max_x += t_delta_x;
                    x += step_x;
                    if (x < 0 || x >= i.grid_w) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= i.grid_d) break;
                }
            }
            else
            {
                if (t_max_y < t_max_z)
                {
                    t_max_y += t_delta_y;
                    y += step_y;
                    if (y < 0 || y >= i.grid_h) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= i.grid_d) break;
                }
            }
        }
    }
    
    if (d != 1.0)
    {
        // Layout : 24 bits depth | 10 bits instance id | 30 bits relative voxel_id

        u64 depth_24b = u64(d * 16777215.0) << u64(40);
        u64 instance_id_10b = u64(v_instance_id) << u64(30);
        u64 voxel_id_30b = u64(voxel_id);
        u64 packed_data = depth_24b | instance_id_10b | voxel_id_30b;

        u32 x = u32(gl_FragCoord.x);
        u32 y = u32(gl_FragCoord.y);
        u32 pixel_idx = visibility_buffer_w * y + x;
        atomicMin(visibility_buffer_data[pixel_idx], packed_data);
    }
}
