// Source: https://gamedev.net/forums/topic/682750-problem-with-raybox-intersection-in-glsl/5313495/
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool tg_intersect_ray_aabb(v3 ray_origin, v3 ray_direction, v3 aabb_min, v3 aabb_max, out f32 enter, out f32 exit)
{
    const f32 vec0_x = (ray_direction.x == 0) ? TG_F32_MIN : ((aabb_min.x - ray_origin.x) / ray_direction.x);
    const f32 vec0_y = (ray_direction.y == 0) ? TG_F32_MIN : ((aabb_min.y - ray_origin.y) / ray_direction.y);
    const f32 vec0_z = (ray_direction.z == 0) ? TG_F32_MIN : ((aabb_min.z - ray_origin.z) / ray_direction.z);

    const f32 vec1_x = (ray_direction.x == 0) ? TG_F32_MAX : ((aabb_max.x - ray_origin.x) / ray_direction.x);
    const f32 vec1_y = (ray_direction.y == 0) ? TG_F32_MAX : ((aabb_max.y - ray_origin.y) / ray_direction.y);
    const f32 vec1_z = (ray_direction.z == 0) ? TG_F32_MAX : ((aabb_max.z - ray_origin.z) / ray_direction.z);

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
