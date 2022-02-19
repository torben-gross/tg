#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"



#define TG_SVO_BLOCK_VOXEL_COUNT    512



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
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool tg_intersect_ray_aabb(tg_ray r, tg_box b, out f32 enter, out f32 exit)
{
    v3 vector1 = (b.min - r.o) * r.invd;
    v3 vector2 = (b.max - r.o) * r.invd;
    v3 n = tg_min_component_wise(vector1, vector2);
    v3 f = tg_max_component_wise(vector1, vector2);
    enter = max(max(n.x, n.y), n.z);
    exit = min(min(f.x, f.y), f.z);
    return exit > 0.0 && enter <= exit;
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
    v3 ray_inv_direction = v3(1.0) / ray_direction;
    tg_ray r = tg_ray(ray_origin, ray_direction, ray_inv_direction);

    const u32 stack_capacity = 7; // Assuming a SVO extent of 1024^3, we have a max of 7 levels of inner nodes, whilst the 8th level contains the 8^3 blocks of voxels
    u32 stack_size;
    u32 parent_idx_stack[ 7] = u32[ 7](0, 0, 0, 0, 0, 0, 0);
    f32 parent_min_stack[21] = f32[21](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    f32 parent_max_stack[21] = f32[21](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    bool result = false;
    f32 d = TG_F32_MAX;
    u32 node_idx = TG_U32_MAX;
    u32 voxel_idx = TG_U32_MAX;

    f32 enter, exit;
    if (tg_intersect_ray_aabb(r, tg_box(s.min.xyz, s.max.xyz), enter, exit))
    {
        f32 total_advance_from_ray_origin = 0.0f;
        v3 position = r.o;
        if (enter > 0.0)
        {
            total_advance_from_ray_origin += enter;
            position += enter * r.d;
        }
        
        stack_size = 1;
        parent_idx_stack[0] = 0;
        parent_min_stack[0] = s.min.x;
        parent_min_stack[1] = s.min.y;
        parent_min_stack[2] = s.min.z;
        parent_max_stack[0] = s.max.x;
        parent_max_stack[1] = s.max.y;
        parent_max_stack[2] = s.max.z;

        while (stack_size > 0)
        {
            u32 parent_idx = parent_idx_stack[stack_size - 1];
            v3 parent_min = v3(
                parent_min_stack[3 * stack_size - 3],
                parent_min_stack[3 * stack_size - 2],
                parent_min_stack[3 * stack_size - 1]);
            v3 parent_max = v3(
                parent_max_stack[3 * stack_size - 3],
                parent_max_stack[3 * stack_size - 2],
                parent_max_stack[3 * stack_size - 1]);
            tg_svo_node parent_node = nodes[parent_idx];
            u32 child_pointer =  parent_node.data        & ((1 << 16) - 1);
            u32 valid_mask    = (parent_node.data >> 16) & ((1 <<  8) - 1);
            u32 leaf_mask     = (parent_node.data >> 24) & ((1 <<  8) - 1);
        
            v3 child_extent = 0.5 * (parent_max - parent_min);

            u32 relative_child_idx = 0;
            v3 child_min = parent_min;
            v3 child_max = child_min + child_extent;
            
            if (child_max.x < position.x || position.x == child_max.x && r.d.x > 0.0)
            {
                relative_child_idx += 1;
                child_min.x += child_extent.x;
                child_max.x += child_extent.x;
            }
            if (child_max.y < position.y || position.y == child_max.y && r.d.y > 0.0)
            {
                relative_child_idx += 2;
                child_min.y += child_extent.y;
                child_max.y += child_extent.y;
            }
            if (child_max.z < position.z || position.z == child_max.z && r.d.z > 0.0)
            {
                relative_child_idx += 4;
                child_min.z += child_extent.z;
                child_max.z += child_extent.z;
            }

            bool advance_to_border = true;
            if ((valid_mask & (1 << relative_child_idx)) != 0)
            {
                // Note: The SVO may not have all children set, so the child may be stored at a smaller offset
                u32 relative_child_offset = 0;
                for (u32 i = 0; i < relative_child_idx; i++)
                {
                    relative_child_offset += (valid_mask >> i) & 1;
                }
                u32 child_idx = parent_idx + child_pointer + relative_child_offset;

                // IF LEAF: TRAVERSE VOXELS
                // ELSE: PUSH INNER CHILD NODE ONTO STACK

                if ((leaf_mask & (1 << relative_child_idx)) != 0)
                {
                    tg_svo_node child_node = nodes[child_idx];
                    u32 data_pointer = child_node.data;
                    tg_svo_leaf_node_data data = leaf_node_data[data_pointer];
                    if (data.instance_count != 0)
                    {
                        // UNCOMMENT TO SHOW BLOCKS
                        //result = true;
                        //d = length(position - r.o) / far;
                        //node_idx = child_idx;
                        //voxel_idx = child_idx;
                        //break;

                        // TRAVERSE BLOCK
                        
                        u32 first_voxel_id = data_pointer * TG_SVO_BLOCK_VOXEL_COUNT;

                        v3 hit = position;
                        v3 xyz = clamp(floor(hit), child_min, child_max - v3(1.0));

                        hit -= child_min;
                        xyz -= child_min;

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
                            t_max_x = (f32(x + 1) - hit.x) / r.d.x;
                            t_delta_x = 1.0 / r.d.x;
                        }
                        else
                        {
                            step_x = -1;
                            t_max_x = (hit.x - f32(x)) / -r.d.x;
                            t_delta_x = 1.0 / -r.d.x;
                        }
                        if (r.d.y > 0.0)
                        {
                            step_y = 1;
                            t_max_y = (f32(y + 1) - hit.y) / r.d.y;
                            t_delta_y = 1.0 / r.d.y;
                        }
                        else
                        {
                            step_y = -1;
                            t_max_y = (hit.y - f32(y)) / -r.d.y;
                            t_delta_y = 1.0 / -r.d.y;
                        }
                        if (r.d.z > 0.0)
                        {
                            step_z = 1;
                            t_max_z = (f32(z + 1) - hit.z) / r.d.z;
                            t_delta_z = 1.0 / r.d.z;
                        }
                        else
                        {
                            step_z = -1;
                            t_max_z = (hit.z - f32(z)) / -r.d.z;
                            t_delta_z = 1.0 / -r.d.z;
                        }

                        while (true)
                        {
                            u32 vox_id = u32(child_extent.x) * u32(child_extent.y) * z + u32(child_extent.x) * y + x;
                            u32 vox_idx = first_voxel_id + vox_id;
                            u32 bits_idx = vox_idx / 32;
                            u32 bit_idx = vox_idx % 32;
                            u32 bits = voxel_data[bits_idx];
                            u32 bit = bits & (1 << bit_idx);
                            if (bit != 0)
                            {
                                v3 voxel_min = child_min + v3(f32(x    ), f32(y    ), f32(z    ));
                                v3 voxel_max = child_min + v3(f32(x + 1), f32(y + 1), f32(z + 1));
                                tg_intersect_ray_aabb(tg_ray(position, ray_direction, ray_inv_direction), tg_box(voxel_min, voxel_max), enter, exit);
                                result = true;
                                d = total_advance_from_ray_origin / far + ((enter < 0.0) ? 0.0 : min(1.0, enter / far));
                                node_idx = child_idx;
                                voxel_idx = vox_idx;
                                break;
                            }
                            if (t_max_x < t_max_y)
                            {
                                if (t_max_x < t_max_z)
                                {
                                    t_max_x += t_delta_x;
                                    x += step_x;
                                    if (x < 0 || x >= child_extent.x) break;
                                }
                                else
                                {
                                    t_max_z += t_delta_z;
                                    z += step_z;
                                    if (z < 0 || z >= child_extent.z) break;
                                }
                            }
                            else
                            {
                                if (t_max_y < t_max_z)
                                {
                                    t_max_y += t_delta_y;
                                    y += step_y;
                                    if (y < 0 || y >= child_extent.y) break;
                                }
                                else
                                {
                                    t_max_z += t_delta_z;
                                    z += step_z;
                                    if (z < 0 || z >= child_extent.z) break;
                                }
                            }
                        }
                        if (result)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    advance_to_border = false;

                    parent_idx_stack[stack_size] = child_idx;
                    parent_min_stack[3 * stack_size    ] = child_min.x;
                    parent_min_stack[3 * stack_size + 1] = child_min.y;
                    parent_min_stack[3 * stack_size + 2] = child_min.z;
                    parent_max_stack[3 * stack_size    ] = child_max.x;
                    parent_max_stack[3 * stack_size + 1] = child_max.y;
                    parent_max_stack[3 * stack_size + 2] = child_max.z;
                    stack_size++;
                }
            }

            if (advance_to_border)
            {
                // ADVANCE POSITION TO BORDER
                
                v3 a = (child_min - position) * r.invd;
                v3 b = (child_max - position) * r.invd;
                v3 f = v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
                exit = min(min(f.x, f.y), f.z);
                total_advance_from_ray_origin += exit;
                position += (exit + TG_F32_EPSILON) * r.d; // TODO: epsilon required?

                // POP ALL NODES WITH EXIT <= 0

                while (stack_size > 0)
                {
                    v3 node_min = v3(
                        parent_min_stack[3 * stack_size - 3],
                        parent_min_stack[3 * stack_size - 2],
                        parent_min_stack[3 * stack_size - 1]);
                    v3 node_max = v3(
                        parent_max_stack[3 * stack_size - 3],
                        parent_max_stack[3 * stack_size - 2],
                        parent_max_stack[3 * stack_size - 1]);

                    a = (node_min - position) * r.invd;
                    b = (node_max - position) * r.invd;
                    f = v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
                    exit = min(min(f.x, f.y), f.z);
                    if (exit > TG_F32_EPSILON)
                    {
                        break;
                    }
                    stack_size--;
                }
            }
        }
    }
    u32 voxel_id = voxel_idx;

    if (result)
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
