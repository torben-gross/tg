#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"



layout(location = 0) in v3 v_position;



layout(set = 0, binding = 0) uniform unique_ubo
{
    m4     u_translation;
    m4     u_rotation;
    m4     u_scale;
    u32    u_first_voxel_id;
};

layout(set = 0, binding = 2) uniform raytracing_ubo
{
    v3    u_camera;
    v3    u_ray00;
    v3    u_ray10;
    v3    u_ray01;
    v3    u_ray11;
};

layout(set = 0, binding = 3) buffer visibility_buffer
{
    u32    u_w;
    u32    u_h;
    u64    u_vb[];
};

layout(set = 0, binding = 4) buffer data
{
    u32    u_data_w;
    u32    u_data_h;
    u32    u_data_d;
    u32    pad;
    u32    u_data[];
};



struct tg_ray
{
    v3    o;
    v3    d;
    v3    invd;
};

struct tg_box
{
    v3    min;
    v3    max;
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
bool tg_intersect_ray_box(tg_ray r, tg_box b, out f32 t)
{
    v3 vector1 = (b.min - r.o) * r.invd;
    v3 vector2 = (b.max - r.o) * r.invd;
    v3 n = tg_min_component_wise(vector1, vector2);
    v3 f = tg_max_component_wise(vector1, vector2);
    f32 enter = max(n.x, max(n.y, n.z));
    f32 exit = min(f.x, min(f.y, f.z));
    
    t = enter;
    return exit > 0.0 && enter < exit;
}



void main()
{
    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx = gl_FragCoord.x / f32(u_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(u_h);



    m4 ray_origin_mat = inverse(u_translation * u_rotation);
    v3 ray_origin = (ray_origin_mat * vec4(u_camera, 1.0)).xyz;

    m4 ray_direction_mat = inverse(u_rotation);
    v3 ray_direction_v3 = normalize(mix(mix(u_ray00, u_ray10, fy), mix(u_ray01, u_ray11, fy), fx));
    v3 ray_direction = (ray_direction_mat * v4(ray_direction_v3, 0.0)).xyz;
    
    v3 ray_inv_direction = vec3(1.0) / ray_direction;
    
    tg_ray r = tg_ray(ray_origin, ray_direction, ray_inv_direction);



    v3 box_max = v3(f32(u_data_w) / 2.0, f32(u_data_h) / 2.0, f32(u_data_d) / 2.0);
    v3 box_min = -box_max;
    tg_box b = tg_box(box_min, box_max);

    f32 d = 1.0;
    u32 id = 1;
    f32 t_box;
    if (tg_intersect_ray_box(r, b, t_box))
    {
        // supercover
        // https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
        
        v3 hit = t_box > 0.0 ? r.o + t_box * r.d : r.o;
        v3 xyz = clamp(floor(hit), box_min, box_max - v3(1.0));

        hit += v3(box_max);
        xyz += v3(box_max);

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
            u32 voxel_idx = u_data_w * u_data_h * z + u_data_w * y + x;
            u32 bits_idx = voxel_idx / 32;
            u32 bit_idx = voxel_idx % 32;
            u32 bits = u32(u_data[bits_idx]);
            u32 bit = bits & (1 << bit_idx);
            if (bit != 0)
            {
                d = abs(t_box) / 1000.0; // TODO: near and far plane! discard if too close!
                id = u_data_w * u_data_h * z + u_data_w * y + x;
                break;
            }
            if (t_max_x < t_max_y)
            {
                if (t_max_x < t_max_z)
                {
                    t_max_x += t_delta_x;
                    x += step_x;
                    if (x < 0 || x >= u_data_w) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= u_data_d) break;
                }
            }
            else
            {
                if (t_max_y < t_max_z)
                {
                    t_max_y += t_delta_y;
                    y += step_y;
                    if (y < 0 || y >= u_data_h) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= u_data_d) break;
                }
            }
        }
    }
    
    //if (d != 1.0)
    {
        u32 x = u32(gl_FragCoord.x);
        u32 y = u32(gl_FragCoord.y);
        u64 data = u64(id) | (u64(d * 4294967295.0) << u64(32));
        u32 i = u_w * y + x;
        atomicMin(u_vb[i], data);
    }
}
