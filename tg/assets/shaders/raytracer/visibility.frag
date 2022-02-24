#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/cluster.inc"
#include "shaders/raytracer/collide.inc"
#include "shaders/util.inc"

TG_IN_FLAT(0, u32    v_cluster_idx);

TG_SSBO(0, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO(1, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_SSBO(2, tg_voxel_cluster_ssbo_entry                voxel_cluster_ssbo[];            );
TG_SSBO(3,
	u32    visibility_buffer_w;
	u32    visibility_buffer_h;
	u64    visibility_buffer_data[];                                                   );
TG_UBO( 5, tg_camera_ubo                              camera_ubo;                      );

void main()
{
	u32 object_idx = cluster_idx_to_object_idx_ssbo[v_cluster_idx].object_idx;
    tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];
    
    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);



    m4 r_mat = object_data.rotation;
    m4 t_mat = m4(
        v4(1.0, 0.0, 0.0, 0.0),                               // col0
        v4(0.0, 1.0, 0.0, 0.0),                               // col1
        v4(0.0, 0.0, 1.0, 0.0),                               // col2
        v4(object_data.translation, 1.0)); // col3
    // TODO: can we just set the translation col in the end? saves some multiplications



    // Instead of transforming the aabb, the ray is transformed with its inverse
    m4 ray_origin_ws2ms_mat = inverse(t_mat * r_mat);
    v3 ray_origin_ws = camera_ubo.camera.xyz;
    v3 ray_origin_ms = (ray_origin_ws2ms_mat * vec4(ray_origin_ws, 1.0)).xyz;

    m4 ray_direction_ws2ms_mat = inverse(r_mat);
    v3 ray_direction_ws = mix(
        mix(camera_ubo.ray00.xyz,
            camera_ubo.ray10.xyz,
            fy),
        mix(camera_ubo.ray01.xyz,
            camera_ubo.ray11.xyz,
            fy),
        fx);
    v3 ray_direction_ms = normalize((ray_direction_ws2ms_mat * v4(ray_direction_ws, 0.0)).xyz);
	
	f32 half_extent_x = f32(object_data.n_clusters_per_dim.x * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);
	f32 half_extent_y = f32(object_data.n_clusters_per_dim.y * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);
	f32 half_extent_z = f32(object_data.n_clusters_per_dim.z * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT / 2);

    v3 aabb_max = v3(half_extent_x, half_extent_y, half_extent_z);
    v3 aabb_min = -aabb_max;

    f32 d = 1.0;
    u32 voxel_idx = 0;
    f32 enter, exit;
    if (tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, aabb_min, aabb_max, enter, exit))
    {
		// TODO: remove
		d = d / camera_ubo.far;
		voxel_idx = 111;
		
		
        //// supercover
        //// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
        //
        //v3 hit = t_aabb > 0.0 ? r.o + t_aabb * r.d : r.o;
        //v3 xyz = clamp(floor(hit), aabb_min, aabb_max - v3(1.0));
		//
        //hit += aabb_max;
        //xyz += aabb_max;
		//
        //i32 x = i32(xyz.x);
        //i32 y = i32(xyz.y);
        //i32 z = i32(xyz.z);
		//
        //i32 step_x;
        //i32 step_y;
        //i32 step_z;
		//
        //f32 t_max_x = TG_F32_MAX;
        //f32 t_max_y = TG_F32_MAX;
        //f32 t_max_z = TG_F32_MAX;
		//
        //f32 t_delta_x = TG_F32_MAX;
        //f32 t_delta_y = TG_F32_MAX;
        //f32 t_delta_z = TG_F32_MAX;
		//
        //if (r.d.x > 0.0)
        //{
        //    step_x = 1;
        //    t_max_x = t_aabb + (f32(x + 1) - hit.x) / r.d.x;
        //    t_delta_x = 1.0 / r.d.x;
        //}
        //else
        //{
        //    step_x = -1;
        //    t_max_x = t_aabb + (hit.x - f32(x)) / -r.d.x;
        //    t_delta_x = 1.0 / -r.d.x;
        //}
        //if (r.d.y > 0.0)
        //{
        //    step_y = 1;
        //    t_max_y = t_aabb + (f32(y + 1) - hit.y) / r.d.y;
        //    t_delta_y = 1.0 / r.d.y;
        //}
        //else
        //{
        //    step_y = -1;
        //    t_max_y = t_aabb + (hit.y - f32(y)) / -r.d.y;
        //    t_delta_y = 1.0 / -r.d.y;
        //}
        //if (r.d.z > 0.0)
        //{
        //    step_z = 1;
        //    t_max_z = t_aabb + (f32(z + 1) - hit.z) / r.d.z;
        //    t_delta_z = 1.0 / r.d.z;
        //}
        //else
        //{
        //    step_z = -1;
        //    t_max_z = t_aabb + (hit.z - f32(z)) / -r.d.z;
        //    t_delta_z = 1.0 / -r.d.z;
        //}
		//
        //while (true)
        //{
        //    d = 0.5;
        //    voxel_id = 53;
        //    break;
		//
        //    //u32 vox_id    = object_data_entry_REPLACE_ME.grid_w * object_data_entry_REPLACE_ME.grid_h * z + object_data_entry_REPLACE_ME.grid_w * y + x;
        //    //u32 vox_idx   = object_data_entry_REPLACE_ME.first_voxel_id + vox_id;
        //    //u32 bits_idx  = vox_idx / 32;
        //    //u32 bit_idx   = vox_idx % 32;
        //    //u32 bits      = voxel_data[bits_idx];
        //    //u32 bit       = bits & (1 << bit_idx);
        //    //if (bit != 0)
        //    //{
        //    //    v3 voxel_min = aabb_min + v3(f32(x    ), f32(y    ), f32(z    ));
        //    //    v3 voxel_max = aabb_min + v3(f32(x + 1), f32(y + 1), f32(z + 1));
        //    //    tg_aabb voxel = tg_aabb(voxel_min, voxel_max);
        //    //    f32 t_voxel;
        //    //    tg_intersect_ray_aabb(r, voxel, t_voxel); // TODO: We should in this case adjust the function to potentially return enter AND exit, because if we are inside of the voxel, we receive a negative 't_voxel', which results in wrong depth. Might be irrelevant...
        //    //    d = min(1.0, t_voxel / far);
        //    //    voxel_id = vox_id;
        //    //    break;
        //    //}
        //    //if (t_max_x < t_max_y)
        //    //{
        //    //    if (t_max_x < t_max_z)
        //    //    {
        //    //        t_max_x += t_delta_x;
        //    //        x += step_x;
        //    //        if (x < 0 || x >= object_data_entry_REPLACE_ME.grid_w) break;
        //    //    }
        //    //    else
        //    //    {
        //    //        t_max_z += t_delta_z;
        //    //        z += step_z;
        //    //        if (z < 0 || z >= object_data_entry_REPLACE_ME.grid_d) break;
        //    //    }
        //    //}
        //    //else
        //    //{
        //    //    if (t_max_y < t_max_z)
        //    //    {
        //    //        t_max_y += t_delta_y;
        //    //        y += step_y;
        //    //        if (y < 0 || y >= object_data_entry_REPLACE_ME.grid_h) break;
        //    //    }
        //    //    else
        //    //    {
        //    //        t_max_z += t_delta_z;
        //    //        z += step_z;
        //    //        if (z < 0 || z >= object_data_entry_REPLACE_ME.grid_d) break;
        //    //    }
        //    //}
        //}
    }
    
    if (d != 1.0)
    {
        // Layout : 24 bits depth | 10 bits object id | 30 bits relative voxel_id

        u64 depth_24b       = u64(d * 16777215.0) << u64(40);
		u64 cluster_idx_31b = u64(v_cluster_idx)  << u64( 9);
		u64 voxel_idx_9b    = u64(voxel_idx)                ;
		u64 packed_data     = depth_24b | cluster_idx_31b | voxel_idx_9b;

        u32 x = u32(gl_FragCoord.x);
        u32 y = u32(gl_FragCoord.y);
        u32 pixel_idx = visibility_buffer_w * y + x;
        atomicMin(visibility_buffer_data[pixel_idx], packed_data);
    }
	
	u32 x = u32(gl_FragCoord.x);
	u32 y = u32(gl_FragCoord.y);
	u32 pixel_idx = visibility_buffer_w * y + x;
	atomicMin(visibility_buffer_data[pixel_idx], 0xffffffff);
}
