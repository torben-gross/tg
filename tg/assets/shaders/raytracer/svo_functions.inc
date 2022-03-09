f32 tg_svo_traverse(tg_svo svo, v3 ray_origin_ws, v3 ray_direction_ws, out v3 out_hit_position, out v3 out_hit_normal, out u32 out_node_idx, out u32 out_voxel_idx)
{
    v3 extent = svo.max.xyz - svo.min.xyz;
    v3 center = 0.5 * extent + svo.min.xyz;

    // Instead of transforming the cuboid, the ray is transformed with its inverse
    v3 ray_origin_ms = ray_origin_ws - center;
    v3 ray_direction_ms = ray_direction_ws;

#if TG_SVO_TRAVERSE_STACK_CAPACITY == 5 // Otherwise, we need to adjust the constructors
    u32 stack_size;
    u32 parent_idx_stack[ 5] = u32[ 5](0, 0, 0, 0, 0);
    f32 parent_min_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    f32 parent_max_stack[15] = f32[15](0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
#endif

    f32 result = 1.0;
    out_node_idx = TG_U32_MAX;
    out_voxel_idx = TG_U32_MAX;

    f32 enter, exit;
    if (tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, svo.min.xyz, svo.max.xyz, enter, exit))
    {
        f32 total_advance_from_ray_origin_ms = 0.0f;
        v3 position = ray_origin_ms;
        if (enter > 0.0)
        {
            total_advance_from_ray_origin_ms += enter;
            position += enter * ray_direction_ms;
        }
        
        stack_size = 1;
        parent_idx_stack[0] = 0;
        parent_min_stack[0] = svo.min.x;
        parent_min_stack[1] = svo.min.y;
        parent_min_stack[2] = svo.min.z;
        parent_max_stack[0] = svo.max.x;
        parent_max_stack[1] = svo.max.y;
        parent_max_stack[2] = svo.max.z;

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
            
            if (child_max.x < position.x || position.x == child_max.x && ray_direction_ms.x > 0.0)
            {
                relative_child_idx += 1;
                child_min.x += child_extent.x;
                child_max.x += child_extent.x;
            }
            if (child_max.y < position.y || position.y == child_max.y && ray_direction_ms.y > 0.0)
            {
                relative_child_idx += 2;
                child_min.y += child_extent.y;
                child_max.y += child_extent.y;
            }
            if (child_max.z < position.z || position.z == child_max.z && ray_direction_ms.z > 0.0)
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
                    if (data.n != 0)
                    {
                        // UNCOMMENT TO SHOW BLOCKS
                        //result = length(position - ray_origin_ms) / camera_ubo.far;
                        //node_idx = child_idx;
                        //voxel_idx = child_idx;
                        //break;

                        // TRAVERSE BLOCK
                        
                        u32 first_voxel_idx = data_pointer * TG_SVO_BLOCK_VOXEL_COUNT;

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

                        if (ray_direction_ms.x > 0.0)
                        {
                            step_x = 1;
                            t_max_x = (f32(x + 1) - hit.x) / ray_direction_ms.x;
                            t_delta_x = 1.0 / ray_direction_ms.x;
                        }
                        else if (ray_direction_ms.x < 0.0)
                        {
                            step_x = -1;
                            t_max_x = (hit.x - f32(x)) / -ray_direction_ms.x;
                            t_delta_x = 1.0 / -ray_direction_ms.x;
                        }
                        if (ray_direction_ms.y > 0.0)
                        {
                            step_y = 1;
                            t_max_y = (f32(y + 1) - hit.y) / ray_direction_ms.y;
                            t_delta_y = 1.0 / ray_direction_ms.y;
                        }
                        else if (ray_direction_ms.y < 0.0)
                        {
                            step_y = -1;
                            t_max_y = (hit.y - f32(y)) / -ray_direction_ms.y;
                            t_delta_y = 1.0 / -ray_direction_ms.y;
                        }
                        if (ray_direction_ms.z > 0.0)
                        {
                            step_z = 1;
                            t_max_z = (f32(z + 1) - hit.z) / ray_direction_ms.z;
                            t_delta_z = 1.0 / ray_direction_ms.z;
                        }
                        else if (ray_direction_ms.z < 0.0)
                        {
                            step_z = -1;
                            t_max_z = (hit.z - f32(z)) / -ray_direction_ms.z;
                            t_delta_z = 1.0 / -ray_direction_ms.z;
                        }

                        u32 steps = 0;
                        while (true)
                        {
                            u32 relative_voxel_idx = u32(child_extent.x) * u32(child_extent.y) * z + u32(child_extent.x) * y + x;
                            u32 voxel_idx = first_voxel_idx + relative_voxel_idx;
                            u32 slot_idx = voxel_idx / 32;
                            u32 bit_idx = voxel_idx % 32;
                            u32 bits = voxel_data[slot_idx];
                            u32 bit = bits & (1 << bit_idx);
                            if (bit != 0)
                            {
                                v3 voxel_min = child_min + v3(f32(x    ), f32(y    ), f32(z    ));
                                v3 voxel_max = child_min + v3(f32(x + 1), f32(y + 1), f32(z + 1));
                                tg_intersect_ray_aabb(ray_origin_ms, ray_direction_ms, voxel_min, voxel_max, enter, exit);

                                v3 hit_position = position + enter * ray_direction_ms;
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

                                result = enter / camera_ubo.far;
                                out_hit_position = hit_position;
                                out_hit_normal = hit_normal;
								out_node_idx = child_idx;
								out_voxel_idx = relative_voxel_idx;
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
                    (ray_direction_ms.x == 0) ? TG_F32_MIN : ((child_min.x - position.x) / ray_direction_ms.x),
                    (ray_direction_ms.y == 0) ? TG_F32_MIN : ((child_min.y - position.y) / ray_direction_ms.y),
                    (ray_direction_ms.z == 0) ? TG_F32_MIN : ((child_min.z - position.z) / ray_direction_ms.z));
                v3 b = v3(
                    (ray_direction_ms.x == 0) ? TG_F32_MAX : ((child_max.x - position.x) / ray_direction_ms.x),
                    (ray_direction_ms.y == 0) ? TG_F32_MAX : ((child_max.y - position.y) / ray_direction_ms.y),
                    (ray_direction_ms.z == 0) ? TG_F32_MAX : ((child_max.z - position.z) / ray_direction_ms.z));
                v3 f = v3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
                exit = min(min(f.x, f.y), f.z);
                total_advance_from_ray_origin_ms += exit;
                position += (exit + TG_F32_EPSILON) * ray_direction_ms; // TODO: epsilon required?

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
                        (ray_direction_ms.x == 0) ? TG_F32_MIN : ((node_min.x - position.x) / ray_direction_ms.x),
                        (ray_direction_ms.y == 0) ? TG_F32_MIN : ((node_min.y - position.y) / ray_direction_ms.y),
                        (ray_direction_ms.z == 0) ? TG_F32_MIN : ((node_min.z - position.z) / ray_direction_ms.z));
                    b = v3(
                        (ray_direction_ms.x == 0) ? TG_F32_MAX : ((node_max.x - position.x) / ray_direction_ms.x),
                        (ray_direction_ms.y == 0) ? TG_F32_MAX : ((node_max.y - position.y) / ray_direction_ms.y),
                        (ray_direction_ms.z == 0) ? TG_F32_MAX : ((node_max.z - position.z) / ray_direction_ms.z));
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
    return result;
}
