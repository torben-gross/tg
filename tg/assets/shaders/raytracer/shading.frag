#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/cluster.inc"
#include "shaders/raytracer/collide.inc"
#include "shaders/raytracer/svo.inc"
#include "shaders/util.inc"




#define TG_DEBUG_SHOW_NONE               0
#define TG_DEBUG_SHOW_OBJECT_INDEX       1
#define TG_DEBUG_SHOW_DEPTH              2
#define TG_DEBUG_SHOW_CLUSTER_INDEX      3
#define TG_DEBUG_SHOW_VOXEL_INDEX        4
#define TG_DEBUG_SHOW_BLOCKS             5
#define TG_DEBUG_SHOW_COLOR_LUT_INDEX    6
#define TG_DEBUG_SHOW_COLOR              7
#define TG_DEBUG_SHOW_NORMAL             8
#define TG_DEBUG_SHOW_SHADING            9



TG_IN(0, v2 v_uv);

TG_SSBO( 4, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO( 5, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_SSBO( 6, tg_object_color_lut_ssbo_entry             object_color_lut_ssbo[];         );
TG_SSBO( 7, u32                                        color_lut_idx_cluster_ssbo[];    );
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
TG_UBO( 15, u32                                        debug_visualization;             );
TG_SSBO(16, u32                                        cluster_pointer_ssbo[];          );

TG_OUT(0, v4 out_color);

#include "shaders/raytracer/cluster_functions.inc"



float tg_distribution_ggx(float clamped_n_dot_h, float roughness)
{
    float a = roughness * roughness;
    float a_sqr = a * a;
    float denom = (clamped_n_dot_h * clamped_n_dot_h * (a_sqr - 1.0) + 1.0);
    denom = TG_PI * denom * denom;
	
    return a_sqr / denom;
}

float tg_geometry_schlick_ggf(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float denom = n_dot_v * (1.0 - k) + k;
	
    return n_dot_v / denom;
}

float tg_geometry_smith(float clamped_n_dot_v, float clamped_n_dot_l, float roughness)
{
    float ggx2 = tg_geometry_schlick_ggf(clamped_n_dot_v, roughness);
    float ggx1 = tg_geometry_schlick_ggf(clamped_n_dot_l, roughness);
	
    return ggx1 * ggx2;
}

float tg_fresnel_schlick(float u, float roughness)
{
    float f_lambda = 1.0 - roughness;
    return f_lambda + (1.0 - f_lambda) * pow(1.0 - u, 5.0);
}

vec3 tg_shade(vec3 n, vec3 v, vec3 l, vec3 diffuse_albedo, vec3 specular_albedo, float metallic, float roughness, vec3 radiance)
{
    vec3 h = normalize(v + l);

    float h_dot_n = dot(h, n);
    float h_dot_v = dot(h, v);
    float l_dot_n = dot(n, l);
    float n_dot_v = dot(n, v);

    float clamped_h_dot_n = clamp(h_dot_n, 0.0, 1.0);
    float clamped_h_dot_v = clamp(h_dot_v, 0.0, 1.0);
    float clamped_l_dot_n = clamp(l_dot_n, 0.0, 1.0);
    float clamped_n_dot_v = clamp(n_dot_v, 0.0, 1.0);

    float d = tg_distribution_ggx(clamped_h_dot_n, roughness);
    float f = tg_fresnel_schlick(clamped_n_dot_v, roughness);
    float g = tg_geometry_smith(clamped_n_dot_v, clamped_l_dot_n, roughness);
    float dfg = d * f * g;
    float denominator = 4.0 * clamped_n_dot_v * clamped_l_dot_n;
    vec3 specular = specular_albedo * (dfg / max(denominator, 0.001));

    vec3 diffuse = (1.0 - f) * (1.0 - metallic) * diffuse_albedo / TG_PI;

    return (diffuse + specular) * radiance * clamped_l_dot_n;
}



void main()
{
    u32 v_buf_idx           = visibility_buffer_w * u32(gl_FragCoord.y) + u32(gl_FragCoord.x);
    u64 packed_data         = visibility_buffer_data[v_buf_idx];
    f32 depth_24b           = f32(packed_data >> u64(40)) / 16777215.0;
	u32 cluster_pointer_31b = u32(packed_data >> u64( 9)) & 2147483647;
	u32 voxel_idx_9b        = u32(packed_data           ) & 511;

    if (depth_24b < 1.0)
    {
		u32 cluster_idx = cluster_pointer_ssbo[cluster_pointer_31b];
		u32 object_idx = cluster_idx_to_object_idx_ssbo[cluster_idx].object_idx;
		tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];
		
		u64 absolute_voxel_idx = u64(cluster_idx) * u64(TG_N_PRIMITIVES_PER_CLUSTER) + u64(voxel_idx_9b);
		u32 color_lut_idx_slot_idx = u32(absolute_voxel_idx / u64(4));
		u32 color_lut_idx_slot = color_lut_idx_cluster_ssbo[color_lut_idx_slot_idx];
		u32 color_lut_idx = (color_lut_idx_slot >> ((absolute_voxel_idx % 4) * 8)) & 255;
		u32 packed_color = object_color_lut_ssbo[color_lut_idx].packed_color_entry;
        f32 color_r = f32( packed_color >> 24        ) / 255.0;
        f32 color_g = f32((packed_color >> 16) & 0xff) / 255.0;
        f32 color_b = f32((packed_color >>  8) & 0xff) / 255.0;



        // Voxel hit position
		
		
		
		
		f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
	    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);
		
		u32 relative_cluster_pointer = cluster_pointer_31b - object_data.first_cluster_pointer;
		u32 relative_cluster_pointer_x = relative_cluster_pointer % object_data.n_cluster_pointers_per_dim.x;
		u32 relative_cluster_pointer_y = (relative_cluster_pointer / object_data.n_cluster_pointers_per_dim.x) % object_data.n_cluster_pointers_per_dim.y;
		u32 relative_cluster_pointer_z = relative_cluster_pointer / (object_data.n_cluster_pointers_per_dim.x * object_data.n_cluster_pointers_per_dim.y); // Note: We don't need modulo here, because z doesn't loop
		
		f32 relative_cluster_offset_x = f32(relative_cluster_pointer_x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
		f32 relative_cluster_offset_y = f32(relative_cluster_pointer_y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
		f32 relative_cluster_offset_z = f32(relative_cluster_pointer_z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT);
		v3 relative_cluster_offset = v3(relative_cluster_offset_x, relative_cluster_offset_y, relative_cluster_offset_z);
		
		v3 cluster_extent = v3(f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT));
		v3 cluster_half_extent = cluster_extent * v3(0.5);
		v3 n_cluster_pointers_per_dim = v3(f32(object_data.n_cluster_pointers_per_dim.x), f32(object_data.n_cluster_pointers_per_dim.y), f32(object_data.n_cluster_pointers_per_dim.z));
		v3 object_half_extent = cluster_half_extent * n_cluster_pointers_per_dim;
		
		// We transform the ray from world space in such a way, that the cluster is assumed to have its
		// min at the origin and an extent of TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT. This way, we can
		// traverse the grid right away.
		// 1. Translate by objects inverse translation
		// 2. Rotate by objects inverse rotation
		// 3. Translate by objects half extent (add)
		// 3. Translate by clusters offset (sub)
#define TG_TRANSLATE(translation) m4(v4(1.0, 0.0, 0.0, 0.0), v4(0.0, 1.0, 0.0, 0.0), v4(0.0, 0.0, 1.0, 0.0), v4((translation), 1.0))
		m4 ws2ms0 = TG_TRANSLATE(-object_data.translation);
		m4 ws2ms1 = inverse(object_data.rotation);
		m4 ws2ms2 = TG_TRANSLATE(object_half_extent);
		m4 ws2ms3 = TG_TRANSLATE(-relative_cluster_offset);
		m4 ws2ms = ws2ms3 * ws2ms2 * ws2ms1 * ws2ms0;
#undef TG_TRANSLATE

		v3 ray_origin_ws = camera_ubo.camera.xyz;
		v3 ray_origin_ms = (ws2ms * v4(ray_origin_ws, 1.0)).xyz;

		v3 ray_direction_ws = tg_lerp_corner_ray_directions_nn(camera_ubo.ray_bl.xyz, camera_ubo.ray_br.xyz, camera_ubo.ray_tr.xyz, camera_ubo.ray_tl.xyz, fx, fy);
		v3 ray_direction_ms = normalize((ws2ms * v4(ray_direction_ws, 0.0)).xyz);
		
		u32 relative_voxel_idx_x =  voxel_idx_9b       % 8;
		u32 relative_voxel_idx_y = (voxel_idx_9b /  8) % 8;
		u32 relative_voxel_idx_z = (voxel_idx_9b / 64);
		
		v3 voxel_min = v3(f32(relative_voxel_idx_x), f32(relative_voxel_idx_y), f32(relative_voxel_idx_z));
		v3 voxel_max = voxel_min + v3(1.0, 1.0, 1.0);
		
		v3 normal_ws = v3(0.0);
		
		f32 enter, exit;
		if (tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, voxel_min, voxel_max, enter, exit))
		{
			v3 hit_position_ms = enter > 0.0 ? ray_origin_ms + enter * ray_direction_ms : ray_origin_ms;
			v3 voxel_center_ms = voxel_min + v3(0.5, 0.5, 0.5);
			
			v3 normal_ms = hit_position_ms - voxel_center_ms;
			
			if (abs(normal_ms.x) > abs(normal_ms.y))
			{
			    normal_ms.y = 0.0f;
			    if (abs(normal_ms.x) > abs(normal_ms.z))
			    {
			        normal_ms.x = sign(normal_ms.x);
			        normal_ms.z = 0.0f;
			    }
			    else
			    {
			        normal_ms.z = sign(normal_ms.z);
			        normal_ms.x = 0.0f;
			    }
			}
			else
			{
			    normal_ms.x = 0.0f;
			    if (abs(normal_ms.y) > abs(normal_ms.z))
			    {
			        normal_ms.y = sign(normal_ms.y);
			        normal_ms.z = 0.0f;
			    }
			    else
			    {
			        normal_ms.z = sign(normal_ms.z);
			        normal_ms.y = 0.0f;
			    }
			}
			normal_ws = normalize(object_data.rotation * v4(normal_ms, 0.0)).xyz;
		}
		
		v3 hit_position_ws = ray_origin_ws + depth_24b * camera_ubo.far * ray_direction_ws;

		if (debug_visualization == TG_DEBUG_SHOW_OBJECT_INDEX)
		{
			u32 object_idx_hash0 = tg_hash_u32(object_idx);
			u32 object_idx_hash1 = tg_hash_u32(object_idx_hash0);
			u32 object_idx_hash2 = tg_hash_u32(object_idx_hash1);
			f32 object_idx_r = f32(object_idx_hash0) / 4294967295.0;
			f32 object_idx_g = f32(object_idx_hash1) / 4294967295.0;
			f32 object_idx_b = f32(object_idx_hash2) / 4294967295.0;
			out_color = v4(object_idx_r, object_idx_g, object_idx_b, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_DEPTH)
		{
			out_color = v4(v3(min(1.0, 8.0 * depth_24b)), 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_CLUSTER_INDEX || debug_visualization == TG_DEBUG_SHOW_BLOCKS)
		{
			u32 cluster_idx_hash0 = tg_hash_u32(cluster_idx);
			u32 cluster_idx_hash1 = tg_hash_u32(cluster_idx_hash0);
			u32 cluster_idx_hash2 = tg_hash_u32(cluster_idx_hash1);
			f32 cluster_idx_r = f32(cluster_idx_hash0) / 4294967295.0;
			f32 cluster_idx_g = f32(cluster_idx_hash1) / 4294967295.0;
			f32 cluster_idx_b = f32(cluster_idx_hash2) / 4294967295.0;
			out_color = v4(cluster_idx_r, cluster_idx_g, cluster_idx_b, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_VOXEL_INDEX)
		{
			u32 voxel_idx_hash0 = tg_hash_u32(voxel_idx_9b);
			u32 voxel_idx_hash1 = tg_hash_u32(voxel_idx_hash0);
			u32 voxel_idx_hash2 = tg_hash_u32(voxel_idx_hash1);
			f32 voxel_idx_r = f32(voxel_idx_hash0) / 4294967295.0;
			f32 voxel_idx_g = f32(voxel_idx_hash1) / 4294967295.0;
			f32 voxel_idx_b = f32(voxel_idx_hash2) / 4294967295.0;
			out_color = v4(voxel_idx_r, voxel_idx_g, voxel_idx_b, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_COLOR_LUT_INDEX)
		{
			u32 color_lut_idx_hash0 = tg_hash_u32(color_lut_idx);
			u32 color_lut_idx_hash1 = tg_hash_u32(color_lut_idx_hash0);
			u32 color_lut_idx_hash2 = tg_hash_u32(color_lut_idx_hash1);
			f32 color_lut_idx_r = f32(color_lut_idx_hash0) / 4294967295.0;
			f32 color_lut_idx_g = f32(color_lut_idx_hash1) / 4294967295.0;
			f32 color_lut_idx_b = f32(color_lut_idx_hash2) / 4294967295.0;
			out_color = v4(color_lut_idx_r, color_lut_idx_g, color_lut_idx_b, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_COLOR)
		{
			out_color = v4(color_r, color_g, color_b, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_NORMAL)
		{
			out_color = v4(normal_ws * 0.5 + 0.5, 1.0);
		}
		else if (debug_visualization == TG_DEBUG_SHOW_SHADING)
		{
			const f32 metallic = 0.1;
			
			v3 n = normal_ws;
			v3 v = normalize(camera_ubo.camera.xyz - hit_position_ws);
			v3 l = normalize(v3(0., 0.8, 0.3));
			v3 albedo = v3(1.0);
			v3 specular_albedo = mix(vec3(0.04), albedo, metallic);
			f32 roughness = 0.8;
			v3 radiance = v3(3.0);
			
			v3 lo = tg_shade(n, v, l, albedo, specular_albedo, metallic, roughness, radiance);
			v3 ambient = 0.1 * albedo;
			out_color = v4(ambient + lo, 1.0);
		}
		else
		{
			const f32 metallic = 0.1;
			
			v3 n = normal_ws;
			v3 v = normalize(camera_ubo.camera.xyz - hit_position_ws);
			v3 l = normalize(v3(0., 0.8, 0.3));
			v3 albedo = v3(color_r, color_g, color_b);
			v3 specular_albedo = mix(vec3(0.04), albedo, metallic);
			f32 roughness = 0.8;
			v3 radiance = v3(3.0);
			
			v3 lo = tg_shade(n, v, l, albedo, specular_albedo, metallic, roughness, radiance);
			v3 ambient = 0.1 * albedo;
			out_color = v4(ambient + lo, 1.0);
		}

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
