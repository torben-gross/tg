#version 450

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_atomic_int64                    : require

#include "shaders/common.inc"



layout(location = 0) in v3 v_position;



layout(set = 0, binding = 0) uniform model
{
    m4     u_model;
    u32    u_first_voxel_id;
};

layout(set = 0, binding = 2) uniform ubo
{
    v3    u_camera;
    v3    u_ray00;
    v3    u_ray10;
    v3    u_ray01;
    v3    u_ray11;
};

layout(set = 0, binding = 3) buffer visibility_buffer
{
    u32         u_w;
    u32         u_h;
    uint64_t    u_vb[];
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

struct tg_boxhit
{
    f32    tmin;
    f32    tmax;
};



bool tg_intersect_ray_box(tg_ray r, tg_box b, out tg_boxhit hit)
{
    v3 tbot = r.invd * (b.min - r.o);
    v3 ttop = r.invd * (b.max - r.o);
    v3 tmin = min(ttop, tbot);
    v3 tmax = max(ttop, tbot);
    v2 t = max(tmin.xx, tmin.yz);
    f32 t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    f32 t1 = min(t.x, t.y);
    hit.tmin = t0;
    hit.tmax = t1;
    return t1 > max(t0, 0.0);
}



void main()
{
    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx = gl_FragCoord.x / f32(u_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(u_h);
    v3 ray_direction = normalize(mix(mix(u_ray00, u_ray10, fy), mix(u_ray01, u_ray11, fy), fx));
    tg_ray r = tg_ray(u_camera, ray_direction, v3(1.0) / ray_direction);

    v3 bmin = -v3(f32(u_data_w) / 2.0, f32(u_data_h) / 2.0, f32(u_data_d) / 2.0);
    v3 bmax = v3(f32(u_data_w) / 2.0, f32(u_data_h) / 2.0, f32(u_data_d) / 2.0);
    bmin *= 0.98;
    bmax *= 0.98;
    tg_box b = tg_box(bmin, bmax);

    f32 d = 1.0;
    u32 id = 1;
    tg_boxhit bh;
    if (tg_intersect_ray_box(r, b, bh))
    {
        // supercover
        // https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
            
        v3 hit = bh.tmin > 0.0 ? r.o + bh.tmin * r.d : r.o;

        i32 x = min(i32(u_data_w), i32(ceil(hit.x)));
        i32 y = min(i32(u_data_h), i32(ceil(hit.y)));
        i32 z = min(i32(u_data_d), i32(ceil(hit.z)));

        i32 step_x = 0;
        i32 step_y = 0;
        i32 step_z = 0;

        f32 t_max_x = TG_F32_MAX;
        f32 t_max_y = TG_F32_MAX;
        f32 t_max_z = TG_F32_MAX;

        f32 t_delta_x = TG_F32_MAX;
        f32 t_delta_y = TG_F32_MAX;
        f32 t_delta_z = TG_F32_MAX;

        if (r.d.x > 0.0)
        {
            step_x = 1;
            t_delta_x = 1.0 / r.d.x;
            t_max_x = bh.tmin + (x - r.o.x) / r.d.x;
        }
        else if (r.d.x < 0.0)
        {
            step_x = -1;
            t_delta_x = 1.0 / -r.d.x;
            t_max_x = bh.tmin + ((x - 1) - r.o.x) / r.d.x;
        }
        if (r.d.y > 0.0)
        {
            step_y = 1;
            t_delta_y = 1.0 / r.d.y;
            t_max_y = bh.tmin + (y - r.o.y) / r.d.y;
        }
        else if (r.d.y < 0.0)
        {
            step_y = -1;
            t_delta_y = 1.0 / -r.d.y;
            t_max_y = bh.tmin + ((y - 1) - r.o.y) / r.d.y;
        }
        if (r.d.z > 0.0)
        {
            step_z = 1;
            t_delta_z = 1.0 / r.d.z;
            t_max_z = bh.tmin + (z - r.o.z) / r.d.z;
        }
        else if (r.d.z < 0.0)
        {
            step_z = -1;
            t_delta_z = 1.0 / -r.d.z;
            t_max_z = bh.tmin + ((z - 1) - r.o.z) / r.d.z;
        }

        while (x > -1 && x < u_data_w + 1 && y > -1 && y < u_data_h + 1 && z > -1 && z < u_data_d + 1)
        {
            if (t_max_x < t_max_y)
            {
                if (t_max_x < t_max_z)
                {
                    x += step_x;
                    t_max_x += t_delta_x;
                }
                else
                {
                    z += step_z;
                    t_max_z += t_delta_z;
                }
            }
            else
            {
                if (t_max_y < t_max_z)
                {
                    y += step_y;
                    t_max_y += t_delta_y;
                }
                else
                {
                    z += step_z;
                    t_max_z += t_delta_z;
                }
            }
            u32 idx = u_data_w/32 * u_data_h * z + u_data_w/32 * y + x/32;
            u32 bits = u32(u_data[idx]);
            u32 bit = bits & (1 << (x % 8));
            if (bit != 0)
            {
                d = abs(bh.tmin) / 1000.0;
                id = u_data_w * u_data_h * z + u_data_w * y + x;
                break;
            }
        }
    }
    
    
    
    u32 x = u32(gl_FragCoord.x);
    u32 y = u32(gl_FragCoord.y);
    //f32 d = gl_FragCoord.z;
    uint64_t data = uint64_t(id) | (uint64_t(d * 4294967295.0) << uint64_t(32));
    u32 i = u_w * y + x;
    atomicMin(u_vb[i], data);
}
