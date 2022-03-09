struct tg_cluster_idx_to_object_idx_ssbo_entry
{
    u32    object_idx;
};

struct tg_object_data_ssbo_entry
{
    v3u    n_clusters_per_dim;
    u32    first_cluster_idx;
    v3     translation;
    u32    pad; // TODO: Get rid of padding somehow?
    m4     rotation;
};

struct tg_object_color_lut_ssbo_entry
{
    u32    packed_color_entry; // TODO: 24 bits used, use remaining 8 for material properties
};

struct tg_voxel_cluster_ssbo_entry
{
    u32    voxel_cluster_data_entry;
};

struct tg_color_lut_idx_cluster_ssbo_entry
{
    u32    color_lut_idx_cluster_data_entry; // Note: One entry is 8 bit
};

struct tg_view_projection_ubo
{
    m4    v;
    m4    p;
};

struct tg_camera_ubo
{
    v4     camera;
    v4     ray00; // TODO: Rename this ridiculousness
    v4     ray10;
    v4     ray01;
    v4     ray11;
    f32    near;
    f32    far;
};

struct tg_environment_ubo
{
    v4    sun_direction;
};