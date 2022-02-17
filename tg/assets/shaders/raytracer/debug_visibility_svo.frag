#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"



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



struct tg_svo
{
    v4    min;
    v4    max;
};

struct tg_svo_node
{
    // Inner node: u16 child_pointer; u8 bit valid_mask; u8 bit leaf_mask;
    // Leaf node:  u32 data_pointer;
    u32    data;
};

struct tg_svo_leaf_node_data
{
    u16    instance_count;
    u16    p_instance_ids[8];
};



TG_IN_FLAT(0, u32 v_instance_id);



TG_UBO(1,
    v4     camera;
    v4     ray00;
    v4     ray10;
    v4     ray01;
    v4     ray11;
    f32    near;
    f32    far;
);

TG_SSBO(2,
    u32    visibility_buffer_w;
    u32    visibility_buffer_h;
    u64    visibility_buffer_data[];
);

TG_SSBO(3, tg_svo svo[];);
TG_SSBO(4, tg_svo_node nodes[];);
TG_SSBO(5, tg_svo_leaf_node_data leaf_node_data[];);
TG_SSBO(6, u32 voxel_data[];);



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
    tg_svo s = svo[v_instance_id];
    v3 extent = s.max.xyz - s.min.xyz;
    v3 center = 0.5 * extent + s.min.xyz;

    u32 giid_x = u32(gl_FragCoord.x);
    u32 giid_y = u32(gl_FragCoord.y);

    f32 fx =       gl_FragCoord.x / f32(visibility_buffer_w);
    f32 fy = 1.0 - gl_FragCoord.y / f32(visibility_buffer_h);

    // Instead of transforming the box, the ray is transformed with its inverse
    v3 ray_origin = camera.xyz - center;
    v3 ray_direction = normalize(mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx));
    v3 ray_inv_direction = vec3(1.0) / ray_direction;
    tg_ray r = tg_ray(ray_origin, ray_direction, ray_inv_direction);

    const u32 stack_capacity = (1 << 10); // Total number of nodes is (8^0 + 8^1 + ... + 8^(log2(1024)-1)), but the stack size is limited
    u32 stack_size = 0;
    
    u32 idx_stack[stack_capacity]; // Contains indices of inner nodes only. Children are processed differently
    f32 min_stack[3 * stack_capacity];
    f32 max_stack[3 * stack_capacity];

    idx_stack[stack_size] = 0;
    min_stack[3 * stack_size    ] = s.min.x;
    min_stack[3 * stack_size + 1] = s.min.y;
    min_stack[3 * stack_size + 2] = s.min.z;
    max_stack[3 * stack_size    ] = s.max.x;
    max_stack[3 * stack_size + 1] = s.max.y;
    max_stack[3 * stack_size + 2] = s.max.z;
    stack_size++;

    while (stack_size > 0)
    {
        stack_size--;
        u32 node_idx = idx_stack[stack_size];
        v3  node_min = v3(min_stack[3 * stack_size], min_stack[3 * stack_size + 1], min_stack[3 * stack_size + 2]);
        v3  node_max = v3(max_stack[3 * stack_size], max_stack[3 * stack_size + 1], max_stack[3 * stack_size + 2]);

        tg_svo_node node = nodes[node_idx];

        tg_box b = tg_box(node_min, node_max);
        f32 t;
        if (tg_intersect_ray_box(r, b, t))
        {
            v3 child_extent = (node_max - node_min) * 0.5;

            u32 child_pointer =  node.data        & ((1 << 16) - 1);
            u32 valid_mask    = (node.data >> 16) & ((1 <<  8) - 1);
            u32 leaf_mask     = (node.data >> 24) & ((1 <<  8) - 1);

            u32 child_offset = 0;
            for (u32 child_idx = 0; child_idx < 8; child_idx++)
            {
                if ((valid_mask & (1 << child_idx)) != 0)
                {
                    f32 dx = f32( child_idx      % 2) * child_extent.x;
                    f32 dy = f32((child_idx / 2) % 2) * child_extent.y;
                    f32 dz = f32((child_idx / 4) % 2) * child_extent.z;

                    u32 child_node_idx = node_idx + child_pointer + child_offset;
                    v3  child_node_min = node_min + v3(dx, dy, dz);
                    v3  child_node_max = child_node_min + child_extent;

                    if ((leaf_mask & (1 << child_idx)) != 0)
                    {
                        if (tg_intersect_ray_box(r, tg_box(child_node_min, child_node_max), t))
                            if (child_extent.x == 8 && child_extent.y == 8 && child_extent.z == 8) return;
                        //return; // TODO: raytrace voxels instead!
                    }
                    else
                    {
                        if (stack_size == stack_capacity)
                        {
                            return; // TODO: Maybe we need a global stack or something...
                        }

                        idx_stack[stack_size] = child_node_idx;
                        min_stack[3 * stack_size    ] = child_node_min.x;
                        min_stack[3 * stack_size + 1] = child_node_min.y;
                        min_stack[3 * stack_size + 2] = child_node_min.z;
                        max_stack[3 * stack_size    ] = child_node_max.x;
                        max_stack[3 * stack_size + 1] = child_node_max.y;
                        max_stack[3 * stack_size + 2] = child_node_max.z;
                        
                        stack_size++;
                    }
                    child_offset++;
                }
            }
        }
    }

    const f32 d = 0.4;
    const u32 voxel_id = 1;
    
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
