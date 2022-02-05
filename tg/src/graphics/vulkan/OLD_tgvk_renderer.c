#if 0
#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "graphics/tg_image_io.h"
#include "util/tg_string.h"



#define TGVK_CAMERA_VIEW(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[0])
#define TGVK_CAMERA_PROJ(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[1])

#define TGVK_HDR_FORMAT                                   VK_FORMAT_R32G32B32A32_SFLOAT

#define TGVK_GEOMETRY_FORMATS(var)                        const VkFormat var[TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT] = { VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM }

#define TGVK_SHADING_UBO                                  (*(tg_shading_data_ubo*)h_renderer->shading_pass.ubo.memory.p_mapped_device_memory)



typedef struct tg_shading_data_ubo
{
    v4     camera_position;
    v4     sun_direction;
    u32    directional_light_count;
    u32    point_light_count; u32 pad[2];
    m4     ivp;
    v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_shading_data_ubo;

typedef struct tg_raytracer_ubo
{
    v4    camera;
    v4    ray00;
    v4    ray10;
    v4    ray01;
    v4    ray11;
} tg_raytracer_ubo;



static void tg__init_geometry_pass(tg_renderer_h h_renderer)
{
    h_renderer->geometry_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    TGVK_GEOMETRY_FORMATS(p_formats);
    for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    {
        h_renderer->geometry_pass.p_color_attachments[i] = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR, swapchain_extent.width, swapchain_extent.height, p_formats[i], TG_NULL);
    }

    VkImageView p_framebuffer_attachments[TGVK_GEOMETRY_ATTACHMENT_COUNT] = { 0 };
    for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    {
        p_framebuffer_attachments[i] = h_renderer->geometry_pass.p_color_attachments[i].image_view;
    }
    p_framebuffer_attachments[TGVK_GEOMETRY_ATTACHMENT_DEPTH] = h_renderer->render_target.depth_attachment.image_view;

    h_renderer->geometry_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.geometry_render_pass, TGVK_GEOMETRY_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);
}

static void tg__init_shading_pass(tg_renderer_h h_renderer)
{
    h_renderer->shading_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    h_renderer->shading_pass.ubo = TGVK_BUFFER_CREATE_UBO(sizeof(tg_shading_data_ubo));
    h_renderer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, 1, &h_renderer->hdr_color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/renderer/shading.vert")->shader;

    tg_file_properties file_properties = { 0 };
    const b32 result = tgp_file_get_properties("shaders/renderer/shading.inc", &file_properties);
    TG_ASSERT(result);

    const tg_size api_shader_size = (tg_size)h_renderer->model.api_shader.total_count;
    const tg_size fragment_shader_buffer_size = api_shader_size + file_properties.size + 1LL;
    char* p_fragment_shader_buffer = TG_MALLOC_STACK(fragment_shader_buffer_size);
    const tg_size fragment_shader_buffer_offset = tg_strncpy(fragment_shader_buffer_size, p_fragment_shader_buffer, api_shader_size, h_renderer->model.api_shader.p_source);

    tgp_file_load("shaders/renderer/shading.inc", fragment_shader_buffer_size - fragment_shader_buffer_offset, &p_fragment_shader_buffer[fragment_shader_buffer_offset]);
    p_fragment_shader_buffer[fragment_shader_buffer_size - 1] = '\0';
    h_renderer->shading_pass.fragment_shader = tgvk_shader_create_from_glsl(TG_SHADER_TYPE_FRAGMENT, p_fragment_shader_buffer);

    graphics_pipeline_create_info.p_fragment_shader = &h_renderer->shading_pass.fragment_shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    graphics_pipeline_create_info.render_pass = shared_render_resources.shading_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->shading_pass.framebuffer.width;
    graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->shading_pass.framebuffer.height;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->shading_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    TG_FREE_STACK(fragment_shader_buffer_size);

    h_renderer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->shading_pass.graphics_pipeline);
    tgvk_atmosphere_model_update_descriptor_set(&h_renderer->model, &h_renderer->shading_pass.descriptor_set);
    const u32 atmosphere_binding_offset = 4;
    u32 binding_offset = atmosphere_binding_offset;
    for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    {
        tgvk_descriptor_set_update_image(h_renderer->shading_pass.descriptor_set.set, &h_renderer->geometry_pass.p_color_attachments[i], binding_offset++);
    }
    tgvk_descriptor_set_update_image(h_renderer->shading_pass.descriptor_set.set, &h_renderer->render_target.depth_attachment, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.descriptor_set.set, &h_renderer->shading_pass.ubo, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.descriptor_set.set, &h_renderer->model.rendering.ubo, binding_offset++);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.descriptor_set.set, &h_renderer->model.rendering.vertex_shader_ubo, binding_offset++);

    tgvk_command_buffer_begin(&h_renderer->shading_pass.command_buffer, 0);
    {
        for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
        {
            tgvk_cmd_transition_image_layout(&h_renderer->shading_pass.command_buffer, &h_renderer->geometry_pass.p_color_attachments[i], TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_F);
        }
        tgvk_cmd_transition_image_layout(&h_renderer->shading_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_F);

        vkCmdBindPipeline(h_renderer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(h_renderer->shading_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->shading_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->shading_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->shading_pass.descriptor_set.set, 0, TG_NULL);

        tgvk_cmd_begin_render_pass(&h_renderer->shading_pass.command_buffer, shared_render_resources.shading_render_pass, &h_renderer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        tgvk_cmd_draw_indexed(&h_renderer->shading_pass.command_buffer, 6);
        vkCmdEndRenderPass(h_renderer->shading_pass.command_buffer.command_buffer);

        for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
        {
            tgvk_cmd_transition_image_layout(&h_renderer->shading_pass.command_buffer, &h_renderer->geometry_pass.p_color_attachments[i], TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
        }
        tgvk_cmd_transition_image_layout(&h_renderer->shading_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
    }
    TGVK_CALL(vkEndCommandBuffer(h_renderer->shading_pass.command_buffer.command_buffer));
}

static void tg__init_forward_pass(tg_renderer_h h_renderer)
{
    h_renderer->forward_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    const VkImageView p_image_views[2] = { h_renderer->hdr_color_attachment.image_view, h_renderer->render_target.depth_attachment.image_view };
    h_renderer->forward_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.forward_render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);
}

static void tg__init_ray_tracing_pass(tg_renderer_h h_renderer)
{
    h_renderer->raytrace_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_COMPUTE, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    h_renderer->raytrace_pass.shader = tg_compute_shader_create("shaders/renderer/terrain_raytracer.comp")->shader;
    h_renderer->raytrace_pass.pipeline = tgvk_pipeline_create_compute(&h_renderer->raytrace_pass.shader);
    h_renderer->raytrace_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->raytrace_pass.pipeline);
    h_renderer->raytrace_pass.ubo = TGVK_BUFFER_CREATE_UBO(sizeof(tg_raytracer_ubo));
}

static void tg__init_tone_mapping_pass(tg_renderer_h h_renderer)
{
    h_renderer->tone_mapping_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    const VkDeviceSize exposure_sum_buffer_size = 2 * sizeof(u32);

    h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer = TGVK_BUFFER_CREATE(exposure_sum_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, TGVK_MEMORY_DEVICE);
    h_renderer->tone_mapping_pass.acquire_exposure_compute_shader = tg_compute_shader_create("shaders/renderer/acquire_exposure.comp")->shader;
    h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline = tgvk_pipeline_create_compute(&h_renderer->tone_mapping_pass.acquire_exposure_compute_shader);
    h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline);
    h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer = TGVK_BUFFER_CREATE(exposure_sum_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    h_renderer->tone_mapping_pass.finalize_exposure_compute_shader = tg_compute_shader_create("shaders/renderer/finalize_exposure.comp")->shader;
    h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline = tgvk_pipeline_create_compute(&h_renderer->tone_mapping_pass.finalize_exposure_compute_shader);
    h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline);
    h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer = TGVK_BUFFER_CREATE(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo = TGVK_BUFFER_CREATE_UBO(sizeof(f32));

    h_renderer->tone_mapping_pass.adapt_exposure_framebuffer = tgvk_framebuffer_create(shared_render_resources.tone_mapping_render_pass, 1, &h_renderer->render_target.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tgvk_graphics_pipeline_create_info exposure_graphics_pipeline_create_info = { 0 };
    exposure_graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/renderer/screen_quad.vert")->shader;
    exposure_graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_create("shaders/renderer/adapt_exposure.frag")->shader;
    exposure_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    exposure_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    exposure_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    exposure_graphics_pipeline_create_info.p_blend_modes = TG_NULL;
    exposure_graphics_pipeline_create_info.render_pass = shared_render_resources.tone_mapping_render_pass;
    exposure_graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->tone_mapping_pass.adapt_exposure_framebuffer.width;
    exposure_graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->tone_mapping_pass.adapt_exposure_framebuffer.height;
    exposure_graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline = tgvk_pipeline_create_graphics(&exposure_graphics_pipeline_create_info);
    h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline);



    tgvk_descriptor_set_update_image(h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.set, &h_renderer->hdr_color_attachment, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.set, &h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer, 1);

    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.set, &h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.set, &h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer, 1);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.set, &h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo, 2);

    tgvk_descriptor_set_update_image(h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.set, &h_renderer->hdr_color_attachment, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.set, &h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer, 1);

    tgvk_command_buffer_begin(&h_renderer->tone_mapping_pass.command_buffer, 0);
    {
        tgvk_cmd_copy_buffer(&h_renderer->tone_mapping_pass.command_buffer, exposure_sum_buffer_size, &h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer, &h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer);
        tgvk_cmd_transition_image_layout(&h_renderer->tone_mapping_pass.command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_C);
        vkCmdBindPipeline(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.set, 0, TG_NULL);
        vkCmdDispatch(h_renderer->tone_mapping_pass.command_buffer.command_buffer, 64, 32, 1);

        VkMemoryBarrier memory_barrier = { 0 };
        memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memory_barrier.pNext = TG_NULL;
        memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            h_renderer->tone_mapping_pass.command_buffer.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &memory_barrier, 0, TG_NULL, 0, TG_NULL
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.set, 0, TG_NULL);
        vkCmdDispatch(h_renderer->tone_mapping_pass.command_buffer.command_buffer, 1, 1, 1);

        vkCmdPipelineBarrier(
            h_renderer->tone_mapping_pass.command_buffer.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &memory_barrier, 0, TG_NULL, 0, TG_NULL
        );

        tgvk_cmd_transition_image_layout(&h_renderer->tone_mapping_pass.command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_SHADER_READ_C, TGVK_LAYOUT_SHADER_READ_F);
        vkCmdBindPipeline(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(h_renderer->tone_mapping_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->tone_mapping_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->tone_mapping_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.set, 0, TG_NULL);

        tgvk_cmd_begin_render_pass(&h_renderer->tone_mapping_pass.command_buffer, shared_render_resources.tone_mapping_render_pass, &h_renderer->tone_mapping_pass.adapt_exposure_framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        tgvk_cmd_draw_indexed(&h_renderer->tone_mapping_pass.command_buffer, 6);
        vkCmdEndRenderPass(h_renderer->tone_mapping_pass.command_buffer.command_buffer);

        tgvk_cmd_transition_image_layout(&h_renderer->tone_mapping_pass.command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    }
    TGVK_CALL(vkEndCommandBuffer(h_renderer->tone_mapping_pass.command_buffer.command_buffer));
}

static void tg__init_ui_pass(tg_renderer_h h_renderer)
{
    h_renderer->ui_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
    const u32 w = h_renderer->render_target.color_attachment.width;
    const u32 h = h_renderer->render_target.color_attachment.height;

    h_renderer->ui_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.ui_render_pass, 1, &h_renderer->render_target.color_attachment.image_view, w, h);

    tg_blend_mode blend_mode = TG_BLEND_MODE_BLEND;
    tgvk_graphics_pipeline_create_info pipeline_create_info = { 0 };
    pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/renderer/ui/text.vert")->shader;
    pipeline_create_info.p_fragment_shader = &tg_fragment_shader_create("shaders/renderer/ui/text.frag")->shader;
    pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    pipeline_create_info.depth_test_enable = TG_FALSE;
    pipeline_create_info.depth_write_enable = TG_FALSE;
    pipeline_create_info.p_blend_modes = &blend_mode;
    pipeline_create_info.render_pass = shared_render_resources.ui_render_pass;
    pipeline_create_info.viewport_size.x = (f32)w;
    pipeline_create_info.viewport_size.y = (f32)h;
    pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->ui_pass.pipeline = tgvk_pipeline_create_graphics(&pipeline_create_info);
    h_renderer->ui_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->ui_pass.pipeline);

    tgvk_descriptor_set_update_image(h_renderer->ui_pass.descriptor_set.set, &h_renderer->text.h_font->texture_atlas, 0);
}

static void tg__init_blit_pass(tg_renderer_h h_renderer)
{
    h_renderer->blit_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_command_buffer_begin(&h_renderer->blit_pass.command_buffer, 0);
    {
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_READ);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_WRITE);

        VkImageBlit color_image_blit = { 0 };
        color_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.srcSubresource.mipLevel = 0;
        color_image_blit.srcSubresource.baseArrayLayer = 0;
        color_image_blit.srcSubresource.layerCount = 1;
        color_image_blit.srcOffsets[0].x = 0;
        color_image_blit.srcOffsets[0].y = 0;
        color_image_blit.srcOffsets[0].z = 0;
        color_image_blit.srcOffsets[1].x = h_renderer->render_target.color_attachment.width;
        color_image_blit.srcOffsets[1].y = h_renderer->render_target.color_attachment.height;
        color_image_blit.srcOffsets[1].z = 1;
        color_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_blit.dstSubresource.mipLevel = 0;
        color_image_blit.dstSubresource.baseArrayLayer = 0;
        color_image_blit.dstSubresource.layerCount = 1;
        color_image_blit.dstOffsets[0].x = 0;
        color_image_blit.dstOffsets[0].y = 0;
        color_image_blit.dstOffsets[0].z = 0;
        color_image_blit.dstOffsets[1].x = h_renderer->render_target.color_attachment.width;
        color_image_blit.dstOffsets[1].y = h_renderer->render_target.color_attachment.height;
        color_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment, &h_renderer->render_target.color_attachment_copy, &color_image_blit);

        VkImageBlit depth_image_blit = { 0 };
        depth_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.srcSubresource.mipLevel = 0;
        depth_image_blit.srcSubresource.baseArrayLayer = 0;
        depth_image_blit.srcSubresource.layerCount = 1;
        depth_image_blit.srcOffsets[0].x = 0;
        depth_image_blit.srcOffsets[0].y = 0;
        depth_image_blit.srcOffsets[0].z = 0;
        depth_image_blit.srcOffsets[1].x = h_renderer->render_target.depth_attachment.width;
        depth_image_blit.srcOffsets[1].y = h_renderer->render_target.depth_attachment.height;
        depth_image_blit.srcOffsets[1].z = 1;
        depth_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_image_blit.dstSubresource.mipLevel = 0;
        depth_image_blit.dstSubresource.baseArrayLayer = 0;
        depth_image_blit.dstSubresource.layerCount = 1;
        depth_image_blit.dstOffsets[0].x = 0;
        depth_image_blit.dstOffsets[0].y = 0;
        depth_image_blit.dstOffsets[0].z = 0;
        depth_image_blit.dstOffsets[1].x = h_renderer->render_target.depth_attachment.width;
        depth_image_blit.dstOffsets[1].y = h_renderer->render_target.depth_attachment.height;
        depth_image_blit.dstOffsets[1].z = 1;

        tgvk_cmd_blit_image(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment, &h_renderer->render_target.depth_attachment_copy, &depth_image_blit);

        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        tgvk_cmd_transition_image_layout(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment_copy, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
    }
    TGVK_CALL(vkEndCommandBuffer(h_renderer->blit_pass.command_buffer.command_buffer));
}

static void tg__init_present_pass(tg_renderer_h h_renderer)
{
    tgvk_command_buffers_create(TGVK_COMMAND_POOL_TYPE_PRESENT, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_MAX_SWAPCHAIN_IMAGES, h_renderer->present_pass.p_command_buffers);
    h_renderer->present_pass.image_acquired_semaphore = tgvk_semaphore_create();

    const u32 w = swapchain_extent.width;
    const u32 h = swapchain_extent.height;

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        h_renderer->present_pass.p_framebuffers[i] = tgvk_framebuffer_create(shared_render_resources.present_render_pass, 1, &p_swapchain_image_views[i], w, h);
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

    h_renderer->present_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_renderer->present_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->present_pass.graphics_pipeline);

    const VkDeviceSize vertex_buffer_offset = 0;

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_renderer->render_target.color_attachment.sampler.sampler;
    descriptor_image_info.imageView = h_renderer->render_target.color_attachment.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_renderer->present_pass.descriptor_set.set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    tgvk_descriptor_sets_update(1, &write_descriptor_set);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        tgvk_command_buffer_begin(&h_renderer->present_pass.p_command_buffers[i], 0);
        {
            tgvk_cmd_transition_image_layout(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_READ_F);

            vkCmdBindPipeline(h_renderer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.pipeline);
            vkCmdBindIndexBuffer(h_renderer->present_pass.p_command_buffers[i].command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindVertexBuffers(h_renderer->present_pass.p_command_buffers[i].command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(h_renderer->present_pass.p_command_buffers[i].command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindDescriptorSets(h_renderer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->present_pass.descriptor_set.set, 0, TG_NULL);

            tgvk_cmd_begin_render_pass(&h_renderer->present_pass.p_command_buffers[i], shared_render_resources.present_render_pass, &h_renderer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            tgvk_cmd_draw_indexed(&h_renderer->present_pass.p_command_buffers[i], 6);
            vkCmdEndRenderPass(h_renderer->present_pass.p_command_buffers[i].command_buffer);

            tgvk_cmd_transition_image_layout(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment, TGVK_LAYOUT_SHADER_READ_F, TGVK_LAYOUT_TRANSFER_WRITE);
            tgvk_cmd_transition_image_layout(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_WRITE);

            tgvk_cmd_clear_image(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment);
            tgvk_cmd_clear_image(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.depth_attachment);

            tgvk_cmd_transition_image_layout(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
            tgvk_cmd_transition_image_layout(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
        }
        TGVK_CALL(vkEndCommandBuffer(h_renderer->present_pass.p_command_buffers[i].command_buffer));
    }
}

static void tg__init_clear_pass(tg_renderer_h h_renderer)
{
    h_renderer->clear_pass.command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgvk_command_buffer_begin(&h_renderer->clear_pass.command_buffer, 0);
    {
        tgvk_cmd_transition_image_layout(&h_renderer->clear_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE, TGVK_LAYOUT_TRANSFER_WRITE);
        tgvk_cmd_clear_image(&h_renderer->clear_pass.command_buffer, &h_renderer->render_target.depth_attachment);
        tgvk_cmd_transition_image_layout(&h_renderer->clear_pass.command_buffer, &h_renderer->render_target.depth_attachment, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
    }
    TGVK_CALL(vkEndCommandBuffer(h_renderer->clear_pass.command_buffer.command_buffer));
}

void tg__destroy_present_pass(tg_renderer_h h_renderer)
{
    tgvk_command_buffers_destroy(TG_MAX_SWAPCHAIN_IMAGES, h_renderer->present_pass.p_command_buffers);
    tgvk_descriptor_set_destroy(&h_renderer->present_pass.descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->present_pass.graphics_pipeline);
    tgvk_framebuffers_destroy(TG_MAX_SWAPCHAIN_IMAGES, h_renderer->present_pass.p_framebuffers);
    tgvk_semaphore_destroy(h_renderer->present_pass.image_acquired_semaphore);
}





tg_renderer_h tg_renderer_create(tg_camera* p_camera)
{
    TG_ASSERT(p_camera);

    tg_renderer_h h_renderer = tgvk_handle_take(TG_STRUCTURE_TYPE_RENDERER);
    h_renderer->p_camera = p_camera;
    h_renderer->view_projection_ubo = TGVK_BUFFER_CREATE_UBO(2 * sizeof(m4));

    h_renderer->hdr_color_attachment = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE, swapchain_extent.width, swapchain_extent.height, TGVK_HDR_FORMAT, TG_NULL);

    h_renderer->render_target = TGVK_RENDER_TARGET_CREATE(
        swapchain_extent.width, swapchain_extent.height, TGVK_HDR_FORMAT, TG_NULL,
        swapchain_extent.width, swapchain_extent.height, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );

    h_renderer->semaphore = tgvk_semaphore_create();

    h_renderer->deferred_command_buffer_count = 0;
    h_renderer->forward_render_command_count = 0;

    h_renderer->text.h_font = tgvk_font_create("fonts/arial.ttf");
    h_renderer->text.capacity = 32;
    h_renderer->text.count = 0;
    h_renderer->text.total_letter_count = 0;
    h_renderer->text.p_string_capacities = TG_MALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.p_string_capacities));
    h_renderer->text.pp_strings = TG_MALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.pp_strings));

    tgvk_atmosphere_model_create(&h_renderer->model);
    tg__init_geometry_pass(h_renderer);
    tg__init_shading_pass(h_renderer);
    tg__init_forward_pass(h_renderer);
    tg__init_ray_tracing_pass(h_renderer);
    tg__init_tone_mapping_pass(h_renderer);
    tg__init_ui_pass(h_renderer);
    tg__init_blit_pass(h_renderer);
    tg__init_present_pass(h_renderer);
    tg__init_clear_pass(h_renderer);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    {
        tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->geometry_pass.p_color_attachments[i], TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
    }
    tgvk_command_buffer_end_and_submit(p_command_buffer);


#if TG_ENABLE_DEBUG_TOOLS == 1
    h_renderer->DEBUG.p_cubes = TG_MALLOC(TG_DEBUG_MAX_CUBES * sizeof(*h_renderer->DEBUG.p_cubes));

    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(24 * sizeof(v3));
    v3* p_pit = (v3*)p_staging_buffer->memory.p_mapped_device_memory;

    *p_pit++ = (v3) { -0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) { -0.5f, -0.5f,  0.5f };
    *p_pit++ = (v3) { -0.5f,  0.5f,  0.5f };
    *p_pit++ = (v3) { -0.5f,  0.5f, -0.5f };

    *p_pit++ = (v3) {  0.5f, -0.5f,  0.5f };
    *p_pit++ = (v3) {  0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f, -0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f,  0.5f };
    
    *p_pit++ = (v3) { -0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) {  0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) {  0.5f, -0.5f,  0.5f };
    *p_pit++ = (v3) { -0.5f, -0.5f,  0.5f };
    
    *p_pit++ = (v3) { -0.5f,  0.5f,  0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f,  0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f, -0.5f };
    *p_pit++ = (v3) { -0.5f,  0.5f, -0.5f };
    
    *p_pit++ = (v3) {  0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) { -0.5f, -0.5f, -0.5f };
    *p_pit++ = (v3) { -0.5f,  0.5f, -0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f, -0.5f };
    
    *p_pit++ = (v3) { -0.5f, -0.5f,  0.5f };
    *p_pit++ = (v3) {  0.5f, -0.5f,  0.5f };
    *p_pit++ = (v3) {  0.5f,  0.5f,  0.5f };
    *p_pit++ = (v3) { -0.5f,  0.5f,  0.5f };

    tgvk_buffer_flush_host_to_device(p_staging_buffer);
    h_renderer->DEBUG.cube_position_buffer = TGVK_BUFFER_CREATE(24 * sizeof(v3), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    tgvk_buffer_copy(24 * sizeof(v3), p_staging_buffer, &h_renderer->DEBUG.cube_position_buffer);

    u16* p_iit = (u16*)p_staging_buffer->memory.p_mapped_device_memory;
    for (u8 i = 0; i < 6; i++)
    {
        *p_iit++ = 4 * i + 0;
        *p_iit++ = 4 * i + 1;
        *p_iit++ = 4 * i + 2;
        *p_iit++ = 4 * i + 2;
        *p_iit++ = 4 * i + 3;
        *p_iit++ = 4 * i + 0;
    }

    tgvk_buffer_flush_host_to_device(p_staging_buffer);
    h_renderer->DEBUG.cube_index_buffer = TGVK_BUFFER_CREATE(36 * sizeof(u16), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    tgvk_buffer_copy(36 * sizeof(u16), p_staging_buffer, &h_renderer->DEBUG.cube_index_buffer);

    tgvk_global_staging_buffer_release();
#endif

    return h_renderer;
}

void tg_renderer_destroy(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

#if TG_ENABLE_DEBUG_TOOLS == 1
    TG_INVALID_CODEPATH();
#endif

    tg__destroy_present_pass(h_renderer);

    tgvk_command_buffer_destroy(&h_renderer->clear_pass.command_buffer);

    tgvk_command_buffer_destroy(&h_renderer->blit_pass.command_buffer);

    tgvk_descriptor_set_destroy(&h_renderer->ui_pass.descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->ui_pass.pipeline);
    tgvk_framebuffer_destroy(&h_renderer->ui_pass.framebuffer);
    tgvk_command_buffer_destroy(&h_renderer->ui_pass.command_buffer);

    tgvk_descriptor_set_destroy(&h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline);
    tgvk_framebuffer_destroy(&h_renderer->tone_mapping_pass.adapt_exposure_framebuffer);
    tgvk_buffer_destroy(&h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo);
    tgvk_buffer_destroy(&h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer);
    tgvk_descriptor_set_destroy(&h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline);
    tgvk_buffer_destroy(&h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer);
    tgvk_descriptor_set_destroy(&h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline);
    tgvk_buffer_destroy(&h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer);
    tgvk_command_buffer_destroy(&h_renderer->tone_mapping_pass.command_buffer);

    tgvk_buffer_destroy(&h_renderer->raytrace_pass.ubo);
    tgvk_descriptor_set_destroy(&h_renderer->raytrace_pass.descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->raytrace_pass.pipeline);
    tgvk_command_buffer_destroy(&h_renderer->raytrace_pass.command_buffer);

    tgvk_framebuffer_destroy(&h_renderer->forward_pass.framebuffer);
    tgvk_command_buffer_destroy(&h_renderer->forward_pass.command_buffer);

    tgvk_buffer_destroy(&h_renderer->shading_pass.ubo);
    tgvk_descriptor_set_destroy(&h_renderer->shading_pass.descriptor_set);
    tgvk_pipeline_destroy(&h_renderer->shading_pass.graphics_pipeline);
    tgvk_shader_destroy(&h_renderer->shading_pass.fragment_shader);
    tgvk_framebuffer_destroy(&h_renderer->shading_pass.framebuffer);
    tgvk_command_buffer_destroy(&h_renderer->shading_pass.command_buffer);

    tgvk_framebuffer_destroy(&h_renderer->geometry_pass.framebuffer);
    for (u32 i = 0; i < TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT; i++)
    {
        tgvk_image_destroy(&h_renderer->geometry_pass.p_color_attachments[i]);
    }
    tgvk_command_buffer_destroy(&h_renderer->geometry_pass.command_buffer);

    tgvk_atmosphere_model_destroy(&h_renderer->model);

    if (h_renderer->text.capacity > 0)
    {
        for (u32 i = 0; i < h_renderer->text.capacity; i++)
        {
            if (!h_renderer->text.pp_strings[i])
            {
                break;
            }
            TG_FREE(h_renderer->text.pp_strings[i]);
        }
        TG_FREE(h_renderer->text.pp_strings);
        TG_FREE(h_renderer->text.p_string_capacities);
    }
    tgvk_font_destroy(h_renderer->text.h_font);
    tgvk_semaphore_destroy(h_renderer->semaphore);
    tgvk_render_target_destroy(&h_renderer->render_target);
    tgvk_image_destroy(&h_renderer->hdr_color_attachment);
    tgvk_buffer_destroy(&h_renderer->view_projection_ubo);
    tgvk_handle_release(h_renderer);
}

void tg_renderer_begin(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    h_renderer->deferred_command_buffer_count = 0;
    h_renderer->forward_render_command_count = 0;
    h_renderer->text.count = 0;
    h_renderer->text.total_letter_count = 0;
#if TG_ENABLE_DEBUG_TOOLS == 1
    h_renderer->DEBUG.cube_count = 0;
#endif
}

void tg_renderer_set_sun_direction(tg_renderer_h h_renderer, v3 direction)
{
    TG_ASSERT(h_renderer && tgm_v3_magsqr(direction));

    TGVK_SHADING_UBO.sun_direction.xyz = direction;
}

void tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(direction) && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_shading_data_ubo* p_shading_ubo = &TGVK_SHADING_UBO;

    p_shading_ubo->p_directional_light_directions[p_shading_ubo->directional_light_count].x = direction.x;
    p_shading_ubo->p_directional_light_directions[p_shading_ubo->directional_light_count].y = direction.y;
    p_shading_ubo->p_directional_light_directions[p_shading_ubo->directional_light_count].z = direction.z;
    p_shading_ubo->p_directional_light_directions[p_shading_ubo->directional_light_count].w = 0.0f;

    p_shading_ubo->p_directional_light_colors[p_shading_ubo->directional_light_count].x = color.x;
    p_shading_ubo->p_directional_light_colors[p_shading_ubo->directional_light_count].y = color.y;
    p_shading_ubo->p_directional_light_colors[p_shading_ubo->directional_light_count].z = color.z;
    p_shading_ubo->p_directional_light_colors[p_shading_ubo->directional_light_count].w = 1.0f;

    p_shading_ubo->directional_light_count++;
}

void tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_shading_data_ubo* p_shading_ubo = &TGVK_SHADING_UBO;

    p_shading_ubo->p_point_light_positions[p_shading_ubo->point_light_count].x = position.x;
    p_shading_ubo->p_point_light_positions[p_shading_ubo->point_light_count].y = position.y;
    p_shading_ubo->p_point_light_positions[p_shading_ubo->point_light_count].z = position.z;
    p_shading_ubo->p_point_light_positions[p_shading_ubo->point_light_count].w = 0.0f;

    p_shading_ubo->p_point_light_colors[p_shading_ubo->point_light_count].x = color.x;
    p_shading_ubo->p_point_light_colors[p_shading_ubo->point_light_count].y = color.y;
    p_shading_ubo->p_point_light_colors[p_shading_ubo->point_light_count].z = color.z;
    p_shading_ubo->p_point_light_colors[p_shading_ubo->point_light_count].w = 1.0f;

    p_shading_ubo->point_light_count++;
}

void tg_renderer_push_render_command(tg_renderer_h h_renderer, tg_render_command_h h_render_command)
{
    TG_ASSERT(h_renderer && h_render_command);

    if (h_render_command->h_material->material_type == TGVK_MATERIAL_TYPE_DEFERRED)
    {
        TG_ASSERT(h_renderer->deferred_command_buffer_count < TG_MAX_RENDER_COMMANDS);

        tgvk_render_command_renderer_info* p_renderer_info = TG_NULL;
        for (u32 i = 0; i < h_render_command->renderer_info_count; i++)
        {
            if (h_render_command->p_renderer_infos[i].h_renderer == h_renderer)
            {
                p_renderer_info = &h_render_command->p_renderer_infos[i];
                h_renderer->p_deferred_command_buffers[h_renderer->deferred_command_buffer_count++] = p_renderer_info->command_buffer.buffer;
                break;
            }
        }
        // TODO: the terrain could still be in the process of building a render command. thus, this would fail!
        //TG_ASSERT(p_renderer_info);
    }
    else
    {
        TG_ASSERT(h_renderer->forward_render_command_count < TG_MAX_RENDER_COMMANDS);
        h_renderer->ph_forward_render_commands[h_renderer->forward_render_command_count++] = h_render_command;
    }
}

void tg_renderer_push_terrain(tg_renderer_h h_renderer, tg_rtvx_terrain_h h_terrain)
{
    TG_ASSERT(h_renderer && h_terrain);

    h_renderer->h_terrain = h_terrain;
}

void tg_renderer_push_text(tg_renderer_h h_renderer, const char* p_text)
{
    TG_ASSERT(h_renderer && p_text);

    if (h_renderer->text.count + 1 >= h_renderer->text.capacity)
    {
        if (h_renderer->text.capacity == 0)
        {
            h_renderer->text.capacity = 32;
            h_renderer->text.p_string_capacities = TG_MALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.p_string_capacities));
            h_renderer->text.pp_strings = TG_MALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.pp_strings));
        }
        else
        {
            h_renderer->text.p_string_capacities = TG_REALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.p_string_capacities), h_renderer->text.p_string_capacities);
            h_renderer->text.pp_strings = TG_REALLOC(h_renderer->text.capacity * sizeof(*h_renderer->text.pp_strings), h_renderer->text.pp_strings);
        }
    }

    const tg_size string_size = tg_strsize(p_text);
    if (string_size > h_renderer->text.p_string_capacities[h_renderer->text.count])
    {
        if (h_renderer->text.p_string_capacities[h_renderer->text.count] == 0)
        {
            TG_ASSERT(h_renderer->text.pp_strings[h_renderer->text.count] == TG_NULL);
            h_renderer->text.pp_strings[h_renderer->text.count] = TG_MALLOC(string_size);
        }
        else
        {
            TG_ASSERT(h_renderer->text.pp_strings[h_renderer->text.count] != TG_NULL);
            h_renderer->text.pp_strings[h_renderer->text.count] = TG_REALLOC(string_size, h_renderer->text.pp_strings[h_renderer->text.count]);
        }
        h_renderer->text.p_string_capacities[h_renderer->text.count] = (u32)string_size;
    }

    tg_strcpy(string_size, h_renderer->text.pp_strings[h_renderer->text.count], p_text);
    h_renderer->text.count++;
    h_renderer->text.total_letter_count += (u32)(string_size - 1);
}

void tg_renderer_end(tg_renderer_h h_renderer, f32 dt, b32 present)
{
    TG_ASSERT(h_renderer);
    
    const tg_camera c = *h_renderer->p_camera;

    const m4 r = tgm_m4_inverse(tgm_m4_euler(c.pitch, c.yaw, c.roll));
    const m4 t = tgm_m4_translate(tgm_v3_neg(c.position));
    const m4 v = tgm_m4_mul(r, t);
    const m4 iv = tgm_m4_inverse(v);
    const m4 p = c.type == TG_CAMERA_TYPE_ORTHOGRAPHIC ? tgm_m4_orthographic(c.ortho.l, c.ortho.r, c.ortho.b, c.ortho.t, c.ortho.f, c.ortho.n) : tgm_m4_perspective(c.persp.fov_y_in_radians, c.persp.aspect, c.persp.n, c.persp.f);
    const m4 ip = tgm_m4_inverse(p);
    const m4 vp = tgm_m4_mul(p, v);
    const m4 ivp = tgm_m4_inverse(vp);

    TGVK_CAMERA_VIEW(h_renderer->view_projection_ubo) = v;
    TGVK_CAMERA_PROJ(h_renderer->view_projection_ubo) = p;

    tg_shading_data_ubo* p_shading_ubo = &TGVK_SHADING_UBO;
    p_shading_ubo->camera_position.xyz = c.position;
    p_shading_ubo->ivp = ivp;

    tgvk_atmosphere_model_update(&h_renderer->model, iv, ip);

    tgvk_command_buffer_begin(&h_renderer->geometry_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
    tgvk_cmd_begin_render_pass(&h_renderer->geometry_pass.command_buffer, shared_render_resources.geometry_render_pass, &h_renderer->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(h_renderer->geometry_pass.command_buffer.command_buffer, h_renderer->deferred_command_buffer_count, h_renderer->p_deferred_command_buffers);
    vkCmdEndRenderPass(h_renderer->geometry_pass.command_buffer.command_buffer);
    TGVK_CALL(vkEndCommandBuffer(h_renderer->geometry_pass.command_buffer.command_buffer));

    tgvk_fence_wait(h_renderer->render_target.fence); // TODO: isn't this wrong? this means that some buffers are potentially updates too early
    tgvk_fence_reset(h_renderer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_renderer->geometry_pass.command_buffer.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);

    const VkPipelineStageFlags color_attachment_pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    shading_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &h_renderer->shading_pass.command_buffer.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);

    tgvk_command_buffer_begin(&h_renderer->forward_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
    {
        tgvk_cmd_begin_render_pass(&h_renderer->forward_pass.command_buffer, shared_render_resources.forward_render_pass, &h_renderer->forward_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        for (u32 i = 0; i < h_renderer->forward_render_command_count; i++) // TODO: are these not sorted at all?
        {
            tg_render_command_h h_render_command = h_renderer->ph_forward_render_commands[i];
            if (h_render_command->h_material->material_type == TGVK_MATERIAL_TYPE_FORWARD)
            {
                for (u32 j = 0; j < h_render_command->renderer_info_count; j++)
                {
                    if (h_render_command->p_renderer_infos[j].h_renderer == h_renderer)
                    {
                        vkCmdExecuteCommands(h_renderer->forward_pass.command_buffer.command_buffer, 1, &h_render_command->p_renderer_infos[j].command_buffer.command_buffer);
                        break;
                    }
                }
            }
        }
    
#if TG_ENABLE_DEBUG_TOOLS == 1
        for (u32 i = 0; i < h_renderer->DEBUG.cube_count; i++)
        {
            tgvk_descriptor_set_update_uniform_buffer(h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, &h_renderer->DEBUG.p_cubes[i].ubo, 0);
            tgvk_descriptor_set_update_uniform_buffer(h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, &h_renderer->view_projection_ubo, 1);

            VkCommandBufferInheritanceInfo DEBUG_command_buffer_inheritance_info = { 0 };
            DEBUG_command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            DEBUG_command_buffer_inheritance_info.pNext = TG_NULL;
            DEBUG_command_buffer_inheritance_info.renderPass = shared_render_resources.forward_render_pass;
            DEBUG_command_buffer_inheritance_info.subpass = 0;
            DEBUG_command_buffer_inheritance_info.framebuffer = h_renderer->forward_pass.framebuffer.framebuffer;
            DEBUG_command_buffer_inheritance_info.occlusionQueryEnable = TG_FALSE;
            DEBUG_command_buffer_inheritance_info.queryFlags = 0;
            DEBUG_command_buffer_inheritance_info.pipelineStatistics = 0;

            command_buffer_begin_info.pInheritanceInfo = &DEBUG_command_buffer_inheritance_info;
            TGVK_CALL(vkBeginCommandBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, &command_buffer_begin_info));
            {
                command_buffer_begin_info.pInheritanceInfo = TG_NULL;

                vkCmdBindPipeline(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->DEBUG.p_cubes[i].graphics_pipeline.pipeline);
                const VkDeviceSize DEBUG_vertex_buffer_offset = 0;
                vkCmdBindIndexBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, h_renderer->DEBUG.cube_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindVertexBuffers(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, 0, 1, &h_renderer->DEBUG.cube_position_buffer.buffer, &DEBUG_vertex_buffer_offset);
                vkCmdBindDescriptorSets(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->DEBUG.p_cubes[i].graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, 0, TG_NULL);
                tgvk_cmd_draw_indexed(&h_renderer->DEBUG.p_cubes[i].command_buffer, 36);
            }
            TGVK_CALL(vkEndCommandBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer));
            vkCmdExecuteCommands(h_renderer->forward_pass.command_buffer.command_buffer, 1, &h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer);
        }
#endif

        vkCmdEndRenderPass(h_renderer->forward_pass.command_buffer.command_buffer);
    }
    TGVK_CALL(vkEndCommandBuffer(h_renderer->forward_pass.command_buffer.command_buffer));

    VkSubmitInfo forward_submit_info = { 0 };
    forward_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    forward_submit_info.pNext = TG_NULL;
    forward_submit_info.waitSemaphoreCount = 1;
    forward_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    forward_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    forward_submit_info.commandBufferCount = 1;
    forward_submit_info.pCommandBuffers = &h_renderer->forward_pass.command_buffer.command_buffer;
    forward_submit_info.signalSemaphoreCount = 1;
    forward_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &forward_submit_info, VK_NULL_HANDLE);

    if (h_renderer->h_terrain)
    {
        const m4 ivp_no_translation = tgm_m4_inverse(tgm_m4_mul(p, r));

        tg_raytracer_ubo* p_ubo = h_renderer->raytrace_pass.ubo.memory.p_mapped_device_memory;
        p_ubo->camera.xyz = c.position;
        p_ubo->ray00.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f,  1.0f,  1.0f,  1.0f }).xyz);
        p_ubo->ray10.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) { -1.0f, -1.0f,  1.0f,  1.0f }).xyz);
        p_ubo->ray01.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f,  1.0f,  1.0f,  1.0f }).xyz);
        p_ubo->ray11.xyz = tgm_v3_normalized(tgm_m4_mulv4(ivp_no_translation, (v4) {  1.0f, -1.0f,  1.0f,  1.0f }).xyz);

        tgvk_descriptor_set_update_image_3d(h_renderer->raytrace_pass.descriptor_set.set, &h_renderer->h_terrain->voxels, 0);
        tgvk_descriptor_set_update_uniform_buffer(h_renderer->raytrace_pass.descriptor_set.set, &h_renderer->raytrace_pass.ubo, 1);
        tgvk_descriptor_set_update_image2(h_renderer->raytrace_pass.descriptor_set.set, &h_renderer->hdr_color_attachment, 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        //tgvk_descriptor_set_update_image2(h_renderer->raytrace_pass.descriptor_set.descriptor_set, &h_renderer->render_target.depth_attachment, 3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        tgvk_command_buffer_begin(&h_renderer->raytrace_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            tgvk_cmd_transition_image_layout(&h_renderer->raytrace_pass.command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE, TGVK_LAYOUT_SHADER_WRITE_C);
            tgvk_cmd_bind_pipeline(&h_renderer->raytrace_pass.command_buffer, &h_renderer->raytrace_pass.pipeline);
            tgvk_cmd_bind_descriptor_set(&h_renderer->raytrace_pass.command_buffer, &h_renderer->raytrace_pass.pipeline, &h_renderer->raytrace_pass.descriptor_set);
            vkCmdDispatch(h_renderer->raytrace_pass.command_buffer.command_buffer, h_renderer->hdr_color_attachment.width / 8, h_renderer->hdr_color_attachment.height / 8, 1);
            tgvk_cmd_transition_image_layout(&h_renderer->raytrace_pass.command_buffer, &h_renderer->hdr_color_attachment, TGVK_LAYOUT_SHADER_WRITE_C, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
        }
        vkEndCommandBuffer(h_renderer->raytrace_pass.command_buffer.command_buffer);

        VkSubmitInfo raytrace_submit_info = { 0 };
        raytrace_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        raytrace_submit_info.pNext = TG_NULL;
        raytrace_submit_info.waitSemaphoreCount = 1;
        raytrace_submit_info.pWaitSemaphores = &h_renderer->semaphore;
        raytrace_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
        raytrace_submit_info.commandBufferCount = 1;
        raytrace_submit_info.pCommandBuffers = &h_renderer->raytrace_pass.command_buffer.command_buffer;
        raytrace_submit_info.signalSemaphoreCount = 1;
        raytrace_submit_info.pSignalSemaphores = &h_renderer->semaphore;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &raytrace_submit_info, VK_NULL_HANDLE);
    }

    VkSubmitInfo tone_mapping_submit_info = { 0 };
    tone_mapping_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    tone_mapping_submit_info.pNext = TG_NULL;
    tone_mapping_submit_info.waitSemaphoreCount = 1;
    tone_mapping_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    tone_mapping_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    tone_mapping_submit_info.commandBufferCount = 1;
    tone_mapping_submit_info.pCommandBuffers = &h_renderer->tone_mapping_pass.command_buffer.command_buffer;
    tone_mapping_submit_info.signalSemaphoreCount = 1;
    tone_mapping_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    *(f32*)h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo.memory.p_mapped_device_memory = dt;
    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &tone_mapping_submit_info, VK_NULL_HANDLE);

    if (h_renderer->text.total_letter_count > 0)
    {
        const u32 vertex_count = 6 * h_renderer->text.total_letter_count;
        const tg_size positions_size = (tg_size)vertex_count * (2LL * sizeof(f32));
        const tg_size uvs_size = (tg_size)vertex_count * (2LL * sizeof(f32));
        tgvk_buffer ui_positions_buffer = TGVK_BUFFER_CREATE(positions_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_HOST);
        tgvk_buffer ui_uvs_buffer = TGVK_BUFFER_CREATE(uvs_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_HOST);

        const f32 w = (f32)h_renderer->render_target.color_attachment.width;
        const f32 h = (f32)h_renderer->render_target.color_attachment.height;
        const u32 text_count = h_renderer->text.count;
        f32* p_positions_it = (f32*)ui_positions_buffer.memory.p_mapped_device_memory;
        f32* p_uvs_it = (f32*)ui_uvs_buffer.memory.p_mapped_device_memory;
        f32 y_offset = 0.0f;
        u32 render_letter_count = 0;
        for (u32 text_idx = 0; text_idx < text_count; text_idx++)
        {
            f32 x_offset = 0.0f;
            const unsigned char* p_text_it = (unsigned char*)h_renderer->text.pp_strings[text_idx];
            while (*p_text_it != '\0')
            {
                const u8 glyph_idx = h_renderer->text.h_font->p_char_to_glyph[*p_text_it++];
                const struct tg_font_glyph* p_glyph = &h_renderer->text.h_font->p_glyphs[glyph_idx];

                const f32 x0 = x_offset + p_glyph->left_side_bearing;
                const f32 x1 = x0 + p_glyph->size.x;
                const f32 y0 = y_offset + p_glyph->bottom_side_bearing;
                const f32 y1 = y0 + p_glyph->size.y;

                if (x0 != x1 && y0 != y1)
                {
                    render_letter_count++;

                    const f32 rel_x0 = (x0 / w) * 2.0f - 1.0f;
                    const f32 rel_x1 = (x1 / w) * 2.0f - 1.0f;
                    const f32 rel_y0 = -((y0 / h) * 2.0f - 1.0f);
                    const f32 rel_y1 = -((y1 / h) * 2.0f - 1.0f);

                    *p_positions_it++ = rel_x0;
                    *p_positions_it++ = rel_y0;
                    *p_positions_it++ = rel_x1;
                    *p_positions_it++ = rel_y0;
                    *p_positions_it++ = rel_x1;
                    *p_positions_it++ = rel_y1;
                    *p_positions_it++ = rel_x1;
                    *p_positions_it++ = rel_y1;
                    *p_positions_it++ = rel_x0;
                    *p_positions_it++ = rel_y1;
                    *p_positions_it++ = rel_x0;
                    *p_positions_it++ = rel_y0;

                    *p_uvs_it++ = p_glyph->uv_min.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_min.y;
                    *p_uvs_it++ = p_glyph->uv_max.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_min.y;
                    *p_uvs_it++ = p_glyph->uv_max.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_max.y;
                    *p_uvs_it++ = p_glyph->uv_max.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_max.y;
                    *p_uvs_it++ = p_glyph->uv_min.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_max.y;
                    *p_uvs_it++ = p_glyph->uv_min.x;
                    *p_uvs_it++ = 1.0f - p_glyph->uv_min.y;
                }

                x_offset += p_glyph->advance_width;
            }
            y_offset += h_renderer->text.h_font->max_glyph_height;
        }

        tgvk_command_buffer_begin(&h_renderer->ui_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindPipeline(h_renderer->ui_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ui_pass.pipeline.pipeline);
        vkCmdBindVertexBuffers(h_renderer->ui_pass.command_buffer.command_buffer, 0, 1, &ui_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->ui_pass.command_buffer.command_buffer, 1, 1, &ui_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->ui_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ui_pass.pipeline.layout.pipeline_layout, 0, 1, &h_renderer->ui_pass.descriptor_set.set, 0, TG_NULL);
        tgvk_cmd_begin_render_pass(&h_renderer->ui_pass.command_buffer, shared_render_resources.ui_render_pass, &h_renderer->ui_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDraw(h_renderer->ui_pass.command_buffer.command_buffer, 6 * render_letter_count, 1, 0, 0);
        vkCmdEndRenderPass(h_renderer->ui_pass.command_buffer.command_buffer);
        vkEndCommandBuffer(h_renderer->ui_pass.command_buffer.command_buffer);

        VkSubmitInfo ui_submit_info = { 0 };
        ui_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ui_submit_info.pNext = TG_NULL;
        ui_submit_info.waitSemaphoreCount = 1;
        ui_submit_info.pWaitSemaphores = &h_renderer->semaphore;
        ui_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
        ui_submit_info.commandBufferCount = 1;
        ui_submit_info.pCommandBuffers = &h_renderer->ui_pass.command_buffer.command_buffer;
        ui_submit_info.signalSemaphoreCount = 1;
        ui_submit_info.pSignalSemaphores = &h_renderer->semaphore;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &ui_submit_info, h_renderer->render_target.fence);
        tgvk_fence_wait(h_renderer->render_target.fence);
        tgvk_fence_reset(h_renderer->render_target.fence);
        tgvk_buffer_destroy(&ui_uvs_buffer);
        tgvk_buffer_destroy(&ui_positions_buffer);
    }

    VkSubmitInfo blit_submit_info = { 0 };
    blit_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    blit_submit_info.pNext = TG_NULL;
    blit_submit_info.waitSemaphoreCount = 1;
    blit_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    blit_submit_info.pWaitDstStageMask = &color_attachment_pipeline_stage_flags;
    blit_submit_info.commandBufferCount = 1;
    blit_submit_info.pCommandBuffers = &h_renderer->blit_pass.command_buffer.command_buffer;

    if (!present)
    {
        blit_submit_info.signalSemaphoreCount = 0;
        blit_submit_info.pSignalSemaphores = TG_NULL;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &blit_submit_info, h_renderer->render_target.fence);
    }
    else
    {
        blit_submit_info.signalSemaphoreCount = 1;
        blit_submit_info.pSignalSemaphores = &h_renderer->semaphore;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &blit_submit_info, VK_NULL_HANDLE);

        u32 current_image;
        TGVK_CALL(vkAcquireNextImageKHR(device, swapchain, TG_U64_MAX, h_renderer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

        const VkSemaphore p_wait_semaphores[2] = { h_renderer->semaphore, h_renderer->present_pass.image_acquired_semaphore };
        const VkPipelineStageFlags p_pipeline_stage_masks[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

        VkSubmitInfo draw_submit_info = { 0 };
        draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        draw_submit_info.pNext = TG_NULL;
        draw_submit_info.waitSemaphoreCount = 2;
        draw_submit_info.pWaitSemaphores = p_wait_semaphores;
        draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
        draw_submit_info.commandBufferCount = 1;
        draw_submit_info.pCommandBuffers = &h_renderer->present_pass.p_command_buffers[current_image].command_buffer;
        draw_submit_info.signalSemaphoreCount = 1;
        draw_submit_info.pSignalSemaphores = &h_renderer->semaphore;

        tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &draw_submit_info, h_renderer->render_target.fence);

        VkPresentInfoKHR present_info = { 0 };
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = TG_NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &h_renderer->semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = TG_NULL;

        tgvk_queue_present(&present_info);
    }
}

void tg_renderer_clear(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    tgvk_fence_wait(h_renderer->render_target.fence);
    tgvk_fence_reset(h_renderer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_renderer->clear_pass.command_buffer.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_renderer->render_target.fence);

    // TODO: forward renderer
    tg_shading_data_ubo* p_shading_ubo = &TGVK_SHADING_UBO;

    p_shading_ubo->directional_light_count = 0;
    p_shading_ubo->point_light_count = 0;
}

tg_render_target_h tg_renderer_get_render_target(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    return &h_renderer->render_target;
}

v3 tg_renderer_screen_to_world_position(tg_renderer_h h_renderer, u32 x, u32 y)
{
    TG_ASSERT(h_renderer && x < h_renderer->render_target.depth_attachment_copy.width && y < h_renderer->render_target.depth_attachment_copy.height);

    v3 result = { 0 };

    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(sizeof(f32));

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->render_target.depth_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_READ);
    tgvk_cmd_copy_depth_image_pixel_to_buffer(p_command_buffer, &h_renderer->render_target.depth_attachment_copy, p_staging_buffer, x, y);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->render_target.depth_attachment_copy, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_SHADER_READ_CFV);
    TGVK_CALL(vkEndCommandBuffer(p_command_buffer->buffer));

    tgvk_fence_wait(h_renderer->render_target.fence);
    tgvk_fence_reset(h_renderer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_command_buffer->buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_renderer->render_target.fence);

    tgvk_fence_wait(h_renderer->render_target.fence);
    const f32 depth = *(f32*)p_staging_buffer->memory.p_mapped_device_memory;
    tgvk_global_staging_buffer_release();

    const f32 rel_x = ((f32)x / (f32)h_renderer->render_target.depth_attachment_copy.width) * 2.0f - 1.0f;
    const f32 rel_y = ((f32)y / (f32)h_renderer->render_target.depth_attachment_copy.height) * 2.0f - 1.0f;
    
    const v4 screen = { rel_x, rel_y, depth, 1.0f };
    // TODO: cache the ivp in the renderer instead of calculating it here again
    const m4 vp = tgm_m4_mul(TGVK_CAMERA_PROJ(h_renderer->view_projection_ubo), TGVK_CAMERA_VIEW(h_renderer->view_projection_ubo));
    const m4 ivp = tgm_m4_inverse(vp);
    const v4 world = tgm_m4_mulv4(ivp, screen);
    result = tgm_v4_to_v3(world);

    return result;
}

void tg_renderer_screenshot(tg_renderer_h h_renderer, const char* p_filename)
{
    TG_ASSERT(h_renderer && p_filename);

    tgvk_fence_wait(h_renderer->render_target.fence);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->render_target.color_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_READ);
    tgvk_command_buffer_end_and_submit(p_command_buffer);

    tgvk_image_store_to_disc(&h_renderer->render_target.color_attachment_copy, p_filename, TG_FALSE, TG_FALSE);

    tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_transition_image_layout(p_command_buffer, &h_renderer->render_target.color_attachment_copy, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_SHADER_READ_CFV);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
}

#if TG_ENABLE_DEBUG_TOOLS == 1
void tg_renderer_draw_cube_DEBUG(tg_renderer_h h_renderer, v3 position, v3 scale, v4 color)
{
    TG_ASSERT(h_renderer && h_renderer->DEBUG.cube_count < TG_DEBUG_MAX_CUBES);

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.buffer == VK_NULL_HANDLE)
    {
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo = TGVK_BUFFER_CREATE_UBO(sizeof(m4) + sizeof(v4));
    }

    *((m4*)h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.memory.p_mapped_device_memory) = tgm_m4_mul(tgm_m4_translate(position), tgm_m4_scale(scale));
    *(v4*)(((m4*)h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.memory.p_mapped_device_memory) + 1) = color;
    tgvk_buffer_flush_host_to_device(&h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo);

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline.pipeline == VK_NULL_HANDLE)
    {
        const tg_blend_mode blend_mode = TG_BLEND_MODE_BLEND;

        tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
        graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_create("shaders/renderer/debug/forward.vert")->shader;
        graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_create("shaders/renderer/debug/forward.frag")->shader;
        graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
        graphics_pipeline_create_info.depth_test_enable = TG_TRUE;
        graphics_pipeline_create_info.depth_write_enable = TG_TRUE;
        graphics_pipeline_create_info.p_blend_modes = &blend_mode;
        graphics_pipeline_create_info.render_pass = shared_render_resources.forward_render_pass;
        graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->forward_pass.framebuffer.width;
        graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->forward_pass.framebuffer.height;
        graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_LINE;

        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].descriptor_set = tgvk_descriptor_set_create(&h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline);
    }

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].command_buffer.command_buffer == VK_NULL_HANDLE)
    {
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    }

    h_renderer->DEBUG.cube_count++;
}
#endif

void tgvk_renderer_on_window_resize(tg_renderer_h h_renderer, u32 width, u32 height)
{
    TG_ASSERT(h_renderer && width && height);

    tg__destroy_present_pass(h_renderer);
    tg__init_present_pass(h_renderer);
}

#endif
#endif
