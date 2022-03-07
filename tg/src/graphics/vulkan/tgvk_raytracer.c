#include "graphics/vulkan/tgvk_raytracer.h"

#ifdef TG_VULKAN

#include "graphics/tg_sparse_voxel_octree.h"
#include "graphics/vulkan/tgvk_shader_library.h"
#include "physics/tg_physics.h"
#include "tg_input.h"
#include "tg_variadic.h"
#include "util/tg_color.h"
#include "util/tg_qsort.h"
#include "util/tg_string.h"



#define TGVK_HDR_FORMAT                               VK_FORMAT_R32G32B32A32_SFLOAT
#define TGVK_VOXEL_CLUSTER_SIZE                       TG_CLUSTER_SIZE(1) // We use 1 bit, so 1/8 byte per voxel
#define TGVK_COLOR_LUT_IDX_CLUSTER_SIZE               TG_CLUSTER_SIZE(8) // We use 8 bit, so 8   byte per color lut index
#define TGVK_MAX_N_CLUSTERS                            (1 << 21)         // This results in a max of 1,073,741,824 voxels
#define TGVK_MAX_VOXEL_CLUSTER_BUFFER_SIZE            (TGVK_MAX_N_CLUSTERS * TGVK_VOXEL_CLUSTER_SIZE)
#define TGVK_MAX_COLOR_LUT_IDX_CLUSTER_BUFFER_SIZE    (TGVK_MAX_N_CLUSTERS * TGVK_COLOR_LUT_IDX_CLUSTER_SIZE)



typedef struct tg_cluster_idx_to_object_idx_ssbo
{
    u32    object_idx;
} tg_cluster_idx_to_object_idx_ssbo;

typedef struct tg_object_data_ssbo
{
    v3u    n_clusters_per_dim;
    u32    first_cluster_idx;
    v3     translation;
    u32    pad; // TODO: Get rid of padding somehow?
    m4     rotation;
} tg_object_data_ssbo;

typedef struct tg_object_color_lut_ssbo
{
    u32    packed_color; // TODO: 24 bits used, use remaining 8 for material properties
} tg_object_color_lut_ssbo;

typedef struct tg_voxel_cluster_ssbo
{
    u32    p_voxel_cluster_data[];
} tg_voxel_cluster_ssbo;

typedef struct tg_color_lut_idx_cluster_ssbo
{
    u8    p_color_lut_idx_cluster_data[];
} tg_color_lut_idx_cluster_ssbo;

typedef struct tg_view_projection_ubo
{
    m4    v;
    m4    p;
} tg_view_projection_ubo;

typedef struct tg_camera_ubo
{
    v4     camera;
    v4     ray00; // TODO: Rename this ridiculousness
    v4     ray10;
    v4     ray01;
    v4     ray11;
    f32    near;
    f32    far;
} tg_camera_ubo;

typedef struct tg_visibility_buffer_ssbo
{
    u32    w;
    u32    h;
    u64    p_data[];
} tg_visibility_buffer_ssbo;

typedef struct tg_environment_ubo
{
    v4    sun_direction;
    //u32    directional_light_count;
    //u32    point_light_count; u32 pad[2];
    //m4     ivp; // inverse view projection
    //v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    //v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_environment_ubo;

typedef struct tggui_transform_ssbo
{
    f32    translation_x;
    f32    translation_y;
    f32    half_scale_x;
    f32    half_scale_y;
    f32    uv_center_x;
    f32    uv_center_y;
    f32    uv_half_scale_x;
    f32    uv_half_scale_y;
    u32    type;
} tggui_transform_ssbo;



static v2 tg__screen_space_to_clip_space(v2 v, v2 target_size)
{
    v2 result = { 0 };
    result.x = (v.x / target_size.x) * 2.0f - 1.0f;
    result.y = (v.y / target_size.y) * 2.0f - 1.0f;
    return result;
}

static void tg__push_debug_svo_node(tg_raytracer* p_raytracer, const tg_svo_inner_node* p_inner, v3 min, v3 max, b32 blocks_only, b32 push_children)
{
    const v3 extent = tgm_v3_sub(max, min);
    const v3 half_extent = tgm_v3_divf(extent, 2.0f);

    if (!blocks_only || !push_children)
    {
        const v3 center = tgm_v3_add(min, half_extent);
        const m4 scale = tgm_m4_scale(extent);
        const m4 rotation = tgm_m4_identity();
        const m4 translation = tgm_m4_translate(center);
        const m4 transformation_matrix = tgm_m4_transform_srt(scale, rotation, translation);
        const v3 color = push_children ? (v3) { 0.0f, 1.0f, 1.0f } : (v3) { 1.0f, 0.0f, 0.0f };
        tg_raytracer_push_debug_cuboid(p_raytracer, transformation_matrix, color);
    }

    if (push_children)
    {
        u32 child_offset = 0;

        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            if (p_inner->valid_mask & (1 << child_idx))
            {
                const f32 dx = (f32)( child_idx      % 2) * half_extent.x;
                const f32 dy = (f32)((child_idx / 2) % 2) * half_extent.y;
                const f32 dz = (f32)((child_idx / 4) % 2) * half_extent.z;

                const tg_svo_node* p_child = (tg_svo_node*)p_inner + p_inner->child_pointer + child_offset;
                const v3 child_min = { min.x + dx, min.y + dy, min.z + dz };
                const v3 child_max = tgm_v3_add(child_min, half_extent);

                tg__push_debug_svo_node(p_raytracer, &p_child->inner, child_min, child_max, blocks_only, (p_inner->leaf_mask & (1 << child_idx)) == 0);

                child_offset++;
            }
        }
    }
}



static void tg__init_buffers(tg_raytracer* p_raytracer)
{
    const u16 p_cube_indices[6 * 6] = {
         0,  1,  2,  2,  3,  0, // x-
         4,  5,  6,  6,  7,  4, // x+
         8,  9, 10, 10, 11,  8, // y-
        12, 13, 14, 14, 15, 12, // y+
        16, 17, 18, 18, 19, 16, // z-
        20, 21, 22, 22, 23, 20  // z+
    };

    const v3 p_cube_positions[6 * 4] = {
        { -0.5f, -0.5f, -0.5f }, // x-
        { -0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f }, // x+
        {  0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        { -0.5f, -0.5f, -0.5f }, // y-
        {  0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f,  0.5f },
        { -0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f, -0.5f }, // y+
        { -0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f, -0.5f },
        { -0.5f, -0.5f, -0.5f }, // z-
        { -0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f }, // z+
        {  0.5f, -0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f }
    };

    const v2 p_quad_positions[6] = {
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f }
    };

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    // ALLOCATE
    
    const tg_size idx_vbo_size                        = (tg_size)p_raytracer->scene.cluster_capacity * sizeof(u32);

    const tg_size cluster_idx_to_object_idx_ssbo_size = (tg_size)p_raytracer->scene.cluster_capacity * sizeof(u32);
    const tg_size object_data_ssbo_size               = (tg_size)p_raytracer->scene.object_capacity  * sizeof(tg_object_data_ssbo);
    const tg_size object_color_lut_ssbo_size          = (tg_size)1  * sizeof(tg_object_color_lut_ssbo); // TODO: more color LUTs
    const tg_size voxel_cluster_ssbo_size             = (tg_size)p_raytracer->scene.cluster_capacity * (tg_size)TGVK_VOXEL_CLUSTER_SIZE;
    const tg_size color_lut_idx_cluster_ssbo_size     = (tg_size)p_raytracer->scene.cluster_capacity * (tg_size)TGVK_COLOR_LUT_IDX_CLUSTER_SIZE;

    const tg_size visibility_buffer_ssbo_size         = sizeof(tg_visibility_buffer_ssbo) + ((VkDeviceSize)w * (VkDeviceSize)h * TG_SIZEOF_MEMBER(tg_visibility_buffer_ssbo, p_data[0]));

    const VkBufferUsageFlags ssbo_usage_flags         = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    p_raytracer->data.idx_vbo                        = TGVK_BUFFER_CREATE_VBO(idx_vbo_size);
    p_raytracer->data.cube_ibo                       = TGVK_BUFFER_CREATE_IBO(sizeof(p_cube_indices));
    p_raytracer->data.cube_vbo                       = TGVK_BUFFER_CREATE_VBO(sizeof(p_cube_positions));
    p_raytracer->data.quad_vbo                       = TGVK_BUFFER_CREATE_VBO(sizeof(p_quad_positions));
    
    p_raytracer->data.cluster_idx_to_object_idx_ssbo = TGVK_BUFFER_CREATE(cluster_idx_to_object_idx_ssbo_size, ssbo_usage_flags, TGVK_MEMORY_DEVICE);
    p_raytracer->data.object_data_ssbo               = TGVK_BUFFER_CREATE(object_data_ssbo_size,               ssbo_usage_flags, TGVK_MEMORY_DEVICE);
    p_raytracer->data.object_color_lut_ssbo          = TGVK_BUFFER_CREATE(object_color_lut_ssbo_size,          ssbo_usage_flags, TGVK_MEMORY_DEVICE);
    p_raytracer->data.voxel_cluster_ssbo             = TGVK_BUFFER_CREATE(voxel_cluster_ssbo_size,             ssbo_usage_flags, TGVK_MEMORY_DEVICE);
    p_raytracer->data.color_lut_idx_cluster_ssbo     = TGVK_BUFFER_CREATE(color_lut_idx_cluster_ssbo_size,     ssbo_usage_flags, TGVK_MEMORY_DEVICE); // [cluster idx + voxel idx] Cluster of color LUT indices (8 bit per voxel)

    // They are generated on the fly as required
    p_raytracer->data.svo_ssbo                = (tgvk_buffer){ 0 };
    p_raytracer->data.svo_nodes_ssbo          = (tgvk_buffer){ 0 };
    p_raytracer->data.svo_leaf_node_data_ssbo = (tgvk_buffer){ 0 };
    p_raytracer->data.svo_voxel_data_ssbo     = (tgvk_buffer){ 0 };

    p_raytracer->data.visibility_buffer_ssbo  = TGVK_BUFFER_CREATE(visibility_buffer_ssbo_size, ssbo_usage_flags, TGVK_MEMORY_DEVICE);
    p_raytracer->data.view_projection_ubo     = TGVK_BUFFER_CREATE_UBO(sizeof(tg_view_projection_ubo));
    p_raytracer->data.camera_ubo              = TGVK_BUFFER_CREATE_UBO(sizeof(tg_camera_ubo));
    p_raytracer->data.environment_ubo         = TGVK_BUFFER_CREATE_UBO(sizeof(tg_environment_ubo));

    p_raytracer->data.debug_matrices_ssbo     = TGVK_BUFFER_CREATE(p_raytracer->debug_pass.capacity * sizeof(m4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_HOST);
    p_raytracer->data.debug_colors_ssbo       = TGVK_BUFFER_CREATE(p_raytracer->debug_pass.capacity * sizeof(v4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_HOST);

    // INITIALIZE

    tgvk_staging_buffer staging_buffer = { 0 };
    tgvk_staging_buffer_take(idx_vbo_size, &p_raytracer->data.idx_vbo, &staging_buffer);
    for (u32 i = 0; i < p_raytracer->scene.cluster_capacity; i++)
    {
        tgvk_staging_buffer_push_u32(&staging_buffer, i);
    }
    
    tgvk_staging_buffer_reinit(&staging_buffer, sizeof(p_cube_indices), &p_raytracer->data.cube_ibo);
    tgvk_staging_buffer_push(&staging_buffer, sizeof(p_cube_indices), p_cube_indices);
    
    tgvk_staging_buffer_reinit(&staging_buffer, sizeof(p_cube_positions), &p_raytracer->data.cube_vbo);
    tgvk_staging_buffer_push(&staging_buffer, sizeof(p_cube_positions), p_cube_positions);
    
    tgvk_staging_buffer_reinit(&staging_buffer, sizeof(p_quad_positions), &p_raytracer->data.quad_vbo);
    tgvk_staging_buffer_push(&staging_buffer, sizeof(p_quad_positions), p_quad_positions);
    
    tgvk_staging_buffer_reinit(&staging_buffer, 2ui64 * sizeof(u32), &p_raytracer->data.visibility_buffer_ssbo);
    tgvk_staging_buffer_push_u32(&staging_buffer, w);
    tgvk_staging_buffer_push_u32(&staging_buffer, h);

    tgvk_staging_buffer_release(&staging_buffer);
}

static void tg__init_visibility_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->visibility_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    p_raytracer->visibility_pass.render_pass = tgvk_render_pass_create(TG_NULL, &subpass_description);

    // TODO shared?
    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/visibility.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/visibility.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_FRONT_BIT;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = p_raytracer->visibility_pass.render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->visibility_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->visibility_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->visibility_pass.graphics_pipeline);
    p_raytracer->visibility_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->visibility_pass.render_pass, 0, TG_NULL, w, h);
}

static void tg__init_svo_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->svo_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    p_raytracer->svo_pass.render_pass = tgvk_render_pass_create(TG_NULL, &subpass_description);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/debug_visibility_svo.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/debug_visibility_svo.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = p_raytracer->svo_pass.render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->svo_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->svo_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->svo_pass.graphics_pipeline);

    p_raytracer->svo_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->svo_pass.render_pass, 0, TG_NULL, w, h);
}

static void tg__init_shading_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->shading_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.format = TGVK_HDR_FORMAT;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;

    p_raytracer->shading_pass.render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/screen_quad.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/shading.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = p_raytracer->shading_pass.render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->shading_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->shading_pass.graphics_pipeline);
    
    VkDescriptorSet set = p_raytracer->shading_pass.descriptor_set.set;
    tg_raytracer_data* p_buffers = &p_raytracer->data;
    const u32 atmosphere_binding_offset = 4;
    u32 binding_offset = atmosphere_binding_offset;

    //tgvk_atmosphere_model_update_descriptor_set(&p_raytracer->model, &p_raytracer->shading_pass.descriptor_set);
    tgvk_descriptor_set_update_storage_buffer(set, &p_buffers->cluster_idx_to_object_idx_ssbo, binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(set, &p_buffers->object_data_ssbo,               binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(set, &p_buffers->object_color_lut_ssbo,          binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(set, &p_buffers->color_lut_idx_cluster_ssbo,     binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(set, &p_buffers->visibility_buffer_ssbo,         binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(set, &p_buffers->camera_ubo,                     binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(set, &p_buffers->environment_ubo,                binding_offset++);

    p_raytracer->shading_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->shading_pass.render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);
}

static void tg__init_debug_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->debug_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.format = TGVK_HDR_FORMAT;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;

    p_raytracer->debug_pass.render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/debug_cuboid.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/debug_cuboid.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = p_raytracer->debug_pass.render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_LINE;

    p_raytracer->debug_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->debug_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->debug_pass.graphics_pipeline);
    p_raytracer->debug_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->debug_pass.render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);

    tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.debug_matrices_ssbo, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.view_projection_ubo, 1);
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.debug_colors_ssbo, 2);
}

static void tg__init_gui_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->gui_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    p_raytracer->gui_pass.fence          = tgvk_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.format         = TGVK_HDR_FORMAT;
    attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments    = &attachment_reference;

    // TODO: simplify
    p_raytracer->gui_pass.render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);

    tg_blend_mode blend_mode = TG_BLEND_MODE_BLEND;
    tgvk_graphics_pipeline_create_info pipeline_create_info = { 0 };
    pipeline_create_info.p_vertex_shader    = tgvk_shader_library_get("shaders/gui.vert");
    pipeline_create_info.p_fragment_shader  = tgvk_shader_library_get("shaders/gui.frag");
    pipeline_create_info.cull_mode          = VK_CULL_MODE_NONE;
    pipeline_create_info.depth_test_enable  = TG_FALSE;
    pipeline_create_info.depth_write_enable = TG_FALSE;
    pipeline_create_info.p_blend_modes      = &blend_mode;
    pipeline_create_info.render_pass        = p_raytracer->gui_pass.render_pass;
    pipeline_create_info.viewport_size.x    = (f32)w;
    pipeline_create_info.viewport_size.y    = (f32)h;
    pipeline_create_info.polygon_mode       = VK_POLYGON_MODE_FILL;

    p_raytracer->gui_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&pipeline_create_info);
    p_raytracer->gui_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->gui_pass.graphics_pipeline);
    p_raytracer->gui_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->gui_pass.render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);
}

static void tg__init_blit_pass(tg_raytracer* p_raytracer)
{
    p_raytracer->blit_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_command_buffer_begin(&p_raytracer->blit_pass.command_buffer, 0);
    {
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.color_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.depth_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);

        VkImageBlit color_image_blit = { 0 };
        color_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.srcSubresource.mipLevel = 0;
        color_image_blit.srcSubresource.baseArrayLayer = 0;
        color_image_blit.srcSubresource.layerCount = 1;
        color_image_blit.srcOffsets[0].x = 0;
        color_image_blit.srcOffsets[0].y = 0;
        color_image_blit.srcOffsets[0].z = 0;
        color_image_blit.srcOffsets[1].x = p_raytracer->render_target.color_attachment.width;
        color_image_blit.srcOffsets[1].y = p_raytracer->render_target.color_attachment.height;
        color_image_blit.srcOffsets[1].z = 1;
        color_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.dstSubresource.mipLevel = 0;
        color_image_blit.dstSubresource.baseArrayLayer = 0;
        color_image_blit.dstSubresource.layerCount = 1;
        color_image_blit.dstOffsets[0].x = 0;
        color_image_blit.dstOffsets[0].y = 0;
        color_image_blit.dstOffsets[0].z = 0;
        color_image_blit.dstOffsets[1].x = p_raytracer->render_target.color_attachment.width;
        color_image_blit.dstOffsets[1].y = p_raytracer->render_target.color_attachment.height;
        color_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.color_attachment, &p_raytracer->render_target.color_attachment_copy, &color_image_blit);

        VkImageBlit depth_image_blit = { 0 };
        depth_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.srcSubresource.mipLevel = 0;
        depth_image_blit.srcSubresource.baseArrayLayer = 0;
        depth_image_blit.srcSubresource.layerCount = 1;
        depth_image_blit.srcOffsets[0].x = 0;
        depth_image_blit.srcOffsets[0].y = 0;
        depth_image_blit.srcOffsets[0].z = 0;
        depth_image_blit.srcOffsets[1].x = p_raytracer->render_target.depth_attachment.width;
        depth_image_blit.srcOffsets[1].y = p_raytracer->render_target.depth_attachment.height;
        depth_image_blit.srcOffsets[1].z = 1;
        depth_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.dstSubresource.mipLevel = 0;
        depth_image_blit.dstSubresource.baseArrayLayer = 0;
        depth_image_blit.dstSubresource.layerCount = 1;
        depth_image_blit.dstOffsets[0].x = 0;
        depth_image_blit.dstOffsets[0].y = 0;
        depth_image_blit.dstOffsets[0].z = 0;
        depth_image_blit.dstOffsets[1].x = p_raytracer->render_target.depth_attachment.width;
        depth_image_blit.dstOffsets[1].y = p_raytracer->render_target.depth_attachment.height;
        depth_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.depth_attachment, &p_raytracer->render_target.depth_attachment_copy, &depth_image_blit);

        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.color_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&p_raytracer->blit_pass.command_buffer, &p_raytracer->render_target.depth_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->blit_pass.command_buffer.buffer));
}

static void tg__init_present_pass(tg_raytracer* p_raytracer)
{
    tgvk_command_buffers_create(TGVK_COMMAND_POOL_TYPE_PRESENT, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_MAX_SWAPCHAIN_IMAGES, p_raytracer->present_pass.p_command_buffers);
    p_raytracer->present_pass.image_acquired_semaphore = tgvk_semaphore_create();

    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.format = surface.format.format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;

    p_raytracer->present_pass.render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        p_raytracer->present_pass.p_framebuffers[i] = tgvk_framebuffer_create(p_raytracer->present_pass.render_pass, 1, &p_swapchain_image_views[i], w, h);
    }

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/screen_quad.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/present.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = p_raytracer->present_pass.render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->present_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->present_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->present_pass.graphics_pipeline);

    tgvk_descriptor_set_update_image(p_raytracer->present_pass.descriptor_set.set, &p_raytracer->render_target.color_attachment, 0);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        tgvk_command_buffer_begin(&p_raytracer->present_pass.p_command_buffers[i], 0);
        {
            tgvk_cmd_transition_image_layout(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_F);

            vkCmdBindPipeline(p_raytracer->present_pass.p_command_buffers[i].buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->present_pass.graphics_pipeline.pipeline);
            vkCmdBindIndexBuffer(p_raytracer->present_pass.p_command_buffers[i].buffer, screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
            const VkDeviceSize vertex_buffer_offset = 0;
            vkCmdBindVertexBuffers(p_raytracer->present_pass.p_command_buffers[i].buffer, 0, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(p_raytracer->present_pass.p_command_buffers[i].buffer, 1, 1, &screen_quad_uvs_vbo.buffer, &vertex_buffer_offset);
            vkCmdBindDescriptorSets(p_raytracer->present_pass.p_command_buffers[i].buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->present_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->present_pass.descriptor_set.set, 0, TG_NULL);

            tgvk_cmd_begin_render_pass(&p_raytracer->present_pass.p_command_buffers[i], p_raytracer->present_pass.render_pass, &p_raytracer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            tgvk_cmd_draw_indexed(&p_raytracer->present_pass.p_command_buffers[i], 6);
            vkCmdEndRenderPass(p_raytracer->present_pass.p_command_buffers[i].buffer);

            tgvk_cmd_transition_image_layout(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.color_attachment, TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_TRANSFER_WRITE);
            tgvk_cmd_transition_image_layout(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_WRITE);

            tgvk_cmd_clear_image(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.color_attachment);
            tgvk_cmd_clear_image(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.depth_attachment);

            tgvk_cmd_transition_image_layout(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
            tgvk_cmd_transition_image_layout(&p_raytracer->present_pass.p_command_buffers[i], &p_raytracer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        }
        TGVK_CALL(vkEndCommandBuffer(p_raytracer->present_pass.p_command_buffers[i].buffer));
    }
}

static void tg__init_clear_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->clear_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    p_raytracer->clear_pass.compute_pipeline = tgvk_pipeline_create_compute(tgvk_shader_library_get("shaders/raytracer/clear.comp"));
    p_raytracer->clear_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->clear_pass.compute_pipeline);
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->clear_pass.descriptor_set.set, &p_raytracer->data.visibility_buffer_ssbo, 0);

    tgvk_command_buffer_begin(&p_raytracer->clear_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->clear_pass.descriptor_set.set, 0, TG_NULL);
        vkCmdDispatch(p_raytracer->clear_pass.command_buffer.buffer, (w + 31) / 32, (h + 31) / 32, 1);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->clear_pass.command_buffer.buffer));
}



void tg_raytracer_create(const tg_camera* p_camera, u32 max_n_instances, u32 max_n_clusters, TG_OUT tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_camera != TG_NULL);
	TG_ASSERT(max_n_instances > 0);
	TG_ASSERT(max_n_instances <= TGVK_MAX_N_CLUSTERS);
	TG_ASSERT(max_n_clusters > 0);
	TG_ASSERT(max_n_clusters <= TGVK_MAX_N_CLUSTERS);
	TG_ASSERT(p_raytracer != TG_NULL);

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->p_camera = p_camera;
    p_raytracer->render_target = TGVK_RENDER_TARGET_CREATE(
        w, h, TGVK_HDR_FORMAT, TG_NULL,
        w, h, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );

    p_raytracer->semaphore = tgvk_semaphore_create();



    p_raytracer->scene.object_capacity             = max_n_instances;
    p_raytracer->scene.n_objects                   = 0;
    p_raytracer->scene.p_objects                   = TG_MALLOC((tg_size)max_n_instances * sizeof(*p_raytracer->scene.p_objects));
    p_raytracer->scene.cluster_capacity            = max_n_clusters;
    p_raytracer->scene.n_clusters                  = 0;
    p_raytracer->scene.p_voxel_cluster_data        = TG_MALLOC((tg_size)max_n_clusters * (tg_size)TGVK_VOXEL_CLUSTER_SIZE);
    p_raytracer->scene.p_cluster_idx_to_object_idx = TG_MALLOC((tg_size)max_n_clusters * sizeof(*p_raytracer->scene.p_cluster_idx_to_object_idx));
    


    p_raytracer->gui_context = (tggui_context){ 0 };

    tggui_style* p_style = &p_raytracer->gui_context.style;
    tgvk_font_create("fonts/arial.ttf", &p_style->font);
    p_style->p_colors[TGGUI_COLOR_TEXT]           = (v4){ 1.0f,  1.0f,  1.0f,  1.0f  };
    p_style->p_colors[TGGUI_COLOR_WINDOW_BG]      = (v4){ 0.06f, 0.06f, 0.06f, 0.94f };
    p_style->p_colors[TGGUI_COLOR_BUTTON]         = (v4){ 0.26f, 0.59f, 0.98f, 0.04f };
    p_style->p_colors[TGGUI_COLOR_BUTTON_HOVERED] = (v4){ 0.26f, 0.59f, 0.98f, 1.0f  };
    p_style->p_colors[TGGUI_COLOR_BUTTON_ACTIVE]  = (v4){ 0.06f, 0.53f, 0.98f, 1.0f  };
    p_style->p_colors[TGGUI_COLOR_FRAME]          = (v4){ 0.16f, 0.29f, 0.48f, 0.54f };
    p_style->p_colors[TGGUI_COLOR_FRAME_HOVERED]  = (v4){ 0.26f, 0.59f, 0.98f, 0.4f  };
    p_style->p_colors[TGGUI_COLOR_FRAME_ACTIVE]   = (v4){ 0.26f, 0.59f, 0.98f, 0.67f };
    p_style->p_colors[TGGUI_COLOR_CHECKMARK]      = (v4){ 0.26f, 0.59f, 0.98f, 1.0f  };

    p_raytracer->gui_context.quad_capacity = 512;
    p_raytracer->gui_context.n_quads = 0;

    p_raytracer->gui_context.transform_ssbo = TGVK_BUFFER_CREATE(p_raytracer->gui_context.quad_capacity * sizeof(tggui_transform_ssbo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_HOST);
    p_raytracer->gui_context.colors_ssbo = TGVK_BUFFER_CREATE(p_raytracer->gui_context.quad_capacity * sizeof(tggui_transform_ssbo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    tgvk_sampler_create_info white_texture_sampler_create_info = { 0 };
    white_texture_sampler_create_info.min_filter = TG_IMAGE_FILTER_NEAREST;
    white_texture_sampler_create_info.mag_filter = TG_IMAGE_FILTER_NEAREST;
    white_texture_sampler_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_REPEAT;
    white_texture_sampler_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_REPEAT;
    white_texture_sampler_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_REPEAT;

    p_raytracer->gui_context.white_texture = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR, 1, 1, TG_COLOR_IMAGE_FORMAT_R8_UNORM, &white_texture_sampler_create_info);
    const u8 white = 0xff;
    tgvk_util_copy_to_image(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, &white, &p_raytracer->gui_context.white_texture);

    tgvk_staging_buffer staging_buffer = { 0 };
    tgvk_staging_buffer_take(TGGUI_COLOR_TYPE_COUNT * sizeof(u32), &p_raytracer->gui_context.colors_ssbo, &staging_buffer);
    for (u32 color_type_idx = 0; color_type_idx < TGGUI_COLOR_TYPE_COUNT; color_type_idx++)
    {
        const u32 color = tg_color_pack(p_raytracer->gui_context.style.p_colors[color_type_idx]);
        tgvk_staging_buffer_push_u32(&staging_buffer, color);
    }
    tgvk_staging_buffer_release(&staging_buffer);



    p_raytracer->debug_pass.capacity = (1 << 14);
    p_raytracer->debug_pass.count = 0;

    tg__init_buffers(p_raytracer);
    tg__init_visibility_pass(p_raytracer);
    tg__init_svo_pass(p_raytracer);
    tg__init_shading_pass(p_raytracer);
    tg__init_debug_pass(p_raytracer);
    tg__init_gui_pass(p_raytracer);
    tg__init_blit_pass(p_raytracer);
    tg__init_present_pass(p_raytracer);
    tg__init_clear_pass(p_raytracer);
}

void tg_raytracer_destroy(tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_raytracer);

	TG_NOT_IMPLEMENTED();
}

void tg_raytracer_create_object(tg_raytracer* p_raytracer, v3 center, v3u extent)
{
    TG_ASSERT(p_raytracer);
    TG_ASSERT(extent.x % TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT == 0);
    TG_ASSERT(extent.y % TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT == 0);
    TG_ASSERT(extent.z % TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT == 0);
    TG_ASSERT(p_raytracer->scene.n_objects < p_raytracer->scene.object_capacity);

    const u32 object_idx = p_raytracer->scene.n_objects++;
    tg_voxel_object* p_object = &p_raytracer->scene.p_objects[object_idx];
    p_object->n_clusters_per_dim = (v3u){ extent.x / 8, extent.y / 8, extent.z / 8 };
    const u32 n_clusters = p_object->n_clusters_per_dim.x * p_object->n_clusters_per_dim.y * p_object->n_clusters_per_dim.z;
    TG_ASSERT(p_raytracer->scene.n_clusters + n_clusters <= p_raytracer->scene.cluster_capacity);
    p_raytracer->scene.n_clusters += n_clusters;

    if (object_idx == 0)
    {
        p_object->first_cluster_idx = 0;
    }
    else
    {
        const tg_voxel_object* p_prev_object = p_object - 1;
        const u32 prev_object_n_clusters = p_prev_object->n_clusters_per_dim.x * p_prev_object->n_clusters_per_dim.y * p_prev_object->n_clusters_per_dim.z;
        p_object->first_cluster_idx = p_prev_object->first_cluster_idx + prev_object_n_clusters;
    }

    p_object->translation = center;
    p_object->angle_in_radians = TG_DEG2RAD((f32)(object_idx * 7));
    p_object->axis = (v3){ 0.0f, 1.0f, 0.0f };

    tg_object_data_ssbo object_data = { 0 };
    object_data.n_clusters_per_dim = p_object->n_clusters_per_dim;
    object_data.first_cluster_idx  = p_object->first_cluster_idx;
    object_data.translation        = p_object->translation;
    object_data.pad                = 0;
    object_data.rotation           = tgm_m4_angle_axis(p_object->angle_in_radians, p_object->axis);

    const tg_size object_data_ubo_staging_buffer_size = sizeof(tg_object_data_ssbo);
    const tg_size object_data_ubo_offset = (tg_size)object_idx * sizeof(tg_object_data_ssbo);
    tgvk_staging_buffer staging_buffer = { 0 };
    tgvk_staging_buffer_take2(object_data_ubo_staging_buffer_size, object_data_ubo_offset, &p_raytracer->data.object_data_ssbo, &staging_buffer);
    tgvk_staging_buffer_push(&staging_buffer, sizeof(object_data), &object_data);

    // CLUSTER IDX TO OBJECT IDX

    tgvk_staging_buffer_reinit2(&staging_buffer, (tg_size)n_clusters * sizeof(u32), p_object->first_cluster_idx * sizeof(u32), &p_raytracer->data.cluster_idx_to_object_idx_ssbo);
    for (u32 relative_cluster_idx = 0; relative_cluster_idx < n_clusters; relative_cluster_idx++)
    {
        tgvk_staging_buffer_push_u32(&staging_buffer, object_idx);
    }

    // TODO: gen on GPU
    // SOLID VOXEL BITMAP

    for (u32 relative_cluster_idx_z = 0; relative_cluster_idx_z < p_object->n_clusters_per_dim.z; relative_cluster_idx_z++)
    {
        for (u32 relative_cluster_idx_y = 0; relative_cluster_idx_y < p_object->n_clusters_per_dim.y; relative_cluster_idx_y++)
        {
            for (u32 relative_cluster_idx_x = 0; relative_cluster_idx_x < p_object->n_clusters_per_dim.x; relative_cluster_idx_x++)
            {
                const u32 relative_cluster_idx
                    = p_object->n_clusters_per_dim.x * p_object->n_clusters_per_dim.y * relative_cluster_idx_z
                    + p_object->n_clusters_per_dim.x * relative_cluster_idx_y
                    + relative_cluster_idx_x;

                const u32 cluster_idx = p_object->first_cluster_idx + relative_cluster_idx;
                TG_ASSERT(cluster_idx < p_raytracer->scene.cluster_capacity);
                p_raytracer->scene.p_cluster_idx_to_object_idx[cluster_idx] = object_idx;

                const tg_size cluster_offset = (tg_size)cluster_idx * TGVK_VOXEL_CLUSTER_SIZE;
                tgvk_staging_buffer_reinit2(&staging_buffer, TGVK_VOXEL_CLUSTER_SIZE, cluster_offset, &p_raytracer->data.voxel_cluster_ssbo);

                const u32 voxel_offset_x = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_idx_x;
                const u32 voxel_offset_y = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_idx_y;
                const u32 voxel_offset_z = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_idx_z;

                u32 voxel_slot_bits = 0;
                u32 voxel_slot = 0;
                for (u32 relative_voxel_z = 0; relative_voxel_z < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_z++)
                {
                    const u32 voxel_z = voxel_offset_z + relative_voxel_z;
                    for (u32 relative_voxel_y = 0; relative_voxel_y < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_y++)
                    {
                        const u32 voxel_y = voxel_offset_y + relative_voxel_y;
                        for (u32 relative_voxel_x = 0; relative_voxel_x < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_x++)
                        {
                            const u32 voxel_x = voxel_offset_x + relative_voxel_x;

                            const f32 xf = (f32)voxel_x + object_idx * 1024.0f;
                            const f32 yf = (f32)voxel_y;
                            const f32 zf = (f32)voxel_z;

                            const f32 n_hills0 = tgm_simplex_noise(xf * 0.008f, 0.0f, zf * 0.008f);
                            const f32 n_hills1 = tgm_simplex_noise(xf * 0.2f, 0.0f, zf * 0.2f);
                            const f32 n_hills = n_hills0 + 0.005f * n_hills1;

                            const f32 s_caves = 0.06f;
                            const f32 c_caves_x = s_caves * xf;
                            const f32 c_caves_y = s_caves * yf;
                            const f32 c_caves_z = s_caves * zf;
                            const f32 unclamped_noise_caves = tgm_simplex_noise(c_caves_x, c_caves_y, c_caves_z);
                            const f32 n_caves = tgm_f32_clamp(unclamped_noise_caves, -1.0f, 0.0f);

                            const f32 n = n_hills;
                            const f32 noise = (n * 64.0f) - ((f32)voxel_y - 8.0f) + (10.0f * n_caves);
                            const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
                            const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
                            const f32 f1 = 254.0f * f0;
                            const i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
                            const b32 solid = f2 <= 0 || voxel_y == 0;

                            voxel_slot |= (solid << voxel_slot_bits++);
                            if (voxel_slot_bits == 32)
                            {
                                const tg_size idx = (cluster_offset + staging_buffer.copied_size + staging_buffer.filled_size) / sizeof(u32);
                                p_raytracer->scene.p_voxel_cluster_data[idx] = voxel_slot;
                                tgvk_staging_buffer_push_u32(&staging_buffer, voxel_slot);
                                voxel_slot_bits = 0;
                                voxel_slot = 0;
                            }
                        }
                    }
                }
            }
        }
    }

    // COLOR LUT

    for (u32 relative_cluster_idx_z = 0; relative_cluster_idx_z < p_object->n_clusters_per_dim.z; relative_cluster_idx_z++)
    {
        for (u32 relative_cluster_idx_y = 0; relative_cluster_idx_y < p_object->n_clusters_per_dim.y; relative_cluster_idx_y++)
        {
            for (u32 relative_cluster_idx_x = 0; relative_cluster_idx_x < p_object->n_clusters_per_dim.x; relative_cluster_idx_x++)
            {
                const u32 relative_cluster_idx
                    = TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_idx_z
                    + TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT * relative_cluster_idx_y
                    + relative_cluster_idx_x;
                const u32 cluster_idx = p_object->first_cluster_idx + relative_cluster_idx;

                const tg_size cluster_offset = (tg_size)cluster_idx * TGVK_COLOR_LUT_IDX_CLUSTER_SIZE;
                tgvk_staging_buffer_reinit2(&staging_buffer, TGVK_COLOR_LUT_IDX_CLUSTER_SIZE, cluster_offset, &p_raytracer->data.color_lut_idx_cluster_ssbo);

                for (u32 relative_voxel_z = 0; relative_voxel_z < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_z++)
                {
                    for (u32 relative_voxel_y = 0; relative_voxel_y < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_y++)
                    {
                        for (u32 relative_voxel_x = 0; relative_voxel_x < TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT; relative_voxel_x++)
                        {
                            const u8 v = (u8)((cluster_idx + relative_voxel_x) % 256);
                            tgvk_staging_buffer_push_u8(&staging_buffer, v);
                        }
                    }
                }
            }
        }
    }

    tgvk_staging_buffer_release(&staging_buffer);

    // TODO: lods
}

void tg_raytracer_push_debug_cuboid(tg_raytracer* p_raytracer, m4 transformation_matrix, v3 color)
{
    // TODO: color
    TG_ASSERT(p_raytracer);
    TG_ASSERT(p_raytracer->debug_pass.count < p_raytracer->debug_pass.capacity);
    TG_ASSERT(p_raytracer->debug_pass.count < p_raytracer->scene.cluster_capacity); // Note: We reuse cluster indices for instancing

    m4* p_transformation_matrices = p_raytracer->data.debug_matrices_ssbo.memory.p_mapped_device_memory;
    p_transformation_matrices[p_raytracer->debug_pass.count] = transformation_matrix;

    v4* p_colors = p_raytracer->data.debug_colors_ssbo.memory.p_mapped_device_memory;
    p_colors[p_raytracer->debug_pass.count].xyz = color;

    p_raytracer->debug_pass.count++;
}

void tg_raytracer_push_debug_line(tg_raytracer* p_raytracer, v3 src, v3 dst, v3 color)
{
    const v3 src2dst = tgm_v3_sub(dst, src);
    const f32 mag = tgm_v3_mag(src2dst);
    const m4 scale = tgm_m4_scale((v3) { mag, 0.0f, 0.0f });
    
    m4 rotation = tgm_m4_identity();
    if (mag > 0.0f)
    {
        const v3 right = { 1.0f, 0.0f, 0.0f };
        const v3 src2dst_n = tgm_v3_divf(src2dst, mag);
        const f32 cos_angle = tgm_v3_dot(right, src2dst_n);
        if (cos_angle != 1.0f && cos_angle != -1.0f)
        {
            const f32 angle = tgm_f32_acos(cos_angle);
            const v3 axis = tgm_v3_cross(right, src2dst_n);
            rotation = tgm_m4_angle_axis(angle, axis);
        }
    }

    const v3 center = tgm_v3_add(src, tgm_v3_divf(src2dst, 2.0f));
    const m4 translation = tgm_m4_translate(center);

    const m4 transformation_matrix = tgm_m4_transform_srt(scale, rotation, translation);
    tg_raytracer_push_debug_cuboid(p_raytracer, transformation_matrix, color);
}

void tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b)
{
    const u32 color_lut_idx = 0; // TODO

    TG_ASSERT(p_raytracer != TG_NULL);
    TG_ASSERT(r <= 1.0f);
    TG_ASSERT(g <= 1.0f);
    TG_ASSERT(b <= 1.0f);

    const u32 r_u32 = (u32)(r * 255.0f);
    const u32 g_u32 = (u32)(g * 255.0f);
    const u32 b_u32 = (u32)(b * 255.0f);
    const u32 a_u32 = 255; // TODO: Metallic/Roughness?
    const u32 packed_color = r_u32 << 24 | g_u32 << 16 | b_u32 << 8 | a_u32;

    tgvk_staging_buffer staging_buffer = { 0 };
    const tg_size dst_offset = (tg_size)color_lut_idx * 256 * sizeof(u32) + (tg_size)index * sizeof(u32);
    tgvk_staging_buffer_take2(sizeof(u32), dst_offset, &p_raytracer->data.object_color_lut_ssbo, &staging_buffer);
    tgvk_staging_buffer_push_u32(&staging_buffer, packed_color);
    tgvk_staging_buffer_release(&staging_buffer);
}

void tg_raytracer_render(tg_raytracer* p_raytracer)
{
    TG_ASSERT(p_raytracer);
    TG_ASSERT(p_raytracer->scene.n_objects > 0);

    tg_view_projection_ubo* p_view_projection_ubo = p_raytracer->data.view_projection_ubo.memory.p_mapped_device_memory;
    tg_camera_ubo*          p_camera_ubo          = p_raytracer->data.camera_ubo.memory.p_mapped_device_memory;
    tg_environment_ubo*     p_environment_ubo     = p_raytracer->data.environment_ubo.memory.p_mapped_device_memory;

    const tg_camera c = *p_raytracer->p_camera;

    const m4 cam_r   = tgm_m4_inverse(tgm_m4_euler(c.pitch, c.yaw, c.roll));
    const m4 cam_t   = tgm_m4_translate(tgm_v3_neg(c.position));
    const m4 cam_v   = tgm_m4_mul(cam_r, cam_t);
    const m4 cam_iv  = tgm_m4_inverse(cam_v);
    const m4 cam_p   = c.type == TG_CAMERA_TYPE_ORTHOGRAPHIC ? tgm_m4_orthographic(c.ortho.l, c.ortho.r, c.ortho.b, c.ortho.t, c.ortho.f, c.ortho.n) : tgm_m4_perspective(c.persp.fov_y_in_radians, c.persp.aspect, c.persp.n, c.persp.f);
    const m4 cam_ip  = tgm_m4_inverse(cam_p);
    const m4 cam_vp  = tgm_m4_mul(cam_p, cam_v);
    const m4 cam_ivp = tgm_m4_inverse(cam_vp);

    TG_UNUSED(cam_iv);
    TG_UNUSED(cam_ip);
    TG_UNUSED(cam_ivp);

    p_view_projection_ubo->v = cam_v;
    p_view_projection_ubo->p = cam_p;

    const m4 ivp_no_translation = tgm_m4_inverse(tgm_m4_mul(cam_p, cam_r));
    const v3 ray00 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray10 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f, -1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray01 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray11 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f, -1.0f, 1.0f, 1.0f }).xyz);

    p_camera_ubo->camera.xyz = c.position;
    p_camera_ubo->ray00.xyz  = ray00;
    p_camera_ubo->ray10.xyz  = ray10;
    p_camera_ubo->ray01.xyz  = ray01;
    p_camera_ubo->ray11.xyz  = ray11;
    p_camera_ubo->near       = c.persp.n;
    p_camera_ubo->far        = c.persp.f;

    p_environment_ubo->sun_direction.xyz = (v3){ 0.0f, -1.0f, 0.0f };
    //p_shading_ubo->ivp = ivp;

    // BVH

    tg_svo* p_svo = &p_raytracer->scene.svo;
    if (p_svo->p_node_buffer == TG_NULL)
    {
        // TODO: update BVH here, parallel to visibility buffer computation
        // https://www.nvidia.com/docs/IO/88972/nvr-2010-001.pdf
        // https://luebke.us/publications/eg09.pdf
        const v3 extent_min = { -512.0f, -512.0f, -512.0f };
        const v3 extent_max = {  512.0f,  512.0f,  512.0f };
        tg_svo_create(extent_min, extent_max, &p_raytracer->scene, p_svo);
        //tg_svo_destroy(p_svo);

        const tg_size svo_ssbo_size                = 2 * sizeof(v4);
        const tg_size svo_nodes_ssbo_size          = p_raytracer->scene.svo.node_buffer_capacity           * sizeof(*p_raytracer->scene.svo.p_node_buffer);
        const tg_size svo_leaf_node_data_ssbo_size = p_raytracer->scene.svo.leaf_node_data_buffer_capacity * sizeof(*p_raytracer->scene.svo.p_leaf_node_data_buffer);
        const tg_size svo_voxel_data_ssbo_size     = p_raytracer->scene.svo.voxel_buffer_capacity_in_u32   * sizeof(*p_raytracer->scene.svo.p_voxels_buffer);
        const VkBufferUsageFlags ssbo_usage_flags  = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        p_raytracer->data.svo_ssbo                = TGVK_BUFFER_CREATE(svo_ssbo_size,                ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->data.svo_nodes_ssbo          = TGVK_BUFFER_CREATE(svo_nodes_ssbo_size,          ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->data.svo_leaf_node_data_ssbo = TGVK_BUFFER_CREATE(svo_leaf_node_data_ssbo_size, ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->data.svo_voxel_data_ssbo     = TGVK_BUFFER_CREATE(svo_voxel_data_ssbo_size,     ssbo_usage_flags, TGVK_MEMORY_DEVICE);

        v4 p_svo_min_max[2] = { 0 };
        p_svo_min_max[0].xyz = p_svo->min;
        p_svo_min_max[1].xyz = p_svo->max;

        tgvk_util_copy_to_buffer(2 * sizeof(v4),               p_svo_min_max,                  &p_raytracer->data.svo_ssbo);
        tgvk_util_copy_to_buffer(svo_nodes_ssbo_size,          p_svo->p_node_buffer,           &p_raytracer->data.svo_nodes_ssbo);
        tgvk_util_copy_to_buffer(svo_leaf_node_data_ssbo_size, p_svo->p_leaf_node_data_buffer, &p_raytracer->data.svo_leaf_node_data_ssbo);
        tgvk_util_copy_to_buffer(svo_voxel_data_ssbo_size,     p_svo->p_voxels_buffer,         &p_raytracer->data.svo_voxel_data_ssbo);
    }



    // VISIBILITY

    //tgvk_atmosphere_model_update(&p_raytracer->model, iv, ip);
    const VkDeviceSize vertex_buffer_offset = 0;

    static b32 show_svo = TG_FALSE;
    if (tg_input_is_key_pressed(TG_KEY_F, TG_TRUE))
    {
        show_svo = !show_svo;
    }
    if (show_svo)
    {
        tgvk_command_buffer_begin(&p_raytracer->svo_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
        tgvk_cmd_begin_render_pass(&p_raytracer->svo_pass.command_buffer, p_raytracer->svo_pass.render_pass, &p_raytracer->svo_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(p_raytracer->svo_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->svo_pass.graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(p_raytracer->svo_pass.command_buffer.buffer, screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->svo_pass.command_buffer.buffer, 0, 1, &p_raytracer->data.idx_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->svo_pass.command_buffer.buffer, 1, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);

        // TODO: update required here? Otherwise mark as dirty. Also, draw indirect
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.camera_ubo,              0);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.visibility_buffer_ssbo,  1);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.svo_ssbo,                2);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.svo_nodes_ssbo,          3);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.svo_leaf_node_data_ssbo, 4);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->data.svo_voxel_data_ssbo,     5);

        vkCmdBindDescriptorSets(p_raytracer->svo_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->svo_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->svo_pass.descriptor_set.set, 0, TG_NULL);

        tgvk_cmd_draw_indexed_instanced(&p_raytracer->svo_pass.command_buffer, 6, 1); // TODO: triangle fans for fewer indices?

        // TODO look at below
        //vkCmdExecuteCommands(p_raytracer->geometry_pass.command_buffer.buffer, p_raytracer->deferred_command_buffer_count, p_raytracer->p_deferred_command_buffers);
        vkCmdEndRenderPass(p_raytracer->svo_pass.command_buffer.buffer);
        TGVK_CALL(vkEndCommandBuffer(p_raytracer->svo_pass.command_buffer.buffer));

        VkSubmitInfo visibility_submit_info = { 0 };
        visibility_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        visibility_submit_info.pNext = TG_NULL;
        visibility_submit_info.waitSemaphoreCount = 0;
        visibility_submit_info.pWaitSemaphores = TG_NULL;
        visibility_submit_info.pWaitDstStageMask = TG_NULL;
        visibility_submit_info.commandBufferCount = 1;
        visibility_submit_info.pCommandBuffers = &p_raytracer->svo_pass.command_buffer.buffer;
        visibility_submit_info.signalSemaphoreCount = 1;
        visibility_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

        // Note: This fence does not need to be at the top, because we only wait for the presentation
        // to finish. The visibility pass has long finished. Therefore, all buffers, that are not
        // required by the presentation pass, can be updated before waiting for this fence.
        tgvk_fence_wait(p_raytracer->render_target.fence);
        tgvk_fence_reset(p_raytracer->render_target.fence);

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &visibility_submit_info, VK_NULL_HANDLE);
    }
    else
    {
        tgvk_command_buffer_begin(&p_raytracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
        tgvk_cmd_begin_render_pass(&p_raytracer->visibility_pass.command_buffer, p_raytracer->visibility_pass.render_pass, &p_raytracer->visibility_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
    
        vkCmdBindPipeline(p_raytracer->visibility_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->visibility_pass.graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(p_raytracer->visibility_pass.command_buffer.buffer, p_raytracer->data.cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 0, 1, &p_raytracer->data.idx_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 1, 1, &p_raytracer->data.cube_vbo.buffer, &vertex_buffer_offset);

        // TODO: update required here? Otherwise mark as dirty. Also, draw indirect
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.cluster_idx_to_object_idx_ssbo, 0);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.object_data_ssbo,               1);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.voxel_cluster_ssbo,             2);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.visibility_buffer_ssbo,         3);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.view_projection_ubo,            4);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->data.camera_ubo,                     5);

        vkCmdBindDescriptorSets(p_raytracer->visibility_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->visibility_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->visibility_pass.descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_draw_indexed_instanced(&p_raytracer->visibility_pass.command_buffer, 6 * 6, p_raytracer->scene.n_clusters); // TODO: triangle fans for fewer indices?

        // TODO look at below
        //vkCmdExecuteCommands(p_raytracer->geometry_pass.command_buffer.buffer, p_raytracer->deferred_command_buffer_count, p_raytracer->p_deferred_command_buffers);
        vkCmdEndRenderPass(p_raytracer->visibility_pass.command_buffer.buffer);
        TGVK_CALL(vkEndCommandBuffer(p_raytracer->visibility_pass.command_buffer.buffer));

        VkSubmitInfo visibility_submit_info = { 0 };
        visibility_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        visibility_submit_info.pNext = TG_NULL;
        visibility_submit_info.waitSemaphoreCount = 0;
        visibility_submit_info.pWaitSemaphores = TG_NULL;
        visibility_submit_info.pWaitDstStageMask = TG_NULL;
        visibility_submit_info.commandBufferCount = 1;
        visibility_submit_info.pCommandBuffers = &p_raytracer->visibility_pass.command_buffer.buffer;
        visibility_submit_info.signalSemaphoreCount = 1;
        visibility_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

        // Note: This fence does not need to be at the top, because we only wait for the presentation
        // to finish. The visibility pass has long finished. Therefore, all buffers, that are not
        // required by the presentation pass, can be updated before waiting for this fence.
        tgvk_fence_wait(p_raytracer->render_target.fence);
        tgvk_fence_reset(p_raytracer->render_target.fence);

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &visibility_submit_info, VK_NULL_HANDLE);
    }



    // SHADING

    // TODO: remove this line
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->data.color_lut_idx_cluster_ssbo, 7);
    tgvk_command_buffer_begin(&p_raytracer->shading_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->shading_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->shading_pass.graphics_pipeline.pipeline);

        vkCmdBindIndexBuffer(p_raytracer->shading_pass.command_buffer.buffer, screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 0, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 1, 1, &screen_quad_uvs_vbo.buffer, &vertex_buffer_offset);

        tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->data.svo_ssbo,                11);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->data.svo_nodes_ssbo,          12);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->data.svo_leaf_node_data_ssbo, 13);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->data.svo_voxel_data_ssbo,     14);

        vkCmdBindDescriptorSets(p_raytracer->shading_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->shading_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->shading_pass.descriptor_set.set, 0, TG_NULL);

        tgvk_cmd_begin_render_pass(&p_raytracer->shading_pass.command_buffer, p_raytracer->shading_pass.render_pass, &p_raytracer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        tgvk_cmd_draw_indexed(&p_raytracer->shading_pass.command_buffer, 6);
        vkCmdEndRenderPass(p_raytracer->shading_pass.command_buffer.buffer);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->shading_pass.command_buffer.buffer));

    const VkPipelineStageFlags color_attachment_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: don't we have to wait for depth as well? also check stages below (draw)

    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &p_raytracer->semaphore;
    shading_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &p_raytracer->shading_pass.command_buffer.buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);



    // DEBUG

    if (1)
    {
        if (1)
        {
            for (u32 object_idx = 0; object_idx < p_raytracer->scene.n_objects; object_idx++)
            {
                const tg_voxel_object* p_object = &p_raytracer->scene.p_objects[object_idx];
                const v3 scale = tgm_v3u_to_v3(tgm_v3u_mulu(p_object->n_clusters_per_dim, TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT));
                const m4 s = tgm_m4_scale(scale);
                const m4 r = tgm_m4_angle_axis(p_object->angle_in_radians, p_object->axis);
                const m4 t = tgm_m4_translate(p_object->translation);
                const m4 model_matrix = tgm_m4_transform_srt(s, r, t);
                tg_raytracer_push_debug_cuboid(p_raytracer, model_matrix, (v3) { 1.0f, 1.0f, 0.0f });
            }
        }
        if (1)
        {
            tg__push_debug_svo_node(p_raytracer, &p_svo->p_node_buffer[0].inner, p_svo->min, p_svo->max, TG_TRUE, TG_TRUE);
        }
        if (0)
        {
            const v3 color_hit = { 1.0f, 0.0f, 0.0f };
            const v3 color_no_hit = { 0.5f, 0.5f, 1.0f };

            f32 distance;
            u32 node_idx, voxel_idx;
            
            const v3 primary_ray_position_ws = c.position;
            const v3 primary_ray_direction_ws = tgm_v3_normalized(tgm_v3_lerp(tgm_v3_lerp(ray00, ray10, 0.5f), tgm_v3_lerp(ray01, ray11, 0.5f), 0.5f));
            if (tg_svo_traverse(p_svo, primary_ray_position_ws, primary_ray_direction_ws, &distance, &node_idx, &voxel_idx))
            {
                const v3 primary_ray_hit_position_ws = tgm_v3_add(primary_ray_position_ws, tgm_v3_mulf(primary_ray_direction_ws, distance));
                tg_raytracer_push_debug_cuboid(p_raytracer, tgm_m4_translate(primary_ray_hit_position_ws), color_hit);

                const tg_system_time time = tgp_get_system_time();
                tg_rand_xorshift32 rand = { 0 };
                tgm_rand_xorshift32_init(time.second == 0 ? 1 : (u32)time.second, &rand);

                for (u32 attempt_idx = 0; attempt_idx < 32; attempt_idx++)
                {
                    const v3 secondary_ray_direction_ws = tgm_v3_normalized((v3) {
                        tgm_rand_xorshift32_next_f32_inclusive_range(&rand, -1.0, 1.0),
                        tgm_rand_xorshift32_next_f32_inclusive_range(&rand, -1.0, 1.0),
                        tgm_rand_xorshift32_next_f32_inclusive_range(&rand, -1.0, 1.0)
                    });

                    if (tgm_v3_dot(secondary_ray_direction_ws, tgm_v3_neg(primary_ray_direction_ws)) > 0.70710678118f) // TODO: forward_ws is only almost correct. We need the normal of the hit object! Then, we can change it to '> 0.0f'
                    {
                        const v3 secondary_ray_position_ws = tgm_v3_add(primary_ray_hit_position_ws, tgm_v3_mulf(secondary_ray_direction_ws, 1.73205080757f));
                        if (tg_svo_traverse(p_svo, secondary_ray_position_ws, secondary_ray_direction_ws, &distance, &node_idx, &voxel_idx))
                        {
                            const v3 new_dst = tgm_v3_add(primary_ray_hit_position_ws, tgm_v3_mulf(secondary_ray_direction_ws, distance));
                            tg_raytracer_push_debug_line(p_raytracer, secondary_ray_position_ws, new_dst, color_hit);
                        }
                        else
                        {
                            const v3 new_dst = tgm_v3_add(primary_ray_hit_position_ws, tgm_v3_mulf(secondary_ray_direction_ws, 100.0f));
                            tg_raytracer_push_debug_line(p_raytracer, secondary_ray_position_ws, new_dst, color_no_hit);
                        }
                        break;
                    }
                }
            }
        }
        if (p_raytracer->debug_pass.count > 0)
        {
            tgvk_command_buffer_begin(&p_raytracer->debug_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
            tgvk_cmd_begin_render_pass(&p_raytracer->debug_pass.command_buffer, p_raytracer->debug_pass.render_pass, &p_raytracer->debug_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.graphics_pipeline.pipeline);
            vkCmdBindIndexBuffer(p_raytracer->debug_pass.command_buffer.buffer, p_raytracer->data.cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 0, 1, &p_raytracer->data.idx_vbo.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 1, 1, &p_raytracer->data.cube_vbo.buffer, &vertex_buffer_offset);

            tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.debug_matrices_ssbo, 0);
            tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.view_projection_ubo, 1);
            tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->data.debug_colors_ssbo,   2);

            vkCmdBindDescriptorSets(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->debug_pass.descriptor_set.set, 0, TG_NULL);
            tgvk_cmd_draw_indexed_instanced(&p_raytracer->debug_pass.command_buffer, 6 * 6, p_raytracer->debug_pass.count); // TODO: triangle fans for fewer indices? same above

            vkCmdEndRenderPass(p_raytracer->debug_pass.command_buffer.buffer);
            TGVK_CALL(vkEndCommandBuffer(p_raytracer->debug_pass.command_buffer.buffer));

            VkSubmitInfo debug_submit_info = { 0 };
            debug_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            debug_submit_info.pNext = TG_NULL;
            debug_submit_info.waitSemaphoreCount = 1;
            debug_submit_info.pWaitSemaphores = &p_raytracer->semaphore;
            debug_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
            debug_submit_info.commandBufferCount = 1;
            debug_submit_info.pCommandBuffers = &p_raytracer->debug_pass.command_buffer.buffer;
            debug_submit_info.signalSemaphoreCount = 1;
            debug_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

            tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &debug_submit_info, VK_NULL_HANDLE);
        }
    }



    // GUI

    tggui_context* p_guic = &p_raytracer->gui_context;
    u32 first_instance = 0;
    for (u32 draw_call_idx = 0; draw_call_idx < p_guic->n_draw_calls; draw_call_idx++)
    {
        TG_ASSERT(p_guic->p_textures[draw_call_idx] != TG_NULL);
        TG_ASSERT(p_guic->p_n_instances_per_draw_call[draw_call_idx] > 0);

        tgvk_command_buffer* p_cmd_buf = &p_raytracer->gui_pass.command_buffer;

        tgvk_fence_wait(p_raytracer->gui_pass.fence);
        tgvk_fence_reset(p_raytracer->gui_pass.fence);

        tgvk_command_buffer_begin(p_cmd_buf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        tgvk_cmd_begin_render_pass(p_cmd_buf, p_raytracer->gui_pass.render_pass, &p_raytracer->gui_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(p_cmd_buf->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->gui_pass.graphics_pipeline.pipeline);
        vkCmdBindVertexBuffers(p_cmd_buf->buffer, 0, 1, &p_raytracer->data.idx_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_cmd_buf->buffer, 1, 1, &p_raytracer->data.quad_vbo.buffer, &vertex_buffer_offset);

        tgvk_descriptor_set_update_storage_buffer(p_raytracer->gui_pass.descriptor_set.set, &p_guic->transform_ssbo, 0);
        tgvk_descriptor_set_update_image(p_raytracer->gui_pass.descriptor_set.set, p_guic->p_textures[draw_call_idx], 1);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->gui_pass.descriptor_set.set, &p_guic->colors_ssbo, 2);

        vkCmdBindDescriptorSets(p_cmd_buf->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->gui_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->gui_pass.descriptor_set.set, 0, TG_NULL);

        const u32 n_instances = p_guic->p_n_instances_per_draw_call[draw_call_idx];
        tgvk_cmd_draw_instanced2(p_cmd_buf, 6, n_instances, first_instance);
        first_instance += p_guic->p_n_instances_per_draw_call[draw_call_idx];

        vkCmdEndRenderPass(p_cmd_buf->buffer);
        TGVK_CALL(vkEndCommandBuffer(p_cmd_buf->buffer));

        VkSubmitInfo ui_submit_info = { 0 };
        ui_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ui_submit_info.pNext = TG_NULL;
        ui_submit_info.waitSemaphoreCount = 1;
        ui_submit_info.pWaitSemaphores = &p_raytracer->semaphore;
        ui_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
        ui_submit_info.commandBufferCount = 1;
        ui_submit_info.pCommandBuffers = &p_cmd_buf->buffer;
        ui_submit_info.signalSemaphoreCount = 1;
        ui_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &ui_submit_info, p_raytracer->gui_pass.fence);
    }



    // PRESENT

    u32 current_image;
    TGVK_CALL(vkAcquireNextImageKHR(device, swapchain, TG_U64_MAX, p_raytracer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    const VkSemaphore p_wait_semaphores[2] = { p_raytracer->semaphore, p_raytracer->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

    VkSubmitInfo present_submit_info = { 0 };
    present_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    present_submit_info.pNext = TG_NULL;
    present_submit_info.waitSemaphoreCount = 2;
    present_submit_info.pWaitSemaphores = p_wait_semaphores;
    present_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
    present_submit_info.commandBufferCount = 1;
    present_submit_info.pCommandBuffers = &p_raytracer->present_pass.p_command_buffers[current_image].buffer;
    present_submit_info.signalSemaphoreCount = 1;
    present_submit_info.pSignalSemaphores = &p_raytracer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &present_submit_info, p_raytracer->render_target.fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &p_raytracer->semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    tgvk_queue_present(&present_info);
}

void tg_raytracer_clear(tg_raytracer* p_raytracer)
{
    TG_ASSERT(p_raytracer);

    tgvk_fence_wait(p_raytracer->render_target.fence);
    tgvk_fence_reset(p_raytracer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_raytracer->clear_pass.command_buffer.buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_raytracer->render_target.fence);

    tggui_context* p_guic = &p_raytracer->gui_context;
    p_guic->n_quads = 0;
    p_guic->n_draw_calls = 0;
    for (u32 i = 0; i < TGGUI_MAX_N_DRAW_CALLS; i++)
    {
        p_guic->p_textures[i] = TG_NULL;
        p_guic->p_n_instances_per_draw_call[i] = 0;
    }
    p_guic->offset_x = 0;
    p_guic->offset_y = 0;

    p_raytracer->debug_pass.count = 0;

    // TODO: forward renderer
    //tg_shading_data_ubo* p_shading_ubo = p_raytracer->shading_pass.shading_data_ubo.memory.p_mapped_device_memory;

    // TODO: lighting
    //p_shading_ubo->directional_light_count = 0;
    //p_shading_ubo->point_light_count = 0;
}



void tggui_window_set_next_position(tg_raytracer* p_raytracer, f32 position_x, f32 position_y)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    p_raytracer->gui_context.window_next_position_x = position_x;
    p_raytracer->gui_context.window_next_position_y = position_y;
}

void tggui_window_set_next_size(tg_raytracer* p_raytracer, f32 size_x, f32 size_y)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    p_raytracer->gui_context.window_next_size_x = size_x;
    p_raytracer->gui_context.window_next_size_y = size_y;
}

void tggui_window_begin(tg_raytracer* p_raytracer, const char* p_window_name)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    const v2 target_size = {
        (f32)p_raytracer->render_target.color_attachment.width,
        (f32)p_raytracer->render_target.color_attachment.height
    };

    tggui_context* p_guic = &p_raytracer->gui_context;

    const u32 draw_call_idx = p_guic->n_draw_calls++;
    p_guic->p_textures[draw_call_idx] = &p_guic->white_texture;
    p_guic->p_n_instances_per_draw_call[draw_call_idx]++;

    const v2 min = { p_guic->window_next_position_x, p_guic->window_next_position_y };
    const v2 max = { min.x + p_guic->window_next_size_x, min.y + p_guic->window_next_size_y };
    const v2 rel_min = tg__screen_space_to_clip_space(min, target_size);
    const v2 rel_max = tg__screen_space_to_clip_space(max, target_size);
    const v2 translation = tgm_v2_divf(tgm_v2_add(rel_min, rel_max), 2.0f);
    const v2 scale = tgm_v2_sub(rel_max, rel_min);

    tggui_transform_ssbo* p_entry = &((tggui_transform_ssbo*)p_raytracer->gui_context.transform_ssbo.memory.p_mapped_device_memory)[p_raytracer->gui_context.n_quads++];

    p_entry->translation_x   = translation.x;
    p_entry->translation_y   = translation.y;
    p_entry->half_scale_x    = scale.x / 2.0f;
    p_entry->half_scale_y    = scale.y / 2.0f;
    p_entry->uv_center_x     = 0.0f;
    p_entry->uv_center_y     = 0.0f;
    p_entry->uv_half_scale_x = 0.0f;
    p_entry->uv_half_scale_y = 0.0f;
    p_entry->type            = (u32)TGGUI_COLOR_WINDOW_BG;

    p_raytracer->gui_context.offset_x = min.x + 0.0f; // TODO: which values for padding?
    p_raytracer->gui_context.offset_y = min.y + 0.0f;
}

void tggui_window_end(tg_raytracer* p_raytracer)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    tggui_context* p_guic = &p_raytracer->gui_context;
    p_guic->offset_x = 0;
    p_guic->offset_y = 0;
}

void tggui_same_line(tg_raytracer* p_raytracer)
{
    TG_ASSERT(p_raytracer != TG_NULL);
}

b32 tggui_button(tg_raytracer* p_raytracer, const char* p_label)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    return TG_FALSE;
}

b32 tggui_checkbox(tg_raytracer* p_raytracer, const char* p_label, b32* p_value)
{
    TG_ASSERT(p_raytracer != TG_NULL);

    return TG_FALSE;
}

void tggui_text(tg_raytracer* p_raytracer, const char* p_format, ...)
{
    TG_ASSERT(p_raytracer != TG_NULL);
    TG_ASSERT(p_format != TG_NULL);
    TG_ASSERT(p_raytracer->gui_context.n_draw_calls > 0); // We need a window

    u32 draw_call_idx = p_raytracer->gui_context.n_draw_calls - 1;
    if (p_raytracer->gui_context.p_textures[draw_call_idx] != &p_raytracer->gui_context.style.font.texture_atlas)
    {
        draw_call_idx = p_raytracer->gui_context.n_draw_calls++;
        p_raytracer->gui_context.p_textures[draw_call_idx] = &p_raytracer->gui_context.style.font.texture_atlas;
    }

    char* p_variadic_arguments = TG_NULL;
    tg_variadic_start(p_variadic_arguments, p_format);
    char p_buffer[1024] = { 0 };
    tg_stringf_va(sizeof(p_buffer), p_buffer, p_format, p_variadic_arguments);
    tg_variadic_end(p_variadic_arguments);

    const tgvk_font* p_font = &p_raytracer->gui_context.style.font;
    const v2 target_size = {
        (f32)p_raytracer->render_target.color_attachment.width,
        (f32)p_raytracer->render_target.color_attachment.height
    };

    f32 x_offset = p_raytracer->gui_context.offset_x;
    f32 y_offset = p_raytracer->gui_context.offset_y;
    const char* p_it = p_buffer;

    while (*p_it != '\0')
    {
        TG_ASSERT(p_raytracer->gui_context.n_quads < p_raytracer->gui_context.quad_capacity);

        const char c = *p_it++;
        const u8 glyph_idx = p_font->p_char_to_glyph[c];
        const tgvk_glyph* p_glyph = &p_font->p_glyphs[glyph_idx];

        const v2 min = {
            x_offset + p_glyph->left_side_bearing,
            y_offset + p_font->max_glyph_height - p_glyph->size.y - p_glyph->bottom_side_bearing
        };
        const v2 max = tgm_v2_add(min, p_glyph->size);

        if (!tgm_v2_eq(min, max))
        {
            const v2 rel_min = tg__screen_space_to_clip_space(min, target_size);
            const v2 rel_max = tg__screen_space_to_clip_space(max, target_size);
            const v2 translation = tgm_v2_divf(tgm_v2_add(rel_min, rel_max), 2.0f);
            const v2 scale = tgm_v2_sub(rel_max, rel_min);

            tggui_transform_ssbo* p_char_ssbo = &((tggui_transform_ssbo*)p_raytracer->gui_context.transform_ssbo.memory.p_mapped_device_memory)[p_raytracer->gui_context.n_quads++];

            p_char_ssbo->translation_x   = translation.x;
            p_char_ssbo->translation_y   = translation.y;
            p_char_ssbo->half_scale_x    = scale.x / 2.0f;
            p_char_ssbo->half_scale_y    = scale.y / 2.0f;
            p_char_ssbo->uv_center_x     = (p_glyph->uv_min.x + p_glyph->uv_max.x) / 2.0f;
            p_char_ssbo->uv_center_y     = ((1.0f - p_glyph->uv_min.y) + (1.0f - p_glyph->uv_max.y)) / 2.0f;
            p_char_ssbo->uv_half_scale_x = (p_glyph->uv_max.x - p_glyph->uv_min.x) / 2.0f;
            p_char_ssbo->uv_half_scale_y = -((1.0f - p_glyph->uv_max.y) - (1.0f - p_glyph->uv_min.y)) / 2.0f;
            p_char_ssbo->type            = (u32)TGGUI_COLOR_TEXT;

            x_offset += p_glyph->advance_width;

            const char next_c = *p_it;
            if (next_c != '\0')
            {
                const u8 next_glyph_idx = p_font->p_char_to_glyph[next_c];
                for (u32 kerning_idx = 0; kerning_idx < p_glyph->kerning_count; kerning_idx++)
                {
                    const tgvk_kerning* p_kerning = &p_glyph->p_kernings[kerning_idx];
                    if (p_kerning->right_glyph_idx == next_glyph_idx)
                    {
                        x_offset += p_kerning->kerning;
                        break;
                    }
                }
            }

            p_raytracer->gui_context.p_n_instances_per_draw_call[draw_call_idx]++;
        }
    }

    p_raytracer->gui_context.offset_y += p_font->max_glyph_height;
}


#endif
