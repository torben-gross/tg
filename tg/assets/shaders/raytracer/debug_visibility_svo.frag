#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"
#include "shaders/raytracer/buffers.inc"
#include "shaders/raytracer/collide.inc"
#include "shaders/raytracer/svo.inc"
#include "shaders/util.inc"

TG_IN_FLAT(0, u32 v_instance_id);

TG_UBO( 0, tg_camera_ubo            camera_ubo;      );
TG_SSBO(1,
	u32    visibility_buffer_w;
	u32    visibility_buffer_h;
	u64    visibility_buffer_data[];                 );
TG_SSBO(2, tg_svo                   svo;             );
TG_SSBO(3, tg_svo_node              nodes[];         );
TG_SSBO(4, tg_svo_leaf_node_data    leaf_node_data[];);
TG_SSBO(5, u32                      voxel_data[];    );

#include "shaders/raytracer/svo_functions.inc"



void main()
{
//    tg_svo s = svo[v_instance_id];
//    v3 extent = s.max.xyz - s.min.xyz;
//    v3 center = 0.5 * extent + s.min.xyz;
//
//    u32 giid_x = u32(gl_FragCoord.x);
//    u32 giid_y = u32(gl_FragCoord.y);
//
//    f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
//    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);
//
//    // Instead of transforming the box, the ray is transformed with its inverse
//    v3 ray_origin = camera.xyz - center;
//    v3 ray_direction = normalize(mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx));
//    v3 ray_inv_direction = v3(1.0) / ray_direction;
//    tg_ray r = tg_ray(ray_origin, ray_direction, ray_inv_direction);
//
//#if TG_SVO_TRAVERSE_STACK_CAPACITY == 5 // Otherwise, we need to adjust the constructors
//    u32 stack_size;
//    u32 parent_idx_stack[ 5] = u32[ 5](0, 0, 0, 0, 0);
//    f32 parent_min_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
//    f32 parent_max_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
//#endif
//
//    bool result = false;
//    f32 d = TG_F32_MAX;
//    u32 node_idx = TG_U32_MAX;
//    u32 voxel_idx = TG_U32_MAX;
//
//
//
//    // TODO ray trace here
//    //tg_svo_traverse
//
//
//
//    u32 voxel_id = voxel_idx;
//
//    if (result)
//    {
//        // Layout : 24 bits depth | 10 bits instance id | 30 bits relative voxel_id
//
//        u64 depth_24b = u64(d * 16777215.0) << u64(40);
//        u64 instance_id_10b = u64(v_instance_id) << u64(30);
//        u64 voxel_id_30b = u64(voxel_id);
//        u64 packed_data = depth_24b | instance_id_10b | voxel_id_30b;
//
//        u32 x = u32(gl_FragCoord.x);
//        u32 y = u32(gl_FragCoord.y);
//        u32 pixel_idx = visibility_buffer_w * y + x;
//        atomicMin(visibility_buffer_data[pixel_idx], packed_data);
//    }
}
