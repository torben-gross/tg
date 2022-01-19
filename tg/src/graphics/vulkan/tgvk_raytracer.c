#include "graphics/vulkan/tgvk_raytracer.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tgvk_shader_library.h"



#define TGVK_CAMERA_VIEW(view_projection_ubo)    (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[0])
#define TGVK_CAMERA_PROJ(view_projection_ubo)    (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[1])
#define TGVK_HDR_FORMAT                          VK_FORMAT_R32G32B32A32_SFLOAT
#define TGVK_SHADING_UBO                         (*(tg_shading_ubo*)p_raytracer->shading_pass.ubo.memory.p_mapped_device_memory)



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



static void tg__init_visibility_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->visibility_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    // TODO shared?
    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/visibility.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/visibility.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_FRONT_BIT; // TODO: is this okay for ray tracing? this culls the front face
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.raytracer.visibility_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->visibility_pass.pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->visibility_pass.view_projection_ubo = TGVK_UNIFORM_BUFFER_CREATE(2ui64 * sizeof(m4));
    p_raytracer->visibility_pass.ray_tracing_ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(tg_ray_tracing_ubo));

    const VkDeviceSize staging_buffer_size = 2ui64 * sizeof(u32);
    const VkDeviceSize visibility_buffer_size = staging_buffer_size + ((VkDeviceSize)w * (VkDeviceSize)h * sizeof(u64));
    p_raytracer->visibility_pass.visibility_buffer = TGVK_BUFFER_CREATE(visibility_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);
    ((u32*)p_staging_buffer->memory.p_mapped_device_memory)[0] = w;
    ((u32*)p_staging_buffer->memory.p_mapped_device_memory)[1] = h;
    tgvk_command_buffer_begin(&p_raytracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_copy_buffer(&p_raytracer->visibility_pass.command_buffer, staging_buffer_size, p_staging_buffer, &p_raytracer->visibility_pass.visibility_buffer);
    tgvk_command_buffer_end_and_submit(&p_raytracer->visibility_pass.command_buffer);
    tgvk_global_staging_buffer_release();

    p_raytracer->visibility_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.raytracer.visibility_render_pass, 0, TG_NULL, w, h);
}

static void tg__init_shading_pass(tg_raytracer* p_raytracer)
{
    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    p_raytracer->shading_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/raytracer/shading.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/raytracer/shading.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.shading_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)w;
    graphics_pipeline_create_info.viewport_size.y = (f32)h;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    p_raytracer->shading_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    p_raytracer->shading_pass.ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(tg_shading_ubo));

    p_raytracer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&p_raytracer->shading_pass.graphics_pipeline);
    //tgvk_atmosphere_model_update_descriptor_set(&p_raytracer->model, &p_raytracer->shading_pass.descriptor_set);
    const u32 atmosphere_binding_offset = 4;
    u32 binding_offset = atmosphere_binding_offset;
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(p_raytracer->shading_pass.descriptor_set.set, &p_raytracer->shading_pass.ubo, binding_offset++);

    // TODO: tone mapping and other passes...
    //p_raytracer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, 1, &p_raytracer->hdr_color_attachment.image_view, w, h);
    p_raytracer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, 1, &p_raytracer->render_target.color_attachment.image_view, w, h);

    tgvk_command_buffer_begin(&p_raytracer->shading_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->shading_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(p_raytracer->shading_pass.command_buffer.buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(p_raytracer->shading_pass.command_buffer.buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(p_raytracer->shading_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->shading_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->shading_pass.descriptor_set.set, 0, TG_NULL);

        tgvk_cmd_begin_render_pass(&p_raytracer->shading_pass.command_buffer, shared_render_resources.shading_render_pass, &p_raytracer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        tgvk_cmd_draw_indexed(&p_raytracer->shading_pass.command_buffer, 6);
        vkCmdEndRenderPass(p_raytracer->shading_pass.command_buffer.buffer);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->shading_pass.command_buffer.buffer));
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

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        p_raytracer->present_pass.p_framebuffers[i] = tgvk_framebuffer_create(shared_render_resources.present_render_pass, 1, &p_swapchain_image_views[i], w, h);
    }

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = tgvk_shader_library_get("shaders/renderer/screen_quad.vert");
    graphics_pipeline_create_info.p_fragment_shader = tgvk_shader_library_get("shaders/renderer/present.frag");
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.present_render_pass;
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
            vkCmdBindIndexBuffer(p_raytracer->present_pass.p_command_buffers[i].buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
            const VkDeviceSize vertex_buffer_offset = 0;
            vkCmdBindVertexBuffers(p_raytracer->present_pass.p_command_buffers[i].buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(p_raytracer->present_pass.p_command_buffers[i].buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindDescriptorSets(p_raytracer->present_pass.p_command_buffers[i].buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->present_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->present_pass.descriptor_set.set, 0, TG_NULL);

            tgvk_cmd_begin_render_pass(&p_raytracer->present_pass.p_command_buffers[i], shared_render_resources.present_render_pass, &p_raytracer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
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
    tgvk_descriptor_set_update_storage_buffer(p_raytracer->clear_pass.descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer, 0);

    tgvk_command_buffer_begin(&p_raytracer->clear_pass.command_buffer, 0);
    {
        vkCmdBindPipeline(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(p_raytracer->clear_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_raytracer->clear_pass.compute_pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->clear_pass.descriptor_set.set, 0, TG_NULL);
        vkCmdDispatch(p_raytracer->clear_pass.command_buffer.buffer, (w + 31) / 32, (h + 31) / 32, 1);
    }
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->clear_pass.command_buffer.buffer));
}



void tg_raytracer_create(const tg_camera* p_camera, u32 max_object_count, TG_OUT tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_camera);
	TG_ASSERT(max_object_count); // TODO: init here
	TG_ASSERT(p_raytracer);

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    if (!shared_render_resources.raytracer.initialized)
    {



        const u16 p_screen_quad_indices[6] = { 0, 1, 2, 2, 3, 0 };
        
        v2 p_screen_quad_positions[4] = { 0 };
        p_screen_quad_positions[0] = (v2){ -1.0f,  1.0f };
        p_screen_quad_positions[1] = (v2){  1.0f,  1.0f };
        p_screen_quad_positions[2] = (v2){  1.0f, -1.0f };
        p_screen_quad_positions[3] = (v2){ -1.0f, -1.0f };
        
        v2 p_screen_quad_uvs[4] = { 0 };
        p_screen_quad_uvs[0] = (v2){ 0.0f,  1.0f };
        p_screen_quad_uvs[1] = (v2){ 1.0f,  1.0f };
        p_screen_quad_uvs[2] = (v2){ 1.0f,  0.0f };
        p_screen_quad_uvs[3] = (v2){ 0.0f,  0.0f };
        
        const tg_size screen_quad_staging_buffer_size = TG_MAX3(sizeof(p_screen_quad_indices), sizeof(p_screen_quad_positions), sizeof(p_screen_quad_uvs));
        tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(screen_quad_staging_buffer_size);
        
        tg_memcpy(sizeof(p_screen_quad_indices), p_screen_quad_indices, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.screen_quad_indices = TGVK_BUFFER_CREATE(sizeof(p_screen_quad_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_screen_quad_indices), p_staging_buffer, &shared_render_resources.screen_quad_indices);
        
        tg_memcpy(sizeof(p_screen_quad_positions), p_screen_quad_positions, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.screen_quad_positions_buffer = TGVK_BUFFER_CREATE(sizeof(p_screen_quad_positions), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_screen_quad_positions), p_staging_buffer, &shared_render_resources.screen_quad_positions_buffer);
        
        tg_memcpy(sizeof(p_screen_quad_uvs), p_screen_quad_uvs, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.screen_quad_uvs_buffer = TGVK_BUFFER_CREATE(sizeof(p_screen_quad_uvs), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_screen_quad_uvs), p_staging_buffer, &shared_render_resources.screen_quad_uvs_buffer);
        
        tgvk_global_staging_buffer_release();
        
        // shading pass
        {
            VkAttachmentDescription attachment_description = { 0 };
            attachment_description.flags = 0;
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
            subpass_description.flags = 0;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.inputAttachmentCount = 0;
            subpass_description.pInputAttachments = TG_NULL;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &attachment_reference;
            subpass_description.pResolveAttachments = TG_NULL;
            subpass_description.pDepthStencilAttachment = TG_NULL;
            subpass_description.preserveAttachmentCount = 0;
            subpass_description.pPreserveAttachments = TG_NULL;
        
            shared_render_resources.shading_render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);
        }
        
        // forward pass
        {
            VkAttachmentDescription p_attachment_descriptions[2] = { 0 };
        
            p_attachment_descriptions[0].flags = 0;
            p_attachment_descriptions[0].format = TGVK_HDR_FORMAT;
            p_attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
            p_attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            p_attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            p_attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            p_attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            p_attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            p_attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
            p_attachment_descriptions[1].flags = 0;
            p_attachment_descriptions[1].format = VK_FORMAT_D32_SFLOAT;
            p_attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
            p_attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            p_attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            p_attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            p_attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            p_attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            p_attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
            VkAttachmentReference color_attachment_reference = { 0 };
            color_attachment_reference.attachment = 0;
            color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
            VkAttachmentReference depth_buffer_attachment_reference = { 0 };
            depth_buffer_attachment_reference.attachment = 1;
            depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
            VkSubpassDescription subpass_description = { 0 };
            subpass_description.flags = 0;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.inputAttachmentCount = 0;
            subpass_description.pInputAttachments = TG_NULL;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pResolveAttachments = TG_NULL;
            subpass_description.pDepthStencilAttachment = &depth_buffer_attachment_reference;
            subpass_description.preserveAttachmentCount = 0;
            subpass_description.pPreserveAttachments = TG_NULL;
        
            shared_render_resources.forward_render_pass = tgvk_render_pass_create(p_attachment_descriptions, &subpass_description);
        }
        
        // tone mapping pass
        {
            VkAttachmentDescription attachment_description = { 0 };
            attachment_description.flags = 0;
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
            subpass_description.flags = 0;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.inputAttachmentCount = 0;
            subpass_description.pInputAttachments = TG_NULL;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &attachment_reference;
            subpass_description.pResolveAttachments = TG_NULL;
            subpass_description.pDepthStencilAttachment = TG_NULL;
            subpass_description.preserveAttachmentCount = 0;
            subpass_description.pPreserveAttachments = TG_NULL;
        
            shared_render_resources.tone_mapping_render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);
        }
        
        // ui pass
        {
            VkAttachmentDescription attachment_description = { 0 };
            attachment_description.flags = 0;
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
            subpass_description.flags = 0;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.inputAttachmentCount = 0;
            subpass_description.pInputAttachments = TG_NULL;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &attachment_reference;
            subpass_description.pResolveAttachments = TG_NULL;
            subpass_description.pDepthStencilAttachment = TG_NULL;
            subpass_description.preserveAttachmentCount = 0;
            subpass_description.pPreserveAttachments = TG_NULL;
        
            shared_render_resources.ui_render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);
        }
        
        // present pass
        {
            VkAttachmentReference color_attachment_reference = { 0 };
            color_attachment_reference.attachment = 0;
            color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
            VkAttachmentDescription attachment_description = { 0 };
            attachment_description.flags = 0;
            attachment_description.format = surface.format.format;
            attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
            VkSubpassDescription subpass_description = { 0 };
            subpass_description.flags = 0;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_description.inputAttachmentCount = 0;
            subpass_description.pInputAttachments = TG_NULL;
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_reference;
            subpass_description.pResolveAttachments = TG_NULL;
            subpass_description.pDepthStencilAttachment = TG_NULL;
            subpass_description.preserveAttachmentCount = 0;
            subpass_description.pPreserveAttachments = TG_NULL;
        
            shared_render_resources.present_render_pass = tgvk_render_pass_create(&attachment_description, &subpass_description);
        }






































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

        const v3 p_cube_normals[6 * 4] = {
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

        // TODO: do all of the copies at once!
        const tg_size cube_staging_buffer_size = TG_MAX3(sizeof(p_cube_indices), sizeof(p_cube_positions), sizeof(p_cube_normals));
        p_staging_buffer = tgvk_global_staging_buffer_take(cube_staging_buffer_size);

        tg_memcpy(sizeof(p_cube_indices), p_cube_indices, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.raytracer.cube_ibo = TGVK_BUFFER_CREATE(sizeof(p_cube_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_cube_indices), p_staging_buffer, &shared_render_resources.raytracer.cube_ibo);

        tg_memcpy(sizeof(p_cube_positions), p_cube_positions, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.raytracer.cube_vbo_p = TGVK_BUFFER_CREATE(sizeof(p_cube_positions), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_cube_positions), p_staging_buffer, &shared_render_resources.raytracer.cube_vbo_p);

        tg_memcpy(sizeof(p_cube_normals), p_cube_normals, p_staging_buffer->memory.p_mapped_device_memory);
        shared_render_resources.raytracer.cube_vbo_n = TGVK_BUFFER_CREATE(sizeof(p_cube_normals), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
        tgvk_buffer_copy(sizeof(p_cube_normals), p_staging_buffer, &shared_render_resources.raytracer.cube_vbo_n);

        tgvk_global_staging_buffer_release();



        VkSubpassDescription vis_subpass_description = { 0 };
        vis_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        shared_render_resources.raytracer.visibility_render_pass = tgvk_render_pass_create(TG_NULL, &vis_subpass_description);



        shared_render_resources.raytracer.initialized = TG_TRUE;
    }

    p_raytracer->p_camera = p_camera;
    p_raytracer->hdr_color_attachment = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE, w, h, TGVK_HDR_FORMAT, TG_NULL);
    p_raytracer->render_target = TGVK_RENDER_TARGET_CREATE(
        w, h, TGVK_HDR_FORMAT, TG_NULL,
        w, h, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );

    p_raytracer->semaphore = tgvk_semaphore_create();

    p_raytracer->objs.capacity = max_object_count;
    p_raytracer->objs.count = 0;
    p_raytracer->objs.p_objs = TG_MALLOC(p_raytracer->objs.capacity * sizeof(*p_raytracer->objs.p_objs));

    tg__init_visibility_pass(p_raytracer);
    tg__init_shading_pass(p_raytracer);
    tg__init_blit_pass(p_raytracer);
    tg__init_present_pass(p_raytracer);
    tg__init_clear_pass(p_raytracer);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &p_raytracer->hdr_color_attachment, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    //for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    //{
    //    tgvk_cmd_transition_image_layout(p_command_buffer, &p_raytracer->geometry_pass.p_color_attachments[i], TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    //}
    tgvk_command_buffer_end_and_submit(p_command_buffer);
}

void tg_raytracer_destroy(tg_raytracer* p_raytracer)
{
	TG_ASSERT(p_raytracer);

	TG_NOT_IMPLEMENTED();






    tgvk_render_pass_destroy(shared_render_resources.present_render_pass);
    tgvk_render_pass_destroy(shared_render_resources.ui_render_pass);
    tgvk_render_pass_destroy(shared_render_resources.tone_mapping_render_pass);
    tgvk_render_pass_destroy(shared_render_resources.forward_render_pass);
    tgvk_render_pass_destroy(shared_render_resources.shading_render_pass);
    tgvk_buffer_destroy(&shared_render_resources.screen_quad_uvs_buffer);
    tgvk_buffer_destroy(&shared_render_resources.screen_quad_positions_buffer);
    tgvk_buffer_destroy(&shared_render_resources.screen_quad_indices);
}

void tg_raytracer_create_obj(tg_raytracer* p_raytracer, u32 log2_w, u32 log2_h, u32 log2_d)
{
    TG_ASSERT(p_raytracer);
    TG_ASSERT(log2_w <= 32);
    TG_ASSERT(log2_h <= 32);
    TG_ASSERT(log2_d <= 32);
    TG_ASSERT(p_raytracer->objs.count < p_raytracer->objs.capacity);

    const u32 obj_id = p_raytracer->objs.count++;
    tgvk_obj* p_obj = &p_raytracer->objs.p_objs[obj_id];

    const u32 w = 1 << log2_w;
    const u32 h = 1 << log2_h;
    const u32 d = 1 << log2_d;

    if (obj_id == 0)
    {
        p_obj->first_voxel_id = 0;
    }
    else
    {
        const tgvk_obj* p_prev_obj = p_obj - 1;
        p_obj->first_voxel_id = p_prev_obj->first_voxel_id;
        const u32 mask = (1 << 5) - 1;
        const u32 prev_log2_w = p_prev_obj->packed_log2_whd & mask;
        const u32 prev_log2_h = (p_prev_obj->packed_log2_whd >> 5) & mask;
        const u32 prev_log2_d = (p_prev_obj->packed_log2_whd >> 10) & mask;
        const u32 prev_w = 1 << prev_log2_w;
        const u32 prev_h = 1 << prev_log2_h;
        const u32 prev_d = 1 << prev_log2_d;
        const u32 prev_num_voxels = prev_w * prev_h * prev_d;
        p_obj->first_voxel_id += prev_num_voxels;
    }

    p_obj->ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(m4) + sizeof(u32));
    m4* p_model = (m4*)p_obj->ubo.memory.p_mapped_device_memory;
    u32* p_first_voxel_id = (u32*)(p_model + 1);
    const v3 scale = { (f32)w, (f32)h, (f32)d };
    *p_model = tgm_m4_scale(scale);
    *p_first_voxel_id = p_obj->first_voxel_id;

    p_obj->descriptor_set = tgvk_descriptor_set_create(&p_raytracer->visibility_pass.pipeline);
    tgvk_descriptor_set_update_uniform_buffer(p_obj->descriptor_set.set, &p_obj->ubo, 0);

    u32 packed = log2_w;
    packed = packed | (log2_h << 5);
    packed = packed | (log2_d << 10);
    p_obj->packed_log2_whd = (u16)packed;

    // TODO: gen on GPU
    const tg_size buffer_size = 4ui64 * sizeof(u32) + ((tg_size)w * (tg_size)h * (tg_size)d * sizeof(u32)) / 32;
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(buffer_size);
    u32* p_it = p_staging_buffer->memory.p_mapped_device_memory;
    *p_it++ = w;
    *p_it++ = h;
    *p_it++ = d;
    p_it++; // TODO: pad required?
    u32 bits = 0;
    u8 bit_idx = 0;

    for (u32 z = 0; z < d; z++)
    {
        for (u32 y = 0; y < h; y++)
        {
            for (u32 x = 0; x < w; x++)
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
                const b32 solid = f2 <= 0;
                //const b32 solid = y == 0;

                bits |= solid << bit_idx;
                bit_idx++;

                // TODO: space filling z curve
                if (bit_idx == 32)
                {
                    *p_it++ = bits;
                    bit_idx = 0;
                    bits = 0;
                }
            }
        }
    }

    //const u32 idx = (d - 1) * (w / 32) * h + (h - 1) * (w / 32) + (w - 1) / 32;
    //((u32*)p_staging_buffer->memory.p_mapped_device_memory)[4ui64 * sizeof(u32) + (tg_size)idx] = TG_U32_MAX;

    // TODO: lods
    p_obj->voxels = TGVK_BUFFER_CREATE(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_copy_buffer(p_command_buffer, buffer_size, p_staging_buffer, &p_obj->voxels);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tgvk_global_staging_buffer_release();

    tgvk_descriptor_set_update_uniform_buffer(p_obj->descriptor_set.set, &p_obj->ubo, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_obj->descriptor_set.set, &p_raytracer->visibility_pass.view_projection_ubo, 1);
    tgvk_descriptor_set_update_uniform_buffer(p_obj->descriptor_set.set, &p_raytracer->visibility_pass.ray_tracing_ubo, 2);
    tgvk_descriptor_set_update_storage_buffer(p_obj->descriptor_set.set, &p_raytracer->visibility_pass.visibility_buffer, 3);
    tgvk_descriptor_set_update_storage_buffer(p_obj->descriptor_set.set, &p_obj->voxels, 4);
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

    TGVK_CAMERA_VIEW(p_raytracer->visibility_pass.view_projection_ubo) = v;
    TGVK_CAMERA_PROJ(p_raytracer->visibility_pass.view_projection_ubo) = p;

    tg_ray_tracing_ubo* p_ray_tracing_ubo = p_raytracer->visibility_pass.ray_tracing_ubo.memory.p_mapped_device_memory;
    const m4 ivp_no_translation = tgm_m4_inverse(tgm_m4_mul(p, r));
    p_ray_tracing_ubo->camera.xyz = c.position;
    p_ray_tracing_ubo->ray00.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray10.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f, -1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray01.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f,  1.0f, 1.0f, 1.0f }).xyz);
    p_ray_tracing_ubo->ray11.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f, -1.0f, 1.0f, 1.0f }).xyz);

    tg_shading_ubo* p_shading_ubo = &TGVK_SHADING_UBO;
    //p_shading_ubo->camera_position.xyz = c.position;
    p_shading_ubo->ivp = ivp;

    //tgvk_atmosphere_model_update(&p_raytracer->model, iv, ip);

    tgvk_command_buffer_begin(&p_raytracer->visibility_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
    tgvk_cmd_begin_render_pass(&p_raytracer->visibility_pass.command_buffer, shared_render_resources.raytracer.visibility_render_pass, &p_raytracer->visibility_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(p_raytracer->visibility_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->visibility_pass.pipeline.pipeline);
    vkCmdBindIndexBuffer(p_raytracer->visibility_pass.command_buffer.buffer, shared_render_resources.raytracer.cube_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 0, 1, &shared_render_resources.raytracer.cube_vbo_p.buffer, &vertex_buffer_offset);
    vkCmdBindVertexBuffers(p_raytracer->visibility_pass.command_buffer.buffer, 1, 1, &shared_render_resources.raytracer.cube_vbo_n.buffer, &vertex_buffer_offset);
    for (u32 i = 0; i < p_raytracer->objs.count; i++)
    {
        vkCmdBindDescriptorSets(p_raytracer->visibility_pass.command_buffer.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_raytracer->visibility_pass.pipeline.layout.pipeline_layout, 0, 1, &p_raytracer->objs.p_objs[i].descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_draw_indexed(&p_raytracer->visibility_pass.command_buffer, 6 * 6); // TODO: triangle fans for less indices?
    }

    // TODO look at below
    //vkCmdExecuteCommands(p_raytracer->geometry_pass.command_buffer.buffer, p_raytracer->deferred_command_buffer_count, p_raytracer->p_deferred_command_buffers);
    vkCmdEndRenderPass(p_raytracer->visibility_pass.command_buffer.buffer);
    TGVK_CALL(vkEndCommandBuffer(p_raytracer->visibility_pass.command_buffer.buffer));

    tgvk_fence_wait(p_raytracer->render_target.fence); // TODO: isn't this wrong? this means that some buffers are potentially updated too early
    tgvk_fence_reset(p_raytracer->render_target.fence);

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

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &visibility_submit_info, VK_NULL_HANDLE);

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

    // TODO: forward renderer
    tg_shading_ubo* p_shading_ubo = &TGVK_SHADING_UBO;

    // TODO: lighting
    //p_shading_ubo->directional_light_count = 0;
    //p_shading_ubo->point_light_count = 0;
}

#endif
