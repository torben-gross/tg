#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"



struct tg_instance_data
{
    m4     t_mat;
    m4     r_mat;
    m4     s_mat;
    u32    grid_w;
    u32    grid_h;
    u32    grid_d;
    u32    first_voxel_id;
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
    v4                               camera;
    v4                               sun_direction;
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
    u32 x = u32(gl_FragCoord.x);
    u32 y = u32(gl_FragCoord.y);
    u32 i = visibility_buffer_w * y + x;
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

        // Visualize depth
        //out_color = vec4(depth_24b, depth_24b, depth_24b, 1.0);

        // Visualize instance ID
        //u32 instance_id_hash0 = tg_hash_u32(instance_id_10b);
        //u32 instance_id_hash1 = tg_hash_u32(instance_id_hash0);
        //u32 instance_id_hash2 = tg_hash_u32(instance_id_hash1);
        //f32 instance_id_r = f32(instance_id_hash0) / 4294967295.0;
        //f32 instance_id_g = f32(instance_id_hash1) / 4294967295.0;
        //f32 instance_id_b = f32(instance_id_hash2) / 4294967295.0;
        //out_color = vec4(instance_id_r, instance_id_g, instance_id_b, 1.0);

        // Visualize voxel ID
        u32 voxel_id_hash0 = tg_hash_u32(voxel_id_30b);
        u32 voxel_id_hash1 = tg_hash_u32(voxel_id_hash0);
        u32 voxel_id_hash2 = tg_hash_u32(voxel_id_hash1);
        f32 voxel_id_r = f32(voxel_id_hash0) / 4294967295.0;
        f32 voxel_id_g = f32(voxel_id_hash1) / 4294967295.0;
        f32 voxel_id_b = f32(voxel_id_hash2) / 4294967295.0;
        out_color = vec4(voxel_id_r, voxel_id_g, voxel_id_b, 1.0);
        
        // Visualize color LUT ID
        //f32 color_lut_id_normalized = f32(color_lut_id) / 255.0;
        //out_color = v4(0.0, 0.0, color_lut_id_normalized, 1.0);

        // Visualize how much of the voxel capacity is utilized
        //f32 voxel_load_normalized = min(1.0, f32(instance_data[instance_id_10b].first_voxel_id + voxel_id_30b) / 1073741823.0);
        //out_color = v4(voxel_load_normalized, 1.0 - voxel_load_normalized, 0.0, 1.0);

        u32 color_u32 = color_lut[color_lut_id];
        u32 r_u32 =  color_u32 >> 24;
        u32 g_u32 = (color_u32 >> 16) & 0xff;
        u32 b_u32 = (color_u32 >>  8) & 0xff;
        f32 r_f32 = f32(r_u32) / 255.0;
        f32 g_f32 = f32(g_u32) / 255.0;
        f32 b_f32 = f32(b_u32) / 255.0;
        out_color = v4(r_f32, g_f32, b_f32, 1.0);
    }
    else
    {
        out_color = v4(1.0, 0.0, 1.0, 1.0);
    }
}
