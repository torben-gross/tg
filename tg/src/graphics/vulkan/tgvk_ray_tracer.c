#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN



#define TGVK_CAMERA_VIEW(view_projection_ubo)    (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[0])
#define TGVK_CAMERA_PROJ(view_projection_ubo)    (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[1])
#define TGVK_HDR_FORMAT                          VK_FORMAT_R32G32B32A32_SFLOAT
#define TGVK_SHADING_UBO                         (*(tg_shading_ubo*)h_ray_tracer->shading_pass.ubo.memory.p_mapped_device_memory)



typedef struct tg_ray_tracing_ubo
{
    v4    camera;
    v4    ray00;
    v4    ray10;
    v4    ray01;
    v4    ray11;
} tg_ray_tracing_ubo;

typedef struct tg_shading_ubo
{
    //v4     camera_position;
    //v4     sun_direction;
    //u32    directional_light_count;
    //u32    point_light_count; u32 pad[2];
    m4     ivp; // inverse view projection
    //v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    //v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    //v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_shading_ubo;



static void tg__init_visibility_pass(tg_ray_tracer_h h_ray_tracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    h_ray_tracer->visibility_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // TODO shared?
    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/ray_tracer/visibility.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_create("shaders/ray_tracer/visibility.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_FRONT_BIT; // TODO: is this okay for ray tracing? this culls the front face
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.ray_tracer.visibility_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_ray_tracer->visibility_pass.pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_ray_tracer->visibility_pass.view_projection_ubo = TGVK_UNIFORM_BUFFER_CREATE(2ui64 * sizeof(m4));
    h_ray_tracer->visibility_pass.ray_tracing_ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(tg_ray_tracing_ubo));

    const VkDeviceSize staging_buffer_size = 2ui64 * sizeof(u32);
    const VkDeviceSize visibility_buffer_size = staging_buffer_size + ((VkDeviceSize)w * (VkDeviceSize)h * sizeof(u64));
    h_ray_tracer->visibility_pass.visibility_buffer = TGVK_BUFFER_CREATE(visibility_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);
    ((u32*)p_staging_buffer->memory.p_mapped_device_memory)[0] = w;
    ((u32*)p_staging_buffer->memory.p_mapped_device_memory)[1] = h;
    tgvk_command_buffer_begin(&h_ray_tracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_copy_buffer(&h_ray_tracer->visibility_pass.command_buffer, staging_buffer_size, p_staging_buffer, &h_ray_tracer->visibility_pass.visibility_buffer);
    tgvk_command_buffer_end_and_submit(&h_ray_tracer->visibility_pass.command_buffer);
    tgvk_global_staging_buffer_release();

    h_ray_tracer->visibility_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.ray_tracer.visibility_render_pass, 0, TG_NULL, w, h);
}

static void tg__init_shading_pass(tg_ray_tracer_h h_ray_tracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    h_ray_tracer->shading_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/ray_tracer/shading.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_vertex_shader_create("shaders/ray_tracer/shading.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.shading_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_ray_tracer->shading_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_ray_tracer->shading_pass.ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(tg_shading_ubo));

    h_ray_tracer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&h_ray_tracer->shading_pass.graphics_pipeline);
    //tgvk_atmosphere_model_update_descriptor_set(&h_ray_tracer->model, &h_ray_tracer->shading_pass.descriptor_set);
    const u32 atmosphere_binding_offset = 4;
    u32 binding_offset = atmosphere_binding_offset;
    tgvk_descriptor_set_update_storage_buffer(h_ray_tracer->shading_pass.descriptor_set.descriptor_set, &h_ray_tracer->visibility_pass.visibility_buffer, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(h_ray_tracer->shading_pass.descriptor_set.descriptor_set, &h_ray_tracer->shading_pass.ubo, binding_offset++);

    // TODO: tone mapping and other passes...
    //h_ray_tracer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, 1, &h_ray_tracer->hdr_color_attachment.image_view, w, h);
    h_ray_tracer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, 1, &h_ray_tracer->render_target.color_attachment.image_view, w, h);

    tgvk_command_buffer_begin(&h_ray_tracer->shading_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(h_ray_tracer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(h_ray_tracer->shading_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_ray_tracer->shading_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_ray_tracer->shading_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_ray_tracer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->shading_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_ray_tracer->shading_pass.descriptor_set.descriptor_set, 0, TG_NULL);

        tgvk_cmd_begin_render_pass(&h_ray_tracer->shading_pass.command_buffer, shared_render_resources.shading_render_pass, &h_ray_tracer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        tgvk_cmd_draw_indexed(&h_ray_tracer->shading_pass.command_buffer, 6);
        vkCmdEndRenderPass(h_ray_tracer->shading_pass.command_buffer.command_buffer);
    }
    TGVK_CALL(vkEndCommandBuffer(h_ray_tracer->shading_pass.command_buffer.command_buffer));
}

static void tg__init_blit_pass(tg_ray_tracer_h h_ray_tracer)
{
    h_ray_tracer->blit_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_command_buffer_begin(&h_ray_tracer->blit_pass.command_buffer, 0);
    {
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.color_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.depth_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);

        VkImageBlit color_image_blit = { 0 };
        color_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.srcSubresource.mipLevel = 0;
        color_image_blit.srcSubresource.baseArrayLayer = 0;
        color_image_blit.srcSubresource.layerCount = 1;
        color_image_blit.srcOffsets[0].x = 0;
        color_image_blit.srcOffsets[0].y = 0;
        color_image_blit.srcOffsets[0].z = 0;
        color_image_blit.srcOffsets[1].x = h_ray_tracer->render_target.color_attachment.width;
        color_image_blit.srcOffsets[1].y = h_ray_tracer->render_target.color_attachment.height;
        color_image_blit.srcOffsets[1].z = 1;
        color_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.dstSubresource.mipLevel = 0;
        color_image_blit.dstSubresource.baseArrayLayer = 0;
        color_image_blit.dstSubresource.layerCount = 1;
        color_image_blit.dstOffsets[0].x = 0;
        color_image_blit.dstOffsets[0].y = 0;
        color_image_blit.dstOffsets[0].z = 0;
        color_image_blit.dstOffsets[1].x = h_ray_tracer->render_target.color_attachment.width;
        color_image_blit.dstOffsets[1].y = h_ray_tracer->render_target.color_attachment.height;
        color_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.color_attachment, &h_ray_tracer->render_target.color_attachment_copy, &color_image_blit);

        VkImageBlit depth_image_blit = { 0 };
        depth_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.srcSubresource.mipLevel = 0;
        depth_image_blit.srcSubresource.baseArrayLayer = 0;
        depth_image_blit.srcSubresource.layerCount = 1;
        depth_image_blit.srcOffsets[0].x = 0;
        depth_image_blit.srcOffsets[0].y = 0;
        depth_image_blit.srcOffsets[0].z = 0;
        depth_image_blit.srcOffsets[1].x = h_ray_tracer->render_target.depth_attachment.width;
        depth_image_blit.srcOffsets[1].y = h_ray_tracer->render_target.depth_attachment.height;
        depth_image_blit.srcOffsets[1].z = 1;
        depth_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.dstSubresource.mipLevel = 0;
        depth_image_blit.dstSubresource.baseArrayLayer = 0;
        depth_image_blit.dstSubresource.layerCount = 1;
        depth_image_blit.dstOffsets[0].x = 0;
        depth_image_blit.dstOffsets[0].y = 0;
        depth_image_blit.dstOffsets[0].z = 0;
        depth_image_blit.dstOffsets[1].x = h_ray_tracer->render_target.depth_attachment.width;
        depth_image_blit.dstOffsets[1].y = h_ray_tracer->render_target.depth_attachment.height;
        depth_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.depth_attachment, &h_ray_tracer->render_target.depth_attachment_copy, &depth_image_blit);

        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.color_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&h_ray_tracer->blit_pass.command_buffer, &h_ray_tracer->render_target.depth_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
    }
    TGVK_CALL(vkEndCommandBuffer(h_ray_tracer->blit_pass.command_buffer.command_buffer));
}

static void tg__init_present_pass(tg_ray_tracer_h h_ray_tracer)
{
    tgvk_command_buffers_create(TGVK_COMMAND_POOL_TYPE_PRESENT, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_MAX_SWAPCHAIN_IMAGES, h_ray_tracer->present_pass.p_command_buffers);
    h_ray_tracer->present_pass.image_acquired_semaphore = tgvk_semaphore_create();

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        h_ray_tracer->present_pass.p_framebuffers[i] = tgvk_framebuffer_create(shared_render_resources.present_render_pass, 1, &p_swapchain_image_views[i], w, h);
    }

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/renderer/screen_quad.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_create("shaders/renderer/present.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.present_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_ray_tracer->present_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_ray_tracer->present_pass.descriptor_set = tgvk_descriptor_set_create(&h_ray_tracer->present_pass.graphics_pipeline);

    tgvk_descriptor_set_update_image(h_ray_tracer->present_pass.descriptor_set.descriptor_set, &h_ray_tracer->render_target.color_attachment, 0);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        tgvk_command_buffer_begin(&h_ray_tracer->present_pass.p_command_buffers[i], 0);
        {
            tgvk_cmd_transition_image_layout(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_F);

            vkCmdBindPipeline(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->present_pass.graphics_pipeline.pipeline);
            vkCmdBindIndexBuffer(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
            const VkDeviceSize vertex_buffer_offset = 0;
            vkCmdBindVertexBuffers(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindDescriptorSets(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->present_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_ray_tracer->present_pass.descriptor_set.descriptor_set, 0, TG_NULL);

            tgvk_cmd_begin_render_pass(&h_ray_tracer->present_pass.p_command_buffers[i], shared_render_resources.present_render_pass, &h_ray_tracer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            tgvk_cmd_draw_indexed(&h_ray_tracer->present_pass.p_command_buffers[i], 6);
            vkCmdEndRenderPass(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer);

            tgvk_cmd_transition_image_layout(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.color_attachment, TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_TRANSFER_WRITE);
            tgvk_cmd_transition_image_layout(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_WRITE);

            tgvk_cmd_clear_image(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.color_attachment);
            tgvk_cmd_clear_image(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.depth_attachment);

            tgvk_cmd_transition_image_layout(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
            tgvk_cmd_transition_image_layout(&h_ray_tracer->present_pass.p_command_buffers[i], &h_ray_tracer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        }
        TGVK_CALL(vkEndCommandBuffer(h_ray_tracer->present_pass.p_command_buffers[i].command_buffer));
    }
}

static void tg__init_clear_pass(tg_ray_tracer_h h_ray_tracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    h_ray_tracer->clear_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    h_ray_tracer->clear_pass.compute_pipeline = tgvk_pipeline_create_compute(&tg_compute_shader_create("shaders/ray_tracer/clear.comp")->shader);
    h_ray_tracer->clear_pass.descriptor_set = tgvk_descriptor_set_create(&h_ray_tracer->clear_pass.compute_pipeline);
    tgvk_descriptor_set_update_storage_buffer(h_ray_tracer->clear_pass.descriptor_set.descriptor_set, &h_ray_tracer->visibility_pass.visibility_buffer, 0);

    tgvk_command_buffer_begin(&h_ray_tracer->clear_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(h_ray_tracer->clear_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_ray_tracer->clear_pass.compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_ray_tracer->clear_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_ray_tracer->clear_pass.compute_pipeline.layout.pipeline_layout, 0, 1, &h_ray_tracer->clear_pass.descriptor_set.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(h_ray_tracer->clear_pass.command_buffer.command_buffer, (w + 31) / 32, (h + 31) / 32, 1);
    }
    TGVK_CALL(vkEndCommandBuffer(h_ray_tracer->clear_pass.command_buffer.command_buffer));
}



tg_ray_tracer_h tg_ray_tracer_create(const tg_camera* p_camera)
{
	TG_ASSERT(p_camera);

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    if (!shared_render_resources.ray_tracer.initialized)
    {
        const u16 p_indices[6 * 6] = {
             0,  1,  2,  2,  3,  0, // x-
             4,  5,  6,  6,  7,  4, // x+
             8,  9, 10, 10, 11,  8, // y-
            12, 13, 14, 14, 15, 12, // y+
            16, 17, 18, 18, 19, 16, // z-
            20, 21, 22, 22, 23, 20  // z+
        };

        const v3 p_positions[6 * 4] = {
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

        const v3 p_normals[6 * 4] = {
            (v3){ -1.0f,  0.0f,  0.0f }, // x-
            (v3){ -1.0f,  0.0f,  0.0f },
            (v3){ -1.0f,  0.0f,  0.0f },
            (v3){ -1.0f,  0.0f,  0.0f },
            (v3){  1.0f,  0.0f,  0.0f }, // x+
            (v3){  1.0f,  0.0f,  0.0f },
            (v3){  1.0f,  0.0f,  0.0f },
            (v3){  1.0f,  0.0f,  0.0f },
            (v3){  0.0f, -1.0f,  0.0f }, // y-
            (v3){  0.0f, -1.0f,  0.0f },
            (v3){  0.0f, -1.0f,  0.0f },
            (v3){  0.0f, -1.0f,  0.0f },
            (v3){  0.0f,  1.0f,  0.0f }, // y+
            (v3){  0.0f,  1.0f,  0.0f },
            (v3){  0.0f,  1.0f,  0.0f },
            (v3){  0.0f,  1.0f,  0.0f },
            (v3){  0.0f,  0.0f, -1.0f }, // z-
            (v3){  0.0f,  0.0f, -1.0f },
            (v3){  0.0f,  0.0f, -1.0f },
            (v3){  0.0f,  0.0f, -1.0f },
            (v3){  0.0f,  0.0f,  1.0f }, // z+
            (v3){  0.0f,  0.0f,  1.0f },
            (v3){  0.0f,  0.0f,  1.0f },
            (v3){  0.0f,  0.0f,  1.0f }
        };

        const tg_size staging_buffer_size = TG_MAX3(sizeof(p_indices), sizeof(p_positions), sizeof(p_normals));
        tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);

        tg_memcpy(sizeof(p_indices), p_indices, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.ray_tracer.cube_ibo = TGVK_BUFFER_CREATE(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_indices), p_staging_buffer, &shared_render_resources.ray_tracer.cube_ibo);

        tg_memcpy(sizeof(p_positions), p_positions, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.ray_tracer.cube_vbo_p = TGVK_BUFFER_CREATE(sizeof(p_positions), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_positions), p_staging_buffer, &shared_render_resources.ray_tracer.cube_vbo_p);

        tg_memcpy(sizeof(p_normals), p_normals, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.ray_tracer.cube_vbo_n = TGVK_BUFFER_CREATE(sizeof(p_normals), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_normals), p_staging_buffer, &shared_render_resources.ray_tracer.cube_vbo_n);

        tgvk_global_staging_buffer_release();



        VkSubpassDescription vis_subpass_description = { 0 };
        vis_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        shared_render_resources.ray_tracer.visibility_render_pass = tgvk_render_pass_create(TG_NULL, &vis_subpass_description);



        shared_render_resources.ray_tracer.initialized = TG_TRUE;
    }

	tg_ray_tracer_h h_ray_tracer = tgvk_handle_take(TG_STRUCTURE_TYPE_RAY_TRACER);

    h_ray_tracer->p_camera = p_camera;
    h_ray_tracer->hdr_color_attachment = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE, w, h, TGVK_HDR_FORMAT, TG_NULL);
    h_ray_tracer->render_target = TGVK_RENDER_TARGET_CREATE(
        w, h, TGVK_HDR_FORMAT, TG_NULL,
        w, h, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );

    h_ray_tracer->semaphore = tgvk_semaphore_create();

    tg__init_visibility_pass(h_ray_tracer);
    tg__init_shading_pass(h_ray_tracer);
    tg__init_blit_pass(h_ray_tracer);
    tg__init_present_pass(h_ray_tracer);
    tg__init_clear_pass(h_ray_tracer);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_ray_tracer->hdr_color_attachment, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    //for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    //{
    //    tgvk_cmd_transition_image_layout(p_command_buffer, &h_ray_tracer->geometry_pass.p_color_attachments[i], TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    //}
    tgvk_command_buffer_end_and_submit(p_command_buffer);

	return h_ray_tracer;
}

void tg_ray_tracer_destroy(tg_ray_tracer_h h_ray_tracer)
{
	TG_ASSERT(h_ray_tracer);

	TG_NOT_IMPLEMENTED();
}

void tg_ray_tracer_push_obj(tg_ray_tracer_h h_ray_tracer, tg_obj_h h_obj)
{
    TG_ASSERT(h_ray_tracer);
    TG_ASSERT(h_obj);

    if (h_ray_tracer->objs.capacity == h_ray_tracer->objs.count)
    {
        if (h_ray_tracer->objs.capacity == 0)
        {
            TG_ASSERT(h_ray_tracer->objs.pp_objs == TG_NULL);
            h_ray_tracer->objs.capacity = 8;
            h_ray_tracer->objs.pp_objs = TG_MALLOC(h_ray_tracer->objs.capacity * sizeof(*h_ray_tracer->objs.pp_objs));
        }
        else
        {
            TG_ASSERT(h_ray_tracer->objs.pp_objs != TG_NULL);
            h_ray_tracer->objs.capacity *= 2;
            h_ray_tracer->objs.pp_objs = TG_REALLOC(h_ray_tracer->objs.capacity * sizeof(*h_ray_tracer->objs.pp_objs), h_ray_tracer->objs.pp_objs);
        }
    }
    h_ray_tracer->objs.pp_objs[h_ray_tracer->objs.count++] = h_obj;
}

void tg_ray_tracer_render(tg_ray_tracer_h h_ray_tracer)
{
    TG_ASSERT(h_ray_tracer);

    const tg_camera c = *h_ray_tracer->p_camera;

    const m4 r = tgm_m4_inverse(tgm_m4_euler(c.pitch, c.yaw, c.roll));
    const m4 t = tgm_m4_translate(tgm_v3_neg(c.position));
    const m4 v = tgm_m4_mul(r, t);
    const m4 iv = tgm_m4_inverse(v);
    const m4 p = c.type == TG_CAMERA_TYPE_ORTHOGRAPHIC ? tgm_m4_orthographic(c.ortho.l, c.ortho.r, c.ortho.b, c.ortho.t, c.ortho.f, c.ortho.n) : tgm_m4_perspective(c.persp.fov_y_in_radians, c.persp.aspect, c.persp.n, c.persp.f);
    const m4 ip = tgm_m4_inverse(p);
    const m4 vp = tgm_m4_mul(p, v);
    const m4 ivp = tgm_m4_inverse(vp);

    TGVK_CAMERA_VIEW(h_ray_tracer->visibility_pass.view_projection_ubo) = v;
    TGVK_CAMERA_PROJ(h_ray_tracer->visibility_pass.view_projection_ubo) = p;

    tg_ray_tracing_ubo* p_ray_tracing_ubo = h_ray_tracer->visibility_pass.ray_tracing_ubo.memory.p_mapped_device_memory;
    const m4 ivp_no_translation = tgm_m4_inverse(tgm_m4_mul(p, r));
    p_ray_tracing_ubo->camera.xyz = c.position;
    p_ray_tracing_ubo->ray00.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray10.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f, -1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray01.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray11.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f, -1.0f, 1.0f, 1.0f }).xyz);

    tg_shading_ubo* p_shading_ubo = &TGVK_SHADING_UBO;
    //p_shading_ubo->camera_position.xyz = c.position;
    p_shading_ubo->ivp = ivp;

    //tgvk_atmosphere_model_update(&h_ray_tracer->model, iv, ip);

    tgvk_command_buffer_begin(&h_ray_tracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
    tgvk_cmd_begin_render_pass(&h_ray_tracer->visibility_pass.command_buffer, shared_render_resources.ray_tracer.visibility_render_pass, &h_ray_tracer->visibility_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(h_ray_tracer->visibility_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->visibility_pass.pipeline.pipeline);
    vkCmdBindIndexBuffer(h_ray_tracer->visibility_pass.command_buffer.command_buffer, shared_render_resources.ray_tracer.cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(h_ray_tracer->visibility_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.ray_tracer.cube_vbo_p.buffer, &vertex_buffer_offset);
    vkCmdBindVertexBuffers(h_ray_tracer->visibility_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.ray_tracer.cube_vbo_n.buffer, &vertex_buffer_offset);
    for (u32 i = 0; i < h_ray_tracer->objs.count; i++)
    {
        vkCmdBindDescriptorSets(h_ray_tracer->visibility_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_ray_tracer->visibility_pass.pipeline.layout.pipeline_layout, 0, 1, &h_ray_tracer->objs.pp_objs[i]->descriptor_set.descriptor_set, 0, TG_NULL);
        tgvk_cmd_draw_indexed(&h_ray_tracer->visibility_pass.command_buffer, 6 * 6); // TODO: triangle fans for less indices?
    }

    // TODO look at below
    //vkCmdExecuteCommands(h_ray_tracer->geometry_pass.command_buffer.command_buffer, h_ray_tracer->deferred_command_buffer_count, h_ray_tracer->p_deferred_command_buffers);
    vkCmdEndRenderPass(h_ray_tracer->visibility_pass.command_buffer.command_buffer);
    TGVK_CALL(vkEndCommandBuffer(h_ray_tracer->visibility_pass.command_buffer.command_buffer));

    tgvk_fence_wait(h_ray_tracer->render_target.fence); // TODO: isn't this wrong? this means that some buffers are potentially updated too early
    tgvk_fence_reset(h_ray_tracer->render_target.fence);

    VkSubmitInfo visibility_submit_info = { 0 };
    visibility_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    visibility_submit_info.pNext = TG_NULL;
    visibility_submit_info.waitSemaphoreCount = 0;
    visibility_submit_info.pWaitSemaphores = TG_NULL;
    visibility_submit_info.pWaitDstStageMask = TG_NULL;
    visibility_submit_info.commandBufferCount = 1;
    visibility_submit_info.pCommandBuffers = &h_ray_tracer->visibility_pass.command_buffer.command_buffer;
    visibility_submit_info.signalSemaphoreCount = 1;
    visibility_submit_info.pSignalSemaphores = &h_ray_tracer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &visibility_submit_info, VK_NULL_HANDLE);

    const VkPipelineStageFlags color_attachment_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: don't we have to wait for depth as well? also check stages below (draw)

    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &h_ray_tracer->semaphore;
    shading_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &h_ray_tracer->shading_pass.command_buffer.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &h_ray_tracer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);

    u32 current_image;
    TGVK_CALL(vkAcquireNextImageKHR(device, swapchain, TG_U64_MAX, h_ray_tracer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    const VkSemaphore p_wait_semaphores[2] = { h_ray_tracer->semaphore, h_ray_tracer->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

    VkSubmitInfo present_submit_info = { 0 };
    present_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    present_submit_info.pNext = TG_NULL;
    present_submit_info.waitSemaphoreCount = 2;
    present_submit_info.pWaitSemaphores = p_wait_semaphores;
    present_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
    present_submit_info.commandBufferCount = 1;
    present_submit_info.pCommandBuffers = &h_ray_tracer->present_pass.p_command_buffers[current_image].command_buffer;
    present_submit_info.signalSemaphoreCount = 1;
    present_submit_info.pSignalSemaphores = &h_ray_tracer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &present_submit_info, h_ray_tracer->render_target.fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &h_ray_tracer->semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    tgvk_queue_present(&present_info);
}

void tg_ray_tracer_clear(tg_ray_tracer_h h_ray_tracer)
{
    TG_ASSERT(h_ray_tracer);

    tgvk_fence_wait(h_ray_tracer->render_target.fence);
    tgvk_fence_reset(h_ray_tracer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_ray_tracer->clear_pass.command_buffer.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_ray_tracer->render_target.fence);

    // TODO: forward renderer
    tg_shading_ubo* p_shading_ubo = &TGVK_SHADING_UBO;

    // TODO: lighting
    //p_shading_ubo->directional_light_count = 0;
    //p_shading_ubo->point_light_count = 0;
}

#endif
