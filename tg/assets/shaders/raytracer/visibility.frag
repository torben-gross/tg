#version 450

#include "shaders/common.inc"
#include "shaders/commoni64.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/cluster.inc"
#include "shaders/raytracer/collide.inc"
#include "shaders/util.inc"

TG_IN_FLAT(0, u32    v_cluster_pointer);

TG_SSBO(0, u32                                        cluster_pointer_ssbo[];          );
TG_SSBO(1, tg_cluster_idx_to_object_idx_ssbo_entry    cluster_idx_to_object_idx_ssbo[];);
TG_SSBO(2, tg_object_data_ssbo_entry                  object_data_ssbo[];              );
TG_SSBO(3, tg_voxel_cluster_ssbo_entry                voxel_cluster_ssbo[];            );
TG_SSBO(4,
	u32    visibility_buffer_w;
	u32    visibility_buffer_h;
	u64    visibility_buffer_data[];                                                   );
TG_UBO( 6, tg_camera_ubo                              camera_ubo;                      );

void main()
{
	u32 cluster_idx = cluster_pointer_ssbo[v_cluster_pointer];
	u32 object_idx = cluster_idx_to_object_idx_ssbo[cluster_idx].object_idx;

    tg_object_data_ssbo_entry object_data = object_data_ssbo[object_idx];
    
    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);
	
	u32 relative_cluster_pointer = v_cluster_pointer - object_data.first_cluster_pointer;
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

    v3 cluster_min = v3(0.0);
    v3 cluster_max = v3(f32(TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT));

    // TODO: Why does frustum culling not work anymore?!
    f32 d = TG_F32_MAX;
    u32 voxel_idx = 0;
    f32 enter, exit;
    if (tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, cluster_min, cluster_max, enter, exit))
    {
        // Supercover
        // https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf
        
        v3 hit = enter > 0.0 ? ray_origin_ms + enter * ray_direction_ms : ray_origin_ms;
        v3 xyz = clamp(floor(hit), cluster_min, cluster_max - v3(1.0));
		
        i32 x = i32(xyz.x);
        i32 y = i32(xyz.y);
        i32 z = i32(xyz.z);
		
        i32 step_x = 0;
        i32 step_y = 0;
        i32 step_z = 0;
		
        f32 t_max_x = TG_F32_MAX;
        f32 t_max_y = TG_F32_MAX;
        f32 t_max_z = TG_F32_MAX;
		
        f32 t_delta_x = TG_F32_MAX;
        f32 t_delta_y = TG_F32_MAX;
        f32 t_delta_z = TG_F32_MAX;
		
        if (ray_direction_ms.x > 0.0)
        {
            step_x = 1;
            t_max_x = enter + (f32(x + 1) - hit.x) / ray_direction_ms.x;
            t_delta_x = 1.0 / ray_direction_ms.x;
        }
        else if (ray_direction_ms.x < 0.0)
        {
            step_x = -1;
            t_max_x = enter + (hit.x - f32(x)) / -ray_direction_ms.x;
            t_delta_x = 1.0 / -ray_direction_ms.x;
        }
        if (ray_direction_ms.y > 0.0)
        {
            step_y = 1;
            t_max_y = enter + (f32(y + 1) - hit.y) / ray_direction_ms.y;
            t_delta_y = 1.0 / ray_direction_ms.y;
        }
        else if (ray_direction_ms.y < 0.0)
        {
            step_y = -1;
            t_max_y = enter + (hit.y - f32(y)) / -ray_direction_ms.y;
            t_delta_y = 1.0 / -ray_direction_ms.y;
        }
        if (ray_direction_ms.z > 0.0)
        {
            step_z = 1;
            t_max_z = enter + (f32(z + 1) - hit.z) / ray_direction_ms.z;
            t_delta_z = 1.0 / ray_direction_ms.z;
        }
        else if (ray_direction_ms.z < 0.0)
        {
            step_z = -1;
            t_max_z = enter + (hit.z - f32(z)) / -ray_direction_ms.z;
            t_delta_z = 1.0 / -ray_direction_ms.z;
        }
		
		u64 cluster_offset_in_voxels = u64(cluster_idx) * u64(TG_N_PRIMITIVES_PER_CLUSTER);

        while (true)
        {
            u32 relative_voxel_idx = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * z + TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * y + x;
			u64 vox_idx            = cluster_offset_in_voxels + u64(relative_voxel_idx); // TODO: do we need 64 bit here? also above
			u32 slot_idx           = u32(vox_idx / 32);
			u32 bit_idx            = u32(vox_idx % 32);
			u32 slot               = voxel_cluster_ssbo[slot_idx].voxel_cluster_data_entry;
			u32 bit                = slot & (1 << bit_idx);
			if (bit != 0)
			{
                v3 voxel_min = cluster_min + v3(f32(x    ), f32(y    ), f32(z    ));
                v3 voxel_max = cluster_min + v3(f32(x + 1), f32(y + 1), f32(z + 1));
                f32 voxel_enter;
                f32 voxel_exit;
                tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, voxel_min, voxel_max, voxel_enter, voxel_exit);
                d = max(0.0, voxel_enter / camera_ubo.far);
                voxel_idx = relative_voxel_idx;
                break;
			}

            if (t_max_x < t_max_y)
            {
                if (t_max_x < t_max_z)
                {
                    t_max_x += t_delta_x;
                    x += step_x;
                    if (x < 0 || x >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT) break;
                }
            }
            else
            {
                if (t_max_y < t_max_z)
                {
                    t_max_y += t_delta_y;
                    y += step_y;
                    if (y < 0 || y >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT) break;
                }
                else
                {
                    t_max_z += t_delta_z;
                    z += step_z;
                    if (z < 0 || z >= TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT) break;
                }
            }
        }
    }
    
    if (d <= 1.0)
    {
        // Layout : 24 bits depth | 10 bits object id | 30 bits relative voxel_id

        u64 depth_24b       = u64(d * 16777215.0) << u64(40);
		u64 cluster_idx_31b = u64(cluster_idx)  << u64( 9);
		u64 voxel_idx_9b    = u64(voxel_idx);
		u64 packed_data     = depth_24b | cluster_idx_31b | voxel_idx_9b;

        u32 x = u32(gl_FragCoord.x);
        u32 y = u32(gl_FragCoord.y);
        u32 pixel_idx = visibility_buffer_w * y + x;
        atomicMin(visibility_buffer_data[pixel_idx], packed_data);
    }
}
