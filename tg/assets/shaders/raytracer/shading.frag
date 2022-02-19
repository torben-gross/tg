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



layout(location = 0) in v2 v_uv;

layout(set = 0, binding = 4) buffer tg_visibility_buffer_ssbo
{
    u32    visibility_buffer_w;
    u32    visibility_buffer_h;
    u64    visibility_buffer_data[];
};

layout(set = 0, binding = 5) uniform tg_shading_data_ubo
{
    v4     camera;
    v4     ray00;
    v4     ray10;
    v4     ray01;
    v4     ray11;
    v4     sun_direction;
    f32    near;
    f32    far;
    //u32                              u_directional_light_count;
    //u32                              u_point_light_count;
    //m4                               u_ivp;
    //v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_directions;
    //v4[TG_MAX_DIRECTIONAL_LIGHTS]    u_directional_light_colors;
    //v4[TG_MAX_POINT_LIGHTS]          u_point_light_positions;
    //v4[TG_MAX_POINT_LIGHTS]          u_point_light_colors;
};

layout(std430, set = 0, binding = 6) buffer tg_instance_data_ssbo
{
    tg_instance_data    instance_data[];
};

layout(std430, set = 0, binding = 7) buffer tg_color_lut_id_ssbo
{
    u32    color_lut_ids[];
};

layout(std430, set = 0, binding = 8) buffer tg_color_lut_ssbo
{
    u32    color_lut[];
};

layout(location = 0) out v4 out_color;



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

u32 tg_hash_u32(u32 v) // MurmurHash3 algorithm by Austin Appleby
{
	u32 result = v;
	result ^= result >> 16;
	result *= 0x85ebca6b;
	result ^= result >> 13;
	result *= 0xc2b2ae35;
	result ^= result >> 16;
	return result;
}

void main()
{
    u32 i = visibility_buffer_w * u32(gl_FragCoord.y) + u32(gl_FragCoord.x);
    u64 packed_data = visibility_buffer_data[i];
    f32 depth_24b = f32(packed_data >> u64(40)) / 16777215.0;
    u32 instance_id_10b = u32(packed_data >> u64(30)) & 1023;
    u32 voxel_id_30b = u32(packed_data & 1073741823);

    if (depth_24b < 1.0)
    {
        tg_instance_data i = instance_data[instance_id_10b];
        
        u32 color_lut_id_slot_idx = (i.first_voxel_id + voxel_id_30b) / 4;
        u32 color_lut_id_byte_idx = voxel_id_30b % 4;
        u32 color_lut_id_shift = color_lut_id_byte_idx * 8;

        u32 color_lut_id_slot = color_lut_ids[color_lut_id_slot_idx];
        u32 color_lut_id = (color_lut_id_slot >> color_lut_id_shift) & 255;

        u32 color_u32 = color_lut[color_lut_id];
        u32 r_u32 =  color_u32 >> 24;
        u32 g_u32 = (color_u32 >> 16) & 0xff;
        u32 b_u32 = (color_u32 >>  8) & 0xff;
        f32 r_f32 = f32(r_u32) / 255.0;
        f32 g_f32 = f32(g_u32) / 255.0;
        f32 b_f32 = f32(b_u32) / 255.0;



        // Voxel hit position

        f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
        f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);

        // We transform ray to model space:
        m4 ray_origin_mat = inverse(i.t_mat * i.r_mat);
        v3 ray_origin = (ray_origin_mat * v4(camera.xyz, 1.0)).xyz;

        m4 ray_direction_mat = inverse(i.r_mat); // Instead of transforming the box, the ray is transformed with its inverse
        v3 ray_direction_v3 = mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx);
        v3 ray_direction = normalize((ray_direction_mat * v4(ray_direction_v3, 0.0)).xyz);
        
        f32 ray_t = depth_24b * far; // TODO: far plane
        
        v3 hit_position_model_space = ray_origin + ray_t * ray_direction;
        


        // Voxel center position
        
        u32 voxel_index_x =  voxel_id_30b                        % i.grid_w;
        u32 voxel_index_y = (voxel_id_30b / i.grid_w)            % i.grid_h;
        u32 voxel_index_z = (voxel_id_30b / i.grid_w / i.grid_h) % i.grid_d;

        // We are in model space:
        // Start in center of grid, then to min corner of grid, then to center of first voxel, then to center of actual voxel
        f32 voxel_center_model_space_x = -f32(i.grid_w / 2) + 0.5 + f32(voxel_index_x);
        f32 voxel_center_model_space_y = -f32(i.grid_h / 2) + 0.5 + f32(voxel_index_y);
        f32 voxel_center_model_space_z = -f32(i.grid_d / 2) + 0.5 + f32(voxel_index_z);
        v3 voxel_center_model_space = v3(voxel_center_model_space_x, voxel_center_model_space_y, voxel_center_model_space_z);



        // Hit normal

        v3 normal_model_space = hit_position_model_space - voxel_center_model_space;
        if (abs(normal_model_space.x) > abs(normal_model_space.y))
        {
            normal_model_space.y = 0.0f;
            if (abs(normal_model_space.x) > abs(normal_model_space.z))
            {
                normal_model_space.x = sign(normal_model_space.x);
                normal_model_space.z = 0.0f;
            }
            else
            {
                normal_model_space.z = sign(normal_model_space.z);
                normal_model_space.x = 0.0f;
            }
        }
        else
        {
            normal_model_space.x = 0.0f;
            if (abs(normal_model_space.y) > abs(normal_model_space.z))
            {
                normal_model_space.y = sign(normal_model_space.y);
                normal_model_space.z = 0.0f;
            }
            else
            {
                normal_model_space.z = sign(normal_model_space.z);
                normal_model_space.y = 0.0f;
            }
        }
        v3 normal_world_space = normalize(i.r_mat * v4(normal_model_space, 0.0)).xyz;





        // TODO: hacked lighting
        v3 point_light = v3(128.0f, 40.0f, 0.0f);
        v3 hit_position_world_space = (i.t_mat * i.r_mat * v4(hit_position_model_space, 1.0)).xyz;
        v3 to_point_light = point_light - hit_position_world_space;
        f32 radiance = max(0.0f, dot(normalize(to_point_light), normal_world_space)) / (length(to_point_light) / 50.0);
        out_color = v4(v3(r_f32, g_f32, b_f32) * radiance, 1.0);







        // Visualize depth
        //out_color = v4(v3(min(1.0, 8.0 * depth_24b)), 1.0);

        // Visualize instance ID
        //u32 instance_id_hash0 = tg_hash_u32(instance_id_10b);
        //u32 instance_id_hash1 = tg_hash_u32(instance_id_hash0);
        //u32 instance_id_hash2 = tg_hash_u32(instance_id_hash1);
        //f32 instance_id_r = f32(instance_id_hash0) / 4294967295.0;
        //f32 instance_id_g = f32(instance_id_hash1) / 4294967295.0;
        //f32 instance_id_b = f32(instance_id_hash2) / 4294967295.0;
        //out_color = v4(instance_id_r, instance_id_g, instance_id_b, 1.0);

        // Visualize voxel ID
        //u32 voxel_id_hash0 = tg_hash_u32(voxel_id_30b);
        //u32 voxel_id_hash1 = tg_hash_u32(voxel_id_hash0);
        //u32 voxel_id_hash2 = tg_hash_u32(voxel_id_hash1);
        //f32 voxel_id_r = f32(voxel_id_hash0) / 4294967295.0;
        //f32 voxel_id_g = f32(voxel_id_hash1) / 4294967295.0;
        //f32 voxel_id_b = f32(voxel_id_hash2) / 4294967295.0;
        //out_color = v4(voxel_id_r, voxel_id_g, voxel_id_b, 1.0);
        
        // Visualize color LUT ID
        //f32 color_lut_id_normalized = f32(color_lut_id) / 255.0;
        //out_color = v4(0.0, 0.0, color_lut_id_normalized, 1.0);

        // Visualize color LUT
        //u32 color_u32 = color_lut[color_lut_id];
        //u32 r_u32 =  color_u32 >> 24;
        //u32 g_u32 = (color_u32 >> 16) & 0xff;
        //u32 b_u32 = (color_u32 >>  8) & 0xff;
        //f32 r_f32 = f32(r_u32) / 255.0;
        //f32 g_f32 = f32(g_u32) / 255.0;
        //f32 b_f32 = f32(b_u32) / 255.0;
        //out_color = v4(r_f32, g_f32, b_f32, 1.0);

        // Visualize how much of the voxel capacity is utilized
        //f32 voxel_load_normalized = min(1.0, f32(instance_data[instance_id_10b].first_voxel_id + voxel_id_30b) / 1073741823.0);
        //out_color = v4(voxel_load_normalized, 1.0 - voxel_load_normalized, 0.0, 1.0);

        // Visualize normals
        //out_color = v4(normal_world_space * 0.5 + 0.5, 1.0);

        // Visualize perfectly reflected ray
        v3 reflect = ray_direction - 2.0 * dot(ray_direction, normal_world_space) * normal_world_space;
        v3 dark = v3(0.0, 0.0, 0.2);
        v3 bright = v3(0.9, 0.8, 0.4);
        f32 brightness = dot(reflect, v3(0.0, 1.0, 0.0));
        out_color = v4(mix(dark, bright, brightness), 1.0);
    }
    else
    {
        out_color = v4(1.0, 0.0, 1.0, 1.0);
    }
}
