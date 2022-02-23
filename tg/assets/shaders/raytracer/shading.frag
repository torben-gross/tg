#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/collide.inc"
#include "shaders/raytracer/svo.inc"
#include "shaders/util.inc"



TG_IN(0, v2 v_uv);

TG_SSBO( 4, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO( 5, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_SSBO( 6, tg_object_color_lut_ssbo_entry             object_color_lut_ssbo[];         );
TG_SSBO( 7, tg_color_lut_idx_cluster_ssbo_entry        color_lut_idx_cluster_ssbo[];    );
TG_SSBO( 8,
	u32    visibility_buffer_w;
	u32    visibility_buffer_h;
	u64    visibility_buffer_data[];                                                    );
TG_UBO(  9, tg_camera_ubo                              camera_ubo;                      );
TG_UBO( 10, tg_environment_ubo                         environment_ubo;                 );
TG_SSBO(11, tg_svo                                     svo;                             );
TG_SSBO(12, tg_svo_node                                nodes[];                         );
TG_SSBO(13, tg_svo_leaf_node_data                      leaf_node_data[];                );
TG_SSBO(14, u32                                        voxel_data[];                    );

TG_OUT(0, v4 out_color);



void main()
{
    u32 v_buf_idx       = visibility_buffer_w * u32(gl_FragCoord.y) + u32(gl_FragCoord.x);
    u64 packed_data     = visibility_buffer_data[v_buf_idx];
    f32 depth_24b       = f32(packed_data >> u64(40)) / 16777215.0;
	u32 cluster_idx_31b = u32(packed_data >> u64( 9)) & 2147483647;
	u32 voxel_idx_9b    = u32(packed_data           ) & 511;
	u32 object_idx      = cluster_idx_to_object_idx_ssbo[cluster_idx_31b].object_idx;

    if (depth_24b < 1.0)
    {
		tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];
        
		//tg_instance_data instance_ddd = instance_data[instance_id_10b];
        //
        //u32 color_lut_id_slot_idx = (instance_ddd.first_voxel_id + voxel_id_30b) / 4;
        //u32 color_lut_id_byte_idx = voxel_id_30b % 4;
        //u32 color_lut_id_shift = color_lut_id_byte_idx * 8;
		//
        //u32 color_lut_id_slot = color_lut_ids[color_lut_id_slot_idx];
        //u32 color_lut_id = (color_lut_id_slot >> color_lut_id_shift) & 255;
		//
        //u32 color_u32 = color_lut[color_lut_id];
        //u32 r_u32 =  color_u32 >> 24;
        //u32 g_u32 = (color_u32 >> 16) & 0xff;
        //u32 b_u32 = (color_u32 >>  8) & 0xff;
        //f32 r_f32 = f32(r_u32) / 255.0;
        //f32 g_f32 = f32(g_u32) / 255.0;
        //f32 b_f32 = f32(b_u32) / 255.0;



        // Voxel hit position

        //f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
        //f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);
		//
        //m4 tr_mat = instance_ddd.t_mat * instance_ddd.r_mat;
		//
        //// We transform ray to model space:
        //m4 ray_origin_ws2ms_mat = inverse(tr_mat);
        //v3 ray_origin_ws = camera.xyz;
        //v3 ray_origin_ms = (ray_origin_ws2ms_mat * v4(ray_origin_ws, 1.0)).xyz;
		//
        //m4 ray_direction_ws2ms_mat = inverse(instance_ddd.r_mat);
        //v3 ray_direction_ws = normalize(mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx));
        //v3 ray_direction_ms = (ray_direction_ws2ms_mat * v4(ray_direction_ws, 0.0)).xyz;
        //
        //f32 ray_t = depth_24b * far; // TODO: far plane
        //v3 hit_position_ws = ray_origin_ws + ray_t * ray_direction_ws;
        //v3 hit_position_ms = ray_origin_ms + ray_t * ray_direction_ms;
        //
		//
		//
        //// Voxel center position
        //
        //u32 voxel_index_x =  voxel_id_30b                                              % instance_ddd.grid_w;
        //u32 voxel_index_y = (voxel_id_30b / instance_ddd.grid_w)                       % instance_ddd.grid_h;
        //u32 voxel_index_z = (voxel_id_30b / instance_ddd.grid_w / instance_ddd.grid_h) % instance_ddd.grid_d;
		//
        //// We are in model space. Start in center of grid, then to min corner of grid, then to center of first voxel, then to center of actual voxel
        //f32 voxel_center_ms_x = -f32(instance_ddd.grid_w / 2) + 0.5 + f32(voxel_index_x);
        //f32 voxel_center_ms_y = -f32(instance_ddd.grid_h / 2) + 0.5 + f32(voxel_index_y);
        //f32 voxel_center_ms_z = -f32(instance_ddd.grid_d / 2) + 0.5 + f32(voxel_index_z);
        //v3 voxel_center_ms = v3(voxel_center_ms_x, voxel_center_ms_y, voxel_center_ms_z);
		//
		//
		//
        //// Hit normal
		//
        //v3 normal_ms = hit_position_ms - voxel_center_ms;
        //
        //if (abs(normal_ms.x) > abs(normal_ms.y))
        //{
        //    normal_ms.y = 0.0f;
        //    if (abs(normal_ms.x) > abs(normal_ms.z))
        //    {
        //        normal_ms.x = sign(normal_ms.x);
        //        normal_ms.z = 0.0f;
        //    }
        //    else
        //    {
        //        normal_ms.z = sign(normal_ms.z);
        //        normal_ms.x = 0.0f;
        //    }
        //}
        //else
        //{
        //    normal_ms.x = 0.0f;
        //    if (abs(normal_ms.y) > abs(normal_ms.z))
        //    {
        //        normal_ms.y = sign(normal_ms.y);
        //        normal_ms.z = 0.0f;
        //    }
        //    else
        //    {
        //        normal_ms.z = sign(normal_ms.z);
        //        normal_ms.y = 0.0f;
        //    }
        //}
        //v3 normal_ws = normalize(instance_ddd.r_mat * v4(normal_ms, 0.0)).xyz;
        //
        //
        //
        //
        //// TODO: hacked lighting
        //v3 to_light_dir_ws = normalize(v3(-0.4, 1.0, 0.1));
        //
        //f32 radiance = max(0.0f, dot(to_light_dir_ws, normal_ws));
        //f32 min_advance = 1.73205080757; // TODO: We may want to compute the min/max of the rotated voxel, and determine, how far we have to go, such that it does not touch that resulting cell of the SVO grid
        //
        //tg_rand_xorshift32 rand;
        //tgm_rand_xorshift32_init(v_buf_idx + 1, rand);
        //
        //const u32 n_rays = 0;
        //const u32 n_bounces = 1;
        //const u32 max_n_bounces = n_rays * n_bounces;
        //
        //f32 shadow_sum = 0.0;
        //for (u32 ray_idx = 0; ray_idx < n_rays; ray_idx++)
        //{
        //    v3 new_position = hit_position_ws;
        //    v3 new_normal = normal_ws;
        //    v3 new_dir = v3(0.0, 0.0, 0.0);
        //
        //    for (u32 bounce_idx = 0; bounce_idx < n_bounces; bounce_idx++)
        //    {
        //        for (u32 compute_new_ray_dir_idx = 0; compute_new_ray_dir_idx < 32; compute_new_ray_dir_idx++)
        //        {
        //            new_dir.x = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
        //            new_dir.y = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
        //            new_dir.z = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
        //            
        //            new_dir = normalize(new_dir);
        //            if (dot(new_dir, new_normal) > 0.0)
        //            {
        //                break;
        //            }
        //        }
        //        if (dot(new_dir, new_normal) <= 0.0)
        //        {
        //            new_dir = new_normal;
        //        }
        //
        //        v3 hit_position;
        //        v3 hit_normal;
        //        if (tg_raytrace_ssvo(new_position + min_advance * new_dir, new_dir, hit_position, hit_normal) == 1.0)
        //        {
        //            f32 contrib = 1.0f - (f32(bounce_idx) / f32(n_bounces));
        //            shadow_sum += contrib * dot(new_dir, v3(0.0, 1.0, 0.0)) * 0.25 + 0.75;
        //            break;
        //        }
        //
        //        new_position = hit_position;
        //        new_normal = hit_normal;
        //    }
        //}
        //if (n_rays > 0.0 && n_bounces > 0.0)
        //{
        //    radiance *= shadow_sum * 0.8;
        //}
        //radiance = radiance * 0.9 + 0.1;
        ////out_color = v4(v3(radiance), 1.0);
        //out_color = v4(v3(r_f32, g_f32, b_f32) * radiance, 1.0);







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
        u32 voxel_idx_hash0 = tg_hash_u32(voxel_idx_9b);
        u32 voxel_idx_hash1 = tg_hash_u32(voxel_idx_hash0);
        u32 voxel_idx_hash2 = tg_hash_u32(voxel_idx_hash1);
        f32 voxel_idx_r = f32(voxel_idx_hash0) / 4294967295.0;
        f32 voxel_idx_g = f32(voxel_idx_hash1) / 4294967295.0;
        f32 voxel_idx_b = f32(voxel_idx_hash2) / 4294967295.0;
        out_color = v4(voxel_idx_r, voxel_idx_g, voxel_idx_b, 1.0);
        
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
        //out_color = v4(normal_ws * 0.5 + 0.5, 1.0);

        // Visualize perfectly reflected ray
        //v3 n = normal_ws;
        //v3 reflect = ray_direction - 2.0 * dot(ray_direction, n) * n;
        //v3 dark = v3(0.0, 0.0, 0.2);
        //v3 bright = v3(0.9, 0.8, 0.4);
        //f32 brightness = dot(reflect, v3(0.0, 1.0, 0.0));
        //out_color = v4(mix(dark, bright, brightness), 1.0);
    }
    else
    {
        out_color = v4(1.0, 0.0, 1.0, 1.0);
    }
}
