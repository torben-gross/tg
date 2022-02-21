#version 450

#include "shaders/common.inc"
#include "shaders/commoni16.inc"
#include "shaders/commoni64.inc"
#include "shaders/random.inc"



#define TG_SVO_SIDE_LENGTH                1024
#define TG_SVO_BLOCK_SIDE_LENGTH          32
#define TG_SVO_BLOCK_VOXEL_COUNT          (TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH)
#define TG_SVO_TRAVERSE_STACK_CAPACITY    5



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

TG_SSBO( 9, tg_svo svo[];);
TG_SSBO(10, tg_svo_node nodes[];);
TG_SSBO(11, tg_svo_leaf_node_data leaf_node_data[];);
TG_SSBO(12, u32 voxel_data[];);

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
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool tg_intersect_ray_aabb(tg_ray r, tg_box b, out f32 enter, out f32 exit)
{
    const f32 vec0_x = (r.d.x == 0) ? TG_F32_MIN : ((b.min.x - r.o.x) * r.invd.x);
    const f32 vec0_y = (r.d.y == 0) ? TG_F32_MIN : ((b.min.y - r.o.y) * r.invd.y);
    const f32 vec0_z = (r.d.z == 0) ? TG_F32_MIN : ((b.min.z - r.o.z) * r.invd.z);

    const f32 vec1_x = (r.d.x == 0) ? TG_F32_MAX : ((b.max.x - r.o.x) * r.invd.x);
    const f32 vec1_y = (r.d.y == 0) ? TG_F32_MAX : ((b.max.y - r.o.y) * r.invd.y);
    const f32 vec1_z = (r.d.z == 0) ? TG_F32_MAX : ((b.max.z - r.o.z) * r.invd.z);

    const f32 n_x = min(vec0_x, vec1_x);
    const f32 n_y = min(vec0_y, vec1_y);
    const f32 n_z = min(vec0_z, vec1_z);

    const f32 f_x = max(vec0_x, vec1_x);
    const f32 f_y = max(vec0_y, vec1_y);
    const f32 f_z = max(vec0_z, vec1_z);

    enter = max(max(n_x, n_y), n_z);
    exit = min(min(f_x, f_y), f_z);
    return exit > 0.0 && enter <= exit;
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

f32 tg_raytrace_ssvo(v3 ray_origin_ws, v3 ray_direction_ws, out v3 out_hit_position, out v3 out_hit_normal)
{
    const u32 v_instance_id = 0; // TODO: figure this one out

    tg_svo s = svo[v_instance_id];
    v3 extent = s.max.xyz - s.min.xyz;
    v3 center = 0.5 * extent + s.min.xyz;

    // Instead of transforming the box, the ray is transformed with its inverse
    v3 ray_origin_ms = ray_origin_ws - center;
    v3 ray_direction_ms = ray_direction_ws;
    v3 ray_inv_direction_ms = v3(1.0) / ray_direction_ms;
    tg_ray r = tg_ray(ray_origin_ms, ray_direction_ms, ray_inv_direction_ms);

#if TG_SVO_TRAVERSE_STACK_CAPACITY == 5 // Otherwise, we need to adjust the constructors
    u32 stack_size;
    u32 parent_idx_stack[ 5] = u32[ 5](0, 0, 0, 0, 0);
    f32 parent_min_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    f32 parent_max_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
#endif

    f32 result = 1.0;
    f32 d = TG_F32_MAX;
    u32 node_idx = TG_U32_MAX;
    u32 voxel_idx = TG_U32_MAX;

    f32 enter, exit;
    if (tg_intersect_ray_aabb(r, tg_box(s.min.xyz, s.max.xyz), enter, exit))
    {
        f32 total_advance_from_ray_origin_ms = 0.0f;
        v3 position = r.o;
        if (enter > 0.0)
        {
            total_advance_from_ray_origin_ms += enter;
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
                            t_max_x = (f32(x + 1) - hit.x) / r.d.x;
                            t_delta_x = 1.0 / r.d.x;
                        }
                        else if (r.d.x < 0.0)
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
                        else if (r.d.y < 0.0)
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
                        else if (r.d.z < 0.0)
                        {
                            step_z = -1;
                            t_max_z = (hit.z - f32(z)) / -r.d.z;
                            t_delta_z = 1.0 / -r.d.z;
                        }

                        u32 steps = 0;
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
                                tg_intersect_ray_aabb(tg_ray(position, r.d, r.invd), tg_box(voxel_min, voxel_max), enter, exit);

                                v3 hit_position = position + enter * r.d;
                                v3 voxel_center = voxel_min + v3(0.5, 0.5, 0.5);
                                v3 hit_normal = hit_position - voxel_center;
                                if (abs(hit_normal.x) > abs(hit_normal.y))
                                {
                                    hit_normal.y = 0.0f;
                                    if (abs(hit_normal.x) > abs(hit_normal.z))
                                    {
                                        hit_normal.x = sign(hit_normal.x);
                                        hit_normal.z = 0.0f;
                                    }
                                    else
                                    {
                                        hit_normal.z = sign(hit_normal.z);
                                        hit_normal.x = 0.0f;
                                    }
                                }
                                else
                                {
                                    hit_normal.x = 0.0f;
                                    if (abs(hit_normal.y) > abs(hit_normal.z))
                                    {
                                        hit_normal.y = sign(hit_normal.y);
                                        hit_normal.z = 0.0f;
                                    }
                                    else
                                    {
                                        hit_normal.z = sign(hit_normal.z);
                                        hit_normal.y = 0.0f;
                                    }
                                }

                                result = 0.0;
                                out_hit_position = hit_position;
                                out_hit_normal = hit_normal;

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
                        if (result < 1.0)
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
                
                v3 a = v3(
                    (r.d.x == 0) ? TG_F32_MIN : ((child_min.x - position.x) * r.invd.x),
                    (r.d.y == 0) ? TG_F32_MIN : ((child_min.y - position.y) * r.invd.y),
                    (r.d.z == 0) ? TG_F32_MIN : ((child_min.z - position.z) * r.invd.z));
                v3 b = v3(
                    (r.d.x == 0) ? TG_F32_MAX : ((child_max.x - position.x) * r.invd.x),
                    (r.d.y == 0) ? TG_F32_MAX : ((child_max.y - position.y) * r.invd.y),
                    (r.d.z == 0) ? TG_F32_MAX : ((child_max.z - position.z) * r.invd.z));
                v3 f = v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
                exit = min(min(f.x, f.y), f.z);
                total_advance_from_ray_origin_ms += exit;
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
                    
                    a = v3(
                        (r.d.x == 0) ? TG_F32_MIN : ((node_min.x - position.x) * r.invd.x),
                        (r.d.y == 0) ? TG_F32_MIN : ((node_min.y - position.y) * r.invd.y),
                        (r.d.z == 0) ? TG_F32_MIN : ((node_min.z - position.z) * r.invd.z));
                    b = v3(
                        (r.d.x == 0) ? TG_F32_MAX : ((node_max.x - position.x) * r.invd.x),
                        (r.d.y == 0) ? TG_F32_MAX : ((node_max.y - position.y) * r.invd.y),
                        (r.d.z == 0) ? TG_F32_MAX : ((node_max.z - position.z) * r.invd.z));
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
    return result;
}

void main()
{
    u32 v_buf_idx = visibility_buffer_w * u32(gl_FragCoord.y) + u32(gl_FragCoord.x);
    u64 packed_data = visibility_buffer_data[v_buf_idx];
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

        m4 tr_mat = i.t_mat * i.r_mat;

        // We transform ray to model space:
        m4 ray_origin_ws2ms_mat = inverse(tr_mat);
        v3 ray_origin_ws = camera.xyz;
        v3 ray_origin_ms = (ray_origin_ws2ms_mat * v4(ray_origin_ws, 1.0)).xyz;

        m4 ray_direction_ws2ms_mat = inverse(i.r_mat);
        v3 ray_direction_ws = normalize(mix(mix(ray00.xyz, ray10.xyz, fy), mix(ray01.xyz, ray11.xyz, fy), fx));
        v3 ray_direction_ms = (ray_direction_ws2ms_mat * v4(ray_direction_ws, 0.0)).xyz;
        
        f32 ray_t = depth_24b * far; // TODO: far plane
        v3 hit_position_ws = ray_origin_ws + ray_t * ray_direction_ws;
        v3 hit_position_ms = ray_origin_ms + ray_t * ray_direction_ms;
        


        // Voxel center position
        
        u32 voxel_index_x =  voxel_id_30b                        % i.grid_w;
        u32 voxel_index_y = (voxel_id_30b / i.grid_w)            % i.grid_h;
        u32 voxel_index_z = (voxel_id_30b / i.grid_w / i.grid_h) % i.grid_d;

        // We are in model space. Start in center of grid, then to min corner of grid, then to center of first voxel, then to center of actual voxel
        f32 voxel_center_ms_x = -f32(i.grid_w / 2) + 0.5 + f32(voxel_index_x);
        f32 voxel_center_ms_y = -f32(i.grid_h / 2) + 0.5 + f32(voxel_index_y);
        f32 voxel_center_ms_z = -f32(i.grid_d / 2) + 0.5 + f32(voxel_index_z);
        v3 voxel_center_ms = v3(voxel_center_ms_x, voxel_center_ms_y, voxel_center_ms_z);



        // Hit normal

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
        v3 normal_ws = normalize(i.r_mat * v4(normal_ms, 0.0)).xyz;
        
        
        
        
        // TODO: hacked lighting
        v3 to_light_dir_ws = normalize(v3(-0.4, 1.0, 0.1));
        
        f32 radiance = max(0.0f, dot(to_light_dir_ws, normal_ws));
        f32 min_advance = 1.41421356237; // TODO: We may want to compute the min/max of the rotated voxel, and determine, how far we have to go, such that it does not touch that resulting cell of the SVO grid
        
        tg_rand_xorshift32 rand;
        tgm_rand_xorshift32_init(v_buf_idx + 1, rand);
        
        const u32 n_rays = 2;
        const u32 n_bounces = 2;
        const u32 max_n_bounces = n_rays * n_bounces;
        
        f32 shadow_sum = 0.0;
        for (u32 ray_idx = 0; ray_idx < n_rays; ray_idx++)
        {
            v3 new_position = hit_position_ws;
            v3 new_normal = normal_ws;
            v3 new_dir = v3(0.0, 0.0, 0.0);
        
            for (u32 bounce_idx = 0; bounce_idx < n_bounces; bounce_idx++)
            {
                for (u32 compute_new_ray_dir_idx = 0; compute_new_ray_dir_idx < 32; compute_new_ray_dir_idx++)
                {
                    new_dir.x = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
                    new_dir.y = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
                    new_dir.z = tgm_rand_xorshift32_next_f32_inclusive_range(rand, -1.0, 1.0);
                    
                    new_dir = normalize(new_dir);
                    if (dot(new_dir, new_normal) > 0.0)
                    {
                        break;
                    }
                }
                if (dot(new_dir, new_normal) <= 0.0)
                {
                    new_dir = new_normal;
                }
        
                v3 hit_position;
                v3 hit_normal;
                if (tg_raytrace_ssvo(new_position + min_advance * new_dir, new_dir, hit_position, hit_normal) == 1.0)
                {
                    f32 contrib = 1.0f - (f32(bounce_idx) / f32(n_bounces));
                    shadow_sum += contrib * dot(new_dir, v3(0.0, 1.0, 0.0)) * 0.25 + 0.75;
                    break;
                }
        
                new_position = hit_position;
                new_normal = hit_normal;
            }
        }
        
        radiance *= shadow_sum * 0.4;
        radiance = radiance * 0.9 + 0.1;
        out_color = v4(v3(radiance), 1.0);
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
