#include "graphics/vulkan/tgvk_raytracer.h"

#ifdef TG_VULKAN

#include "graphics/tg_sparse_voxel_octree.h"
#include "graphics/vulkan/tgvk_shader_library.h"
#include "physics/tg_physics.h"
#include "tg_input.h"
#include "util/tg_qsort.h"



#define TGVK_HDR_FORMAT                    VK_FORMAT_R32G32B32A32_SFLOAT
#define TGVK_STORED_NUM_VOXELS(w, h, d)    ((((w) * (h) * (d) + 31) / 32) * 32)
#define TGVK_STORED_SIZE(w, h, d)          (TGVK_STORED_NUM_VOXELS(w, h, d) / 8)
#define TGVK_MAX_VOXELS                    1073741823 /* We represent the voxel    ids with 30 bits */
#define TGVK_MAX_INSTANCES                 1023       /* We represent the instance ids with 10 bits */
#define TGVK_VOXEL_ID_BITS                 30
#define TGVK_INSTANCE_ID_BITS              10



typedef struct tg_instance_data_ssbo
{
    m4     s;
    m4     r;
    m4     t;
    u32    w;
    u32    h;
    u32    d;
    u32    first_voxel_id;
} tg_instance_data_ssbo;

typedef struct tg_view_projection_ubo
{
    m4    v;
    m4    p;
} tg_view_projection_ubo;

typedef struct tg_raytracer_data_ubo
{
    v4     camera;
    v4     ray00;
    v4     ray10;
    v4     ray01;
    v4     ray11;
    f32    near;
    f32    far;
} tg_raytracer_data_ubo;

typedef struct tg_visibility_buffer_ssbo
{
    u32    w;
    u32    h;
    u64    p_data[];
} tg_visibility_buffer_ssbo;

typedef struct tg_shading_data_ubo
{
    v4     camera;
    v4     ray00;
    v4     ray10;
    v4     ray01;
    v4     ray11;
    v4     sun_direction;
    f32    near;
    f32    far;
    //u32    directional_light_count;
    //u32    point_light_count; u32 pad[2];
    //m4     ivp; // inverse view projection
    //v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    //v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_shading_data_ubo;



static void tg__push_debug_svo_node(tg_raytracer* p_raytracer, const tg_svo_inner_node* p_inner, v3 min, v3 max, b32 push_children)
{
    const v3 extent = tgm_v3_sub(max, min);
    const v3 half_extent = tgm_v3_divf(extent, 2.0f);
    const v3 center = tgm_v3_add(min, half_extent);

    const m4 scale = tgm_m4_scale(extent);
    const m4 rotation = tgm_m4_identity();
    const m4 translation = tgm_m4_translate(center);
    const m4 transformation_matrix = tgm_m4_mul(tgm_m4_mul(translation, rotation), scale);

    tg_raytracer_push_debug_cuboid(p_raytracer, transformation_matrix);

    if (push_children)
    {
        const v3 child_extent = tgm_v3_divf(extent, 2.0f);
        u32 child_offset = 0;

        for (u32 child_idx = 0; child_idx < 8; child_idx++)
        {
            if (p_inner->valid_mask & (1 << child_idx))
            {
                const f32 dx = (f32)(child_idx % 2) * child_extent.x;
                const f32 dy = (f32)((child_idx / 2) % 2) * child_extent.y;
                const f32 dz = (f32)((child_idx / 4) % 2) * child_extent.z;

                const tg_svo_node* p_child = (tg_svo_node*)p_inner + p_inner->child_pointer + child_offset;
                const v3 child_min = { min.x + dx, min.y + dy, min.z + dz };
                const v3 child_max = tgm_v3_add(child_min, half_extent);

                tg__push_debug_svo_node(p_raytracer, &p_child->inner, child_min, child_max, (p_inner->leaf_mask & (1 << child_idx)) == 0);

                child_offset++;
            }
        }
    }
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

    VkDeviceSize voxel_data_ssbo_size = (VkDeviceSize)TGVK_MAX_VOXELS / 8; // One byte contains the solid value for 8 voxels
    const VkDeviceSize visibility_buffer_ssbo_size = sizeof(tg_visibility_buffer_ssbo) + ((VkDeviceSize)w * (VkDeviceSize)h * TG_SIZEOF_MEMBER(tg_visibility_buffer_ssbo, p_data[0]));

    p_raytracer->visibility_pass.raytracer_data_ubo = TGVK_BUFFER_CREATE_UBO(sizeof(tg_raytracer_data_ubo));
    p_raytracer->visibility_pass.p_voxel_data = TG_MALLOC(voxel_data_ssbo_size);
    p_raytracer->visibility_pass.svo_voxel_data_ssbo = TGVK_BUFFER_CREATE(voxel_data_ssbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    p_raytracer->visibility_pass.visibility_buffer_ssbo = TGVK_BUFFER_CREATE(visibility_buffer_ssbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    const VkDeviceSize staging_buffer_size = 2ui64 * sizeof(u32); // We only set the width and height of the visibility buffer
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);
    tg_visibility_buffer_ssbo* p_visibility_buffer = p_staging_buffer->memory.p_mapped_device_memory;
    p_visibility_buffer->w = w;
    p_visibility_buffer->h = h;
    tgvk_command_buffer_begin(&p_raytracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_copy_buffer(&p_raytracer->visibility_pass.command_buffer, staging_buffer_size, p_staging_buffer, &p_raytracer->visibility_pass.visibility_buffer_ssbo);
    tgvk_command_buffer_end_and_submit(&p_raytracer->visibility_pass.command_buffer);
    tgvk_global_staging_buffer_release();

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

    // They are generated on the fly as required
    p_raytracer->svo_pass.svo_ssbo                = (tgvk_buffer){ 0 };
    p_raytracer->svo_pass.svo_nodes_ssbo          = (tgvk_buffer){ 0 };
    p_raytracer->svo_pass.svo_leaf_node_data_ssbo = (tgvk_buffer){ 0 };
    p_raytracer->svo_pass.svo_voxel_data_ssbo     = (tgvk_buffer){ 0 };

    p_raytracer->svo_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->svo_pass.render_pass, 0, TG_NULL, w, h);
    p_raytracer->svo_pass.instance_id_vbo = p_raytracer->visibility_pass.instance_id_vbo; // TODO: for now!
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
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/shading.vert");
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
    p_raytracer->shading_pass.shading_data_ubo = TGVK_BUFFER_CREATE_UBO(sizeof(tg_shading_data_ubo));
    p_raytracer->shading_pass.color_lut_id_ssbo = TGVK_BUFFER_CREATE(TG_CEIL_TO_MULTIPLE(TGVK_MAX_VOXELS, 32), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    p_raytracer->shading_pass.color_lut_ssbo = TGVK_BUFFER_CREATE(256 * sizeof(u32), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    p_raytracer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->shading_pass.graphics_pipeline);
    //tgvk_atmosphere_model_update_descriptor_set(&p_raytracer->model, &p_raytracer->shading_pass.descriptor_set);
    const u32 atmosphere_binding_offset = 4;
    u32 binding_offset = atmosphere_binding_offset;
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer_ssbo, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->shading_pass.shading_data_ubo, binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->instance_data_ssbo, binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->shading_pass.color_lut_id_ssbo, binding_offset++);
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->shading_pass.color_lut_ssbo, binding_offset++);

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
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/debug.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/debug.frag");
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

    tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->instance_data_ssbo, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->view_projection_ubo, 1);

    p_raytracer->debug_pass.framebuffer = tgvk_framebuffer_create(p_raytracer->debug_pass.render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);



    // Debug cuboids

    p_raytracer->debug_pass.cb_capacity = (1 << 14);
    p_raytracer->debug_pass.cb_count = 0;

    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/debug_cuboid.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/debug_cuboid.frag");

    p_raytracer->debug_pass.cb_graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->debug_pass.cb_transformation_matrix_ssbo = TGVK_BUFFER_CREATE(p_raytracer->debug_pass.cb_capacity * sizeof(m4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_HOST);
    p_raytracer->debug_pass.cb_descriptor_set = tgvk_descriptor_set_create(&p_raytracer->debug_pass.cb_graphics_pipeline);

    tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.cb_descriptor_set.set, &p_raytracer->debug_pass.cb_transformation_matrix_ssbo, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.cb_descriptor_set.set, &p_raytracer->view_projection_ubo, 1);

    p_raytracer->debug_pass.cb_framebuffer = tgvk_framebuffer_create(p_raytracer->debug_pass.render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);
    p_raytracer->debug_pass.cb_instance_id_vbo = TGVK_BUFFER_CREATE(p_raytracer->debug_pass.cb_capacity * sizeof(u32), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_HOST);
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
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/renderer/screen_quad.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/renderer/present.frag");
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
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->clear_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer_ssbo, 0);

    tgvk_command_buffer_begin(&p_raytracer->clear_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->clear_pass.descriptor_set.set, 0, TG_NULL);
        vkCmdDispatch(p_raytracer->clear_pass.command_buffer.buffer, (w + 31) / 32, (h + 31) / 32, 1);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->clear_pass.command_buffer.buffer));
}



void tg_raytracer_create(const tg_camera* p_camera, u32 max_instance_count, TG_OUT tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_camera);
	TG_ASSERT(max_instance_count > 0);
	TG_ASSERT(max_instance_count <= TGVK_MAX_INSTANCES);
	TG_ASSERT(p_raytracer);

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->p_camera = p_camera;
    p_raytracer->render_target = TGVK_RENDER_TARGET_CREATE(
        w, h, TGVK_HDR_FORMAT, TG_NULL,
        w, h, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );

    p_raytracer->semaphore = tgvk_semaphore_create();

    const u16 p_cube_indices[6 * 6] = {
         0,  1,  2,  2,  3,  0, // x-
         4,  5,  6,  6,  7,  4, // x+
         8,  9, 10, 10, 11,  8, // y-
        12, 13, 14, 14, 15, 12, // y+
        16, 17, 18, 18, 19, 16, // z-
        20, 21, 22, 22, 23, 20  // z+
    };

    const v3 p_cube_positions[6 * 4] = {
        (v3){ -0.5f, -0.5f, -0.5f }, // x-
        (v3){ -0.5f, -0.5f,  0.5f },
        (v3){ -0.5f,  0.5f,  0.5f },
        (v3){ -0.5f,  0.5f, -0.5f },
        (v3){  0.5f, -0.5f, -0.5f }, // x+
        (v3){  0.5f,  0.5f, -0.5f },
        (v3){  0.5f,  0.5f,  0.5f },
        (v3){  0.5f, -0.5f,  0.5f },
        (v3){ -0.5f, -0.5f, -0.5f }, // y-
        (v3){  0.5f, -0.5f, -0.5f },
        (v3){  0.5f, -0.5f,  0.5f },
        (v3){ -0.5f, -0.5f,  0.5f },
        (v3){ -0.5f,  0.5f, -0.5f }, // y+
        (v3){ -0.5f,  0.5f,  0.5f },
        (v3){  0.5f,  0.5f,  0.5f },
        (v3){  0.5f,  0.5f, -0.5f },
        (v3){ -0.5f, -0.5f, -0.5f }, // z-
        (v3){ -0.5f,  0.5f, -0.5f },
        (v3){  0.5f,  0.5f, -0.5f },
        (v3){  0.5f, -0.5f, -0.5f },
        (v3){ -0.5f, -0.5f,  0.5f }, // z+
        (v3){  0.5f, -0.5f,  0.5f },
        (v3){  0.5f,  0.5f,  0.5f },
        (v3){ -0.5f,  0.5f,  0.5f }
    };

    // TODO: do all of the copies at once!

    const tg_size instance_id_vbo_size = max_instance_count * sizeof(u32);

    const tg_size staging_buffer_size = TG_MAX3(
        sizeof(p_cube_indices),
        sizeof(p_cube_positions),
        instance_id_vbo_size);
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);

    p_raytracer->visibility_pass.instance_id_vbo = TGVK_BUFFER_CREATE_VBO(instance_id_vbo_size);
    u32* p_it = (u32*)p_staging_buffer->memory.p_mapped_device_memory;
    for (u32 i = 0; i < max_instance_count; i++)
    {
        *p_it++ = i;
    }
    tgvk_buffer_copy(instance_id_vbo_size, p_staging_buffer, &p_raytracer->visibility_pass.instance_id_vbo);

    tg_memcpy(sizeof(p_cube_indices), p_cube_indices, p_staging_buffer->memory.p_mapped_device_memory);
    p_raytracer->cube_ibo = TGVK_BUFFER_CREATE_IBO(sizeof(p_cube_indices));
    tgvk_buffer_copy(sizeof(p_cube_indices), p_staging_buffer, &p_raytracer->cube_ibo);

    tg_memcpy(sizeof(p_cube_positions), p_cube_positions, p_staging_buffer->memory.p_mapped_device_memory);
    p_raytracer->cube_vbo = TGVK_BUFFER_CREATE_VBO(sizeof(p_cube_positions));
    tgvk_buffer_copy(sizeof(p_cube_positions), p_staging_buffer, &p_raytracer->cube_vbo);

    tgvk_global_staging_buffer_release();

    const VkDeviceSize instance_data_ssbo_size = max_instance_count * sizeof(tg_instance_data_ssbo);
    p_raytracer->instance_data_ssbo = TGVK_BUFFER_CREATE(instance_data_ssbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);



    p_raytracer->view_projection_ubo = TGVK_BUFFER_CREATE_UBO(sizeof(tg_view_projection_ubo));



    p_raytracer->instance_buffer.capacity = max_instance_count;
    p_raytracer->instance_buffer.count = 0;
    p_raytracer->instance_buffer.p_instances = TG_MALLOC(max_instance_count * sizeof(*p_raytracer->instance_buffer.p_instances));

    tg__init_visibility_pass(p_raytracer);
    tg__init_svo_pass(p_raytracer);
    tg__init_shading_pass(p_raytracer);
    tg__init_debug_pass(p_raytracer);
    tg__init_blit_pass(p_raytracer);
    tg__init_present_pass(p_raytracer);
    tg__init_clear_pass(p_raytracer);
}

void tg_raytracer_destroy(tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_raytracer);

	TG_NOT_IMPLEMENTED();
}

void tg_raytracer_create_instance(tg_raytracer* p_raytracer, f32 center_x, f32 center_y, f32 center_z, u32 grid_width, u32 grid_height, u32 grid_depth)
{
    TG_ASSERT(p_raytracer);
    TG_ASSERT(tgm_u32_count_set_bits(grid_width) == 1);
    TG_ASSERT(tgm_u32_count_set_bits(grid_height) == 1);
    TG_ASSERT(tgm_u32_count_set_bits(grid_depth) == 1);
    TG_ASSERT(TGVK_STORED_NUM_VOXELS(grid_width, grid_height, grid_depth) <= TGVK_MAX_VOXELS);
    TG_ASSERT(p_raytracer->instance_buffer.count < p_raytracer->instance_buffer.capacity);

    const u32 instance_id = p_raytracer->instance_buffer.count++;
    tg_instance* p_instance = &p_raytracer->instance_buffer.p_instances[instance_id];
    p_instance->half_extent = (v3u){ grid_width / 2, grid_height / 2, grid_depth / 2 };
    p_instance->translation = tgm_m4_translate((v3) { center_x, center_y, center_z });
    p_instance->rotation = tgm_m4_rotate_y(TG_DEG2RAD((f32)(instance_id * 7)));

    if (instance_id == 0)
    {
        p_instance->first_voxel_id = 0;
    }
    else
    {
        const tg_instance* p_prev_instance = p_instance - 1;
        const u32 mask = (1 << 5) - 1;
        const u32 prev_log2_w =  p_prev_instance->packed_log2_whd        & mask;
        const u32 prev_log2_h = (p_prev_instance->packed_log2_whd >>  5) & mask;
        const u32 prev_log2_d = (p_prev_instance->packed_log2_whd >> 10) & mask;
        const u32 prev_w = 1 << prev_log2_w;
        const u32 prev_h = 1 << prev_log2_h;
        const u32 prev_d = 1 << prev_log2_d;
        const u32 prev_stored_num_voxels = TGVK_STORED_NUM_VOXELS(prev_w, prev_h, prev_d);
        p_instance->first_voxel_id = p_prev_instance->first_voxel_id + prev_stored_num_voxels;
    }

    const VkDeviceSize instance_data_ubo_staging_buffer_size = sizeof(tg_instance_data_ssbo);
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(instance_data_ubo_staging_buffer_size);

    tg_instance_data_ssbo* p_instance_data = p_staging_buffer->memory.p_mapped_device_memory;
    p_instance_data->s = tgm_m4_scale((v3) { (f32)grid_width, (f32)grid_height, (f32)grid_depth });
    p_instance_data->r = p_instance->rotation;
    p_instance_data->t = p_instance->translation;
    p_instance_data->w = grid_width;
    p_instance_data->h = grid_height;
    p_instance_data->d = grid_depth;
    p_instance_data->first_voxel_id = p_instance->first_voxel_id;

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_copy_buffer2(p_command_buffer, 0, (VkDeviceSize)instance_id * sizeof(tg_instance_data_ssbo), instance_data_ubo_staging_buffer_size, p_staging_buffer, &p_raytracer->instance_data_ssbo);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tgvk_global_staging_buffer_release();

    const u32 packed = tgm_u32_count_zero_bits_from_right(grid_width)
        | (tgm_u32_count_zero_bits_from_right(grid_height) << 5)
        | (tgm_u32_count_zero_bits_from_right(grid_depth) << 10);
    p_instance->packed_log2_whd = (u16)packed;

    // TODO: gen on GPU
    const tg_size staging_buffer_size = tgvk_global_stating_buffer_size();
    p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);
    p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);

    tg_size staged_size;
    u32 staging_iteration_idx;



    // SOLID VOXEL BITMAP

    u32* p_voxel_bitmap_it = p_staging_buffer->memory.p_mapped_device_memory;
    u32 voxel_bitmap = 0;
    u8 voxel_bitmap_idx = 0;
    staged_size = 0;
    staging_iteration_idx = 0;
    for (u32 z = 0; z < grid_depth; z++)
    {
        for (u32 y = 0; y < grid_height; y++)
        {
            for (u32 x = 0; x < grid_width; x++)
            {
                const f32 xf = (f32)x;
                const f32 yf = (f32)y;
                const f32 zf = (f32)z;

                const f32 n_hills0 = tgm_noise(xf * 0.008f, 0.0f, zf * 0.008f);
                const f32 n_hills1 = tgm_noise(xf * 0.2f, 0.0f, zf * 0.2f);
                const f32 n_hills = n_hills0 + 0.005f * n_hills1;
                
                const f32 s_caves = 0.06f;
                const f32 c_caves_x = s_caves * xf;
                const f32 c_caves_y = s_caves * yf;
                const f32 c_caves_z = s_caves * zf;
                const f32 unclamped_noise_caves = tgm_noise(c_caves_x, c_caves_y, c_caves_z);
                const f32 n_caves = tgm_f32_clamp(unclamped_noise_caves, -1.0f, 0.0f);
                
                const f32 n = n_hills;
                f32 noise = (n * 64.0f) - (f32)(y - 8.0f);
                noise += 10.0f * n_caves;
                const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
                const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
                const f32 f1 = 254.0f * f0;
                const i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);
                const b32 solid = f2 <= 0 || y == 0;

                voxel_bitmap |= solid << voxel_bitmap_idx;
                voxel_bitmap_idx++;

                // TODO: space filling z curve
                if (voxel_bitmap_idx == 32)
                {
                    *p_voxel_bitmap_it++ = voxel_bitmap;
                    voxel_bitmap = 0;
                    voxel_bitmap_idx = 0;
                    staged_size += sizeof(u32);

                    if (staged_size == staging_buffer_size)
                    {
                        const tg_size dst_offset = p_instance->first_voxel_id / 8 + staging_iteration_idx * staging_buffer_size;
                        tg_memcpy(staged_size, p_staging_buffer->memory.p_mapped_device_memory, ((u8*)p_raytracer->visibility_pass.p_voxel_data) + dst_offset);
                        tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                        tgvk_cmd_copy_buffer2(p_command_buffer, 0, dst_offset, staged_size, p_staging_buffer, &p_raytracer->visibility_pass.svo_voxel_data_ssbo);
                        tgvk_command_buffer_end_and_submit(p_command_buffer);
                        p_voxel_bitmap_it = p_staging_buffer->memory.p_mapped_device_memory;
                        staged_size = 0;
                        staging_iteration_idx++;
                    }
                }
            }
        }
    }

    // Transfer remaining bits
    if (voxel_bitmap_idx > 0)
    {
        *p_voxel_bitmap_it++ = voxel_bitmap;
        voxel_bitmap = 0;
        voxel_bitmap_idx = 0;
        staged_size += sizeof(u32);
    }
    if (staged_size > 0)
    {
        const tg_size dst_offset = p_instance->first_voxel_id / 8 + staging_iteration_idx * staging_buffer_size;
        tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        tgvk_cmd_copy_buffer2(p_command_buffer, 0, dst_offset, staged_size, p_staging_buffer, &p_raytracer->visibility_pass.svo_voxel_data_ssbo);
        tgvk_command_buffer_end_and_submit(p_command_buffer);
        staged_size = 0;
        staging_iteration_idx++;
    }



    // COLOR LUT ID

    u32* p_color_lut_id_it = p_staging_buffer->memory.p_mapped_device_memory;
    u32 packed_color_lut_ids = 0;
    u32 packed_color_lut_id_idx = 0;
    staged_size = 0;
    staging_iteration_idx = 0;
    for (u32 z = 0; z < grid_depth; z++)
    {
        for (u32 y = 0; y < grid_height; y++)
        {
            for (u32 x = 0; x < grid_width; x++)
            {
                const u8 color_lut_id = (u8)(x % 256);
                packed_color_lut_ids |= color_lut_id << (packed_color_lut_id_idx * 8);
                packed_color_lut_id_idx++;

                if (packed_color_lut_id_idx == 4)
                {
                    *p_color_lut_id_it++ = packed_color_lut_ids;
                    packed_color_lut_ids = 0;
                    packed_color_lut_id_idx = 0;
                    staged_size += sizeof(u32);

                    if (staged_size == staging_buffer_size)
                    {
                        const tg_size dst_offset = p_instance->first_voxel_id * sizeof(u32) / 4 + staging_iteration_idx * staging_buffer_size;
                        tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                        tgvk_cmd_copy_buffer2(p_command_buffer, 0, dst_offset, staged_size, p_staging_buffer, &p_raytracer->shading_pass.color_lut_id_ssbo);
                        tgvk_command_buffer_end_and_submit(p_command_buffer);
                        p_color_lut_id_it = p_staging_buffer->memory.p_mapped_device_memory;
                        staged_size = 0;
                        staging_iteration_idx++;
                    }
                }
            }
        }
    }
    TG_ASSERT(staged_size == 0);

    tgvk_global_staging_buffer_release();

    // TODO: lods
}

void tg_raytracer_push_debug_cuboid(tg_raytracer* p_raytracer, m4 transformation_matrix)
{
    // TODO: color
    TG_ASSERT(p_raytracer);
    TG_ASSERT(p_raytracer->debug_pass.cb_count < p_raytracer->debug_pass.cb_capacity);

    m4* p_transformation_matrices = p_raytracer->debug_pass.cb_transformation_matrix_ssbo.memory.p_mapped_device_memory;
    p_transformation_matrices[p_raytracer->debug_pass.cb_count] = transformation_matrix;

    // TODO: this can be used from the other id buffer and if not possible later for destruction of objects reasons, this can be created only once and reused.
    u32* p_instance_ids = p_raytracer->debug_pass.cb_instance_id_vbo.memory.p_mapped_device_memory;
    p_instance_ids[p_raytracer->debug_pass.cb_count] = p_raytracer->debug_pass.cb_count;

    p_raytracer->debug_pass.cb_count++;
}

void tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b)
{
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(sizeof(u32));

    const u32 r_u32 = (u32)(r * 255.0f);
    const u32 g_u32 = (u32)(g * 255.0f);
    const u32 b_u32 = (u32)(b * 255.0f);
    const u32 a_u32 = 255; // TODO: Metallic/Roughness?
    const u32 color = r_u32 << 24 | g_u32 << 16 | b_u32 << 8 | a_u32;
    *(u32*)p_staging_buffer->memory.p_mapped_device_memory = color;

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_copy_buffer2(p_command_buffer, 0, index * sizeof(u32), sizeof(u32), p_staging_buffer, &p_raytracer->shading_pass.color_lut_ssbo);
    tgvk_command_buffer_end_and_submit(p_command_buffer);

    tgvk_global_staging_buffer_release();
}

void tg_raytracer_render(tg_raytracer* p_raytracer)
{
    TG_ASSERT(p_raytracer);

    const tg_camera c = *p_raytracer->p_camera;

    const m4 r = tgm_m4_inverse(tgm_m4_euler(c.pitch, c.yaw, c.roll));
    const m4 t = tgm_m4_translate(tgm_v3_neg(c.position));
    const m4 v = tgm_m4_mul(r, t);
    const m4 iv = tgm_m4_inverse(v);
    const m4 p = c.type == TG_CAMERA_TYPE_ORTHOGRAPHIC ? tgm_m4_orthographic(c.ortho.l, c.ortho.r, c.ortho.b, c.ortho.t, c.ortho.f, c.ortho.n) : tgm_m4_perspective(c.persp.fov_y_in_radians, c.persp.aspect, c.persp.n, c.persp.f);
    const m4 ip = tgm_m4_inverse(p);
    const m4 vp = tgm_m4_mul(p, v);
    const m4 ivp = tgm_m4_inverse(vp);

    tg_view_projection_ubo* p_view_projection_ubo = p_raytracer->view_projection_ubo.memory.p_mapped_device_memory;
    p_view_projection_ubo->v = v;
    p_view_projection_ubo->p = p;

    const m4 ivp_no_translation = tgm_m4_inverse(tgm_m4_mul(p, r));
    const v3 ray00 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray10 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f, -1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray01 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    const v3 ray11 = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f, -1.0f, 1.0f, 1.0f }).xyz);

    tg_raytracer_data_ubo* p_raytracer_data_ubo = p_raytracer->visibility_pass.raytracer_data_ubo.memory.p_mapped_device_memory;
    p_raytracer_data_ubo->camera.xyz = c.position;
    p_raytracer_data_ubo->ray00.xyz = ray00;
    p_raytracer_data_ubo->ray10.xyz = ray10;
    p_raytracer_data_ubo->ray01.xyz = ray01;
    p_raytracer_data_ubo->ray11.xyz = ray11;
    p_raytracer_data_ubo->near = c.persp.n;
    p_raytracer_data_ubo->far = c.persp.f;

    // TODO: calculated twice!
    tg_shading_data_ubo* p_shading_ubo = p_raytracer->shading_pass.shading_data_ubo.memory.p_mapped_device_memory;
    p_shading_ubo->camera.xyz = c.position;
    p_shading_ubo->ray00.xyz = ray00;
    p_shading_ubo->ray10.xyz = ray10;
    p_shading_ubo->ray01.xyz = ray01;
    p_shading_ubo->ray11.xyz = ray11;
    p_shading_ubo->sun_direction.xyz = (v3){ 0.0f, -1.0f, 0.0f };
    p_shading_ubo->near = c.persp.n;
    p_shading_ubo->far = c.persp.f;
    //p_shading_ubo->camera_position.xyz = c.position;
    //p_shading_ubo->ivp = ivp;

    // BVH

    tg_svo* p_svo = &p_raytracer->svo;
    if (p_svo->p_node_buffer == TG_NULL)
    {
        // TODO: update BVH here, parallel to visibility buffer computation
        // https://www.nvidia.com/docs/IO/88972/nvr-2010-001.pdf
        // https://luebke.us/publications/eg09.pdf
        const v3 extent_min = { -512.0f, -512.0f, -512.0f };
        const v3 extent_max = {  512.0f,  512.0f,  512.0f };
        tg_svo_create(extent_min, extent_max, p_raytracer->instance_buffer.count, p_raytracer->instance_buffer.p_instances, p_raytracer->visibility_pass.p_voxel_data, p_svo);
        //tg_svo_destroy(&svo);

        const tg_size svo_ssbo_size                = 2 * sizeof(v4);
        const tg_size svo_nodes_ssbo_size          = p_raytracer->svo.node_buffer_capacity           * sizeof(*p_raytracer->svo.p_node_buffer);
        const tg_size svo_leaf_node_data_ssbo_size = p_raytracer->svo.leaf_node_data_buffer_capacity * sizeof(*p_raytracer->svo.p_leaf_node_data_buffer);
        const tg_size svo_voxel_data_ssbo_size     = p_raytracer->svo.voxel_buffer_capacity_in_u32   * sizeof(*p_raytracer->svo.p_voxels_buffer);
        const VkBufferUsageFlags ssbo_usage_flags  = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        p_raytracer->svo_pass.svo_ssbo                = TGVK_BUFFER_CREATE(svo_ssbo_size,                ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->svo_pass.svo_nodes_ssbo          = TGVK_BUFFER_CREATE(svo_nodes_ssbo_size,          ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->svo_pass.svo_leaf_node_data_ssbo = TGVK_BUFFER_CREATE(svo_leaf_node_data_ssbo_size, ssbo_usage_flags, TGVK_MEMORY_DEVICE);
        p_raytracer->svo_pass.svo_voxel_data_ssbo     = TGVK_BUFFER_CREATE(svo_voxel_data_ssbo_size,     ssbo_usage_flags, TGVK_MEMORY_DEVICE);

        v4 p_svo_min_max[2] = { 0 };
        p_svo_min_max[0].xyz = p_svo->min;
        p_svo_min_max[1].xyz = p_svo->max;

        tgvk_copy_to_buffer(2 * sizeof(v4),               p_svo_min_max,                  &p_raytracer->svo_pass.svo_ssbo);
        tgvk_copy_to_buffer(svo_nodes_ssbo_size,          p_svo->p_node_buffer,           &p_raytracer->svo_pass.svo_nodes_ssbo);
        tgvk_copy_to_buffer(svo_leaf_node_data_ssbo_size, p_svo->p_leaf_node_data_buffer, &p_raytracer->svo_pass.svo_leaf_node_data_ssbo);
        tgvk_copy_to_buffer(svo_voxel_data_ssbo_size,     p_svo->p_voxels_buffer,         &p_raytracer->svo_pass.svo_voxel_data_ssbo);
    }
    tg__push_debug_svo_node(p_raytracer, &p_svo->p_node_buffer[0].inner, p_svo->min, p_svo->max, TG_TRUE);

    // TODO: keep for a while to catch potential errors of implementation
    f32  distance0, distance1, distance2, distance3;
    u32  node_idx0, node_idx1, node_idx2, node_idx3;
    u32 voxel_idx0, voxel_idx1, voxel_idx2, voxel_idx3;
    const b32 result0 = tg_svo_traverse(p_svo, c.position, ray00, &distance0, &node_idx0, &voxel_idx0);
    const b32 result1 = tg_svo_traverse(p_svo, c.position, ray10, &distance1, &node_idx1, &voxel_idx1);
    const b32 result2 = tg_svo_traverse(p_svo, c.position, ray01, &distance2, &node_idx2, &voxel_idx2);
    const b32 result3 = tg_svo_traverse(p_svo, c.position, ray11, &distance3, &node_idx3, &voxel_idx3);


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
        vkCmdBindVertexBuffers(p_raytracer->svo_pass.command_buffer.buffer, 0, 1, &p_raytracer->svo_pass.instance_id_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->svo_pass.command_buffer.buffer, 1, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);

        // TODO: update required here? Otherwise mark as dirty. Also, draw indirect
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->view_projection_ubo, 0);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->visibility_pass.raytracer_data_ubo, 1);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer_ssbo, 2);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->svo_pass.svo_ssbo, 3);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->svo_pass.svo_nodes_ssbo, 4);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->svo_pass.svo_leaf_node_data_ssbo, 5);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->svo_pass.descriptor_set.set, &p_raytracer->svo_pass.svo_voxel_data_ssbo, 6);

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
        vkCmdBindIndexBuffer(p_raytracer->visibility_pass.command_buffer.buffer, p_raytracer->cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 0, 1, &p_raytracer->visibility_pass.instance_id_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 1, 1, &p_raytracer->cube_vbo.buffer, &vertex_buffer_offset);

        // TODO: update required here? Otherwise mark as dirty. Also, draw indirect
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->instance_data_ssbo, 0);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->view_projection_ubo, 1);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->visibility_pass.raytracer_data_ubo, 2);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->visibility_pass.svo_voxel_data_ssbo, 3);
        tgvk_descriptor_set_update_storage_buffer(p_raytracer->visibility_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer_ssbo, 4);

        vkCmdBindDescriptorSets(p_raytracer->visibility_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->visibility_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->visibility_pass.descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_draw_indexed_instanced(&p_raytracer->visibility_pass.command_buffer, 6 * 6, p_raytracer->instance_buffer.count); // TODO: triangle fans for fewer indices?

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

    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->shading_pass.color_lut_id_ssbo, 7);
    tgvk_command_buffer_begin(&p_raytracer->shading_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->shading_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->shading_pass.graphics_pipeline.pipeline);

        vkCmdBindIndexBuffer(p_raytracer->shading_pass.command_buffer.buffer, screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 0, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 1, 1, &screen_quad_uvs_vbo.buffer, &vertex_buffer_offset);
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

    if (0)
    {
        tgvk_command_buffer_begin(&p_raytracer->debug_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
        tgvk_cmd_begin_render_pass(&p_raytracer->debug_pass.command_buffer, p_raytracer->debug_pass.render_pass, &p_raytracer->debug_pass.cb_framebuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.cb_graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(p_raytracer->debug_pass.command_buffer.buffer, p_raytracer->cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 0, 1, &p_raytracer->debug_pass.cb_instance_id_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 1, 1, &p_raytracer->cube_vbo.buffer, &vertex_buffer_offset);

        tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.cb_descriptor_set.set, &p_raytracer->debug_pass.cb_transformation_matrix_ssbo, 0);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.cb_descriptor_set.set, &p_raytracer->view_projection_ubo, 1);

        vkCmdBindDescriptorSets(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.cb_graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->debug_pass.cb_descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_draw_indexed_instanced(&p_raytracer->debug_pass.command_buffer, 6 * 6, p_raytracer->debug_pass.cb_count); // TODO: triangle fans for fewer indices? same above

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

    if (1)
    {
        vkDeviceWaitIdle(device);

        tgvk_command_buffer_begin(&p_raytracer->debug_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
        tgvk_cmd_begin_render_pass(&p_raytracer->debug_pass.command_buffer, p_raytracer->debug_pass.render_pass, &p_raytracer->debug_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(p_raytracer->debug_pass.command_buffer.buffer, p_raytracer->cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 0, 1, &p_raytracer->visibility_pass.instance_id_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->debug_pass.command_buffer.buffer, 1, 1, &p_raytracer->cube_vbo.buffer, &vertex_buffer_offset);

        tgvk_descriptor_set_update_storage_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->instance_data_ssbo, 0);
        tgvk_descriptor_set_update_uniform_buffer(p_raytracer->debug_pass.descriptor_set.set, &p_raytracer->view_projection_ubo, 1);

        vkCmdBindDescriptorSets(p_raytracer->debug_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->debug_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->debug_pass.descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_draw_indexed_instanced(&p_raytracer->debug_pass.command_buffer, 6 * 6, p_raytracer->instance_buffer.count); // TODO: triangle fans for fewer indices? same above

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

    p_raytracer->debug_pass.cb_count = 0;

    // TODO: forward renderer
    //tg_shading_data_ubo* p_shading_ubo = p_raytracer->shading_pass.shading_data_ubo.memory.p_mapped_device_memory;

    // TODO: lighting
    //p_shading_ubo->directional_light_count = 0;
    //p_shading_ubo->point_light_count = 0;
}

#endif
