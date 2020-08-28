#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_assets.h"



#define TG_CAMERA_VIEW(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[0])
#define TG_CAMERA_PROJ(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[1])

#define TG_GEOMETRY_DEPTH_ATTACHMENT                    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT
#define TG_GEOMETRY_ATTACHMENT_COUNT                    (TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1)

#define TG_SHADING_COLOR_ATTACHMENT_COUNT               1
#define TG_SHADING_DEPTH_ATTACHMENT_COUNT               0
#define TG_SHADING_ATTACHMENT_COUNT                     (TG_SHADING_COLOR_ATTACHMENT_COUNT + TG_SHADING_DEPTH_ATTACHMENT_COUNT)
#define TG_SHADING_COLOR_ATTACHMENT_FORMAT              VK_FORMAT_B8G8R8A8_UNORM
#define TG_RENDER_TARGET_FORMAT                         VK_FORMAT_B8G8R8A8_UNORM



typedef struct tg_ssao_info
{
    v4    p_kernel[64];
    v2    noise_scale; u32 pad[2];
    m4    view;
    m4    projection;
} tg_ssao_info;

typedef struct tg_shading_info
{
    v3     camera_position;
    u32    directional_light_count;
    u32    point_light_count; u32 pad[3];
    v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
    m4     p_lightspace_matrices[TG_CASCADED_SHADOW_MAPS];
    v4     p_shadow_distances[(TG_CASCADED_SHADOW_MAPS + 3) / 4]; // because of padding in the shader, this must at least be a vec4 in the shader
} tg_shading_info;



static void tg__init_shadow_pass(tg_renderer_h h_renderer)
{
    h_renderer->shadow_pass.enabled = TG_TRUE;
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        h_renderer->shadow_pass.p_shadow_maps[i] = tgvk_depth_image_create(TG_CASCADED_SHADOW_MAP_SIZE, TG_CASCADED_SHADOW_MAP_SIZE, VK_FORMAT_D32_SFLOAT, TG_NULL);
    }

    tgvk_command_buffer_begin(&global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        tgvk_command_buffer_cmd_transition_depth_image_layout(&global_graphics_command_buffer, &h_renderer->shadow_pass.p_shadow_maps[i], 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    }
    tgvk_command_buffer_end_and_submit(&global_graphics_command_buffer);

    h_renderer->shadow_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        h_renderer->shadow_pass.p_framebuffers[i] = tgvk_framebuffer_create(shared_render_resources.shadow_render_pass, 1, &h_renderer->shadow_pass.p_shadow_maps[i].image_view, TG_CASCADED_SHADOW_MAP_SIZE, TG_CASCADED_SHADOW_MAP_SIZE);
        h_renderer->shadow_pass.p_lightspace_ubos[i] = tgvk_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

static void tg__init_geometry_pass(tg_renderer_h h_renderer)
{
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        h_renderer->geometry_pass.p_color_attachments[i] = tgvk_color_image_create(swapchain_extent.width, swapchain_extent.height, shared_render_resources.p_color_attachment_formats[i], TG_NULL);
    }

    tgvk_command_buffer_begin(&global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tgvk_command_buffer_cmd_transition_color_image_layout(&global_graphics_command_buffer, &h_renderer->geometry_pass.p_color_attachments[i], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    tgvk_command_buffer_end_and_submit(&global_graphics_command_buffer);

    VkImageView p_framebuffer_attachments[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_framebuffer_attachments[i] = h_renderer->geometry_pass.p_color_attachments[i].image_view;
    }
    p_framebuffer_attachments[TG_GEOMETRY_DEPTH_ATTACHMENT] = h_renderer->render_target.depth_attachment.image_view;

    h_renderer->geometry_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.geometry_render_pass, TG_GEOMETRY_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);
    h_renderer->geometry_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

static void tg__init_ssao_pass(tg_renderer_h h_renderer)
{
    h_renderer->ssao_pass.ssao_attachment = tgvk_color_image_create(TG_SSAO_MAP_SIZE, TG_SSAO_MAP_SIZE, VK_FORMAT_R32_SFLOAT, TG_NULL);
    h_renderer->ssao_pass.ssao_framebuffer = tgvk_framebuffer_create(shared_render_resources.ssao_render_pass, 1, &h_renderer->ssao_pass.ssao_attachment.image_view, TG_SSAO_MAP_SIZE, TG_SSAO_MAP_SIZE);

    tgvk_graphics_pipeline_create_info ssao_pipeline_create_info = { 0 };
    ssao_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/screen_quad.vert")->shader;
    ssao_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/ssao.frag")->shader;
    ssao_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    ssao_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    ssao_pipeline_create_info.depth_test_enable = VK_FALSE;
    ssao_pipeline_create_info.depth_write_enable = VK_FALSE;
    ssao_pipeline_create_info.blend_enable = VK_FALSE;
    ssao_pipeline_create_info.render_pass = shared_render_resources.ssao_render_pass;
    ssao_pipeline_create_info.viewport_size.x = (f32)TG_SSAO_MAP_SIZE;
    ssao_pipeline_create_info.viewport_size.y = (f32)TG_SSAO_MAP_SIZE;
    ssao_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->ssao_pass.ssao_graphics_pipeline = tgvk_pipeline_create_graphics(&ssao_pipeline_create_info);
    h_renderer->ssao_pass.ssao_descriptor_set = tgvk_descriptor_set_create(&h_renderer->ssao_pass.ssao_graphics_pipeline);
    h_renderer->ssao_pass.ssao_ubo = tgvk_buffer_create(sizeof(tg_ssao_info), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    tg_random random = tgm_random_init(13031995);
    for (u32 i = 0; i < 64; i++)
    {
        v3 sample = { tgm_random_next_f32(&random) * 2.0f - 1.0f, tgm_random_next_f32(&random) * 2.0f - 1.0f, tgm_random_next_f32(&random) };
        sample = tgm_v3_normalized(sample);
        sample = tgm_v3_mulf(sample, tgm_random_next_f32(&random));
        f32 scale = (f32)i / 64.0f;
        scale = tgm_f32_lerp(0.1f, 1.0f, scale * scale);
        sample = tgm_v3_mulf(sample, scale);
        ((tg_ssao_info*)h_renderer->ssao_pass.ssao_ubo.memory.p_mapped_device_memory)->p_kernel[i].xyz = sample;
    }
    ((tg_ssao_info*)h_renderer->ssao_pass.ssao_ubo.memory.p_mapped_device_memory)->noise_scale.x = (f32)TG_SSAO_MAP_SIZE / 4.0f;
    ((tg_ssao_info*)h_renderer->ssao_pass.ssao_ubo.memory.p_mapped_device_memory)->noise_scale.y = (f32)TG_SSAO_MAP_SIZE / 4.0f;

    tgvk_buffer staging_buffer = tgvk_buffer_create(16 * sizeof(v2), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    for (u32 i = 0; i < 16; i++)
    {
        const v2 noise = { tgm_random_next_f32(&random) * 2.0f - 1.0f, tgm_random_next_f32(&random) * 2.0f - 1.0f };
        ((v2*)staging_buffer.memory.p_mapped_device_memory)[i] = noise;
    }

    tg_sampler_create_info ssao_noise_sampler_create_info = { 0 };
    ssao_noise_sampler_create_info.min_filter = VK_FILTER_NEAREST;
    ssao_noise_sampler_create_info.mag_filter = VK_FILTER_NEAREST;
    ssao_noise_sampler_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ssao_noise_sampler_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ssao_noise_sampler_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    h_renderer->ssao_pass.ssao_noise_image = tgvk_color_image_create(4, 4, VK_FORMAT_R32G32_SFLOAT, &ssao_noise_sampler_create_info);

    tgvk_descriptor_set_update_image(h_renderer->ssao_pass.ssao_descriptor_set.descriptor_set, &h_renderer->geometry_pass.p_color_attachments[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_POSITION], 0);
    tgvk_descriptor_set_update_image(h_renderer->ssao_pass.ssao_descriptor_set.descriptor_set, &h_renderer->geometry_pass.p_color_attachments[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_NORMAL], 1);
    tgvk_descriptor_set_update_image(h_renderer->ssao_pass.ssao_descriptor_set.descriptor_set, &h_renderer->ssao_pass.ssao_noise_image, 2);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->ssao_pass.ssao_descriptor_set.descriptor_set, h_renderer->ssao_pass.ssao_ubo.buffer, 3);


    
    h_renderer->ssao_pass.blur_attachment = tgvk_color_image_create(TG_SSAO_MAP_SIZE, TG_SSAO_MAP_SIZE, VK_FORMAT_R32_SFLOAT, TG_NULL);
    h_renderer->ssao_pass.blur_framebuffer = tgvk_framebuffer_create(shared_render_resources.ssao_blur_render_pass, 1, &h_renderer->ssao_pass.blur_attachment.image_view, TG_SSAO_MAP_SIZE, TG_SSAO_MAP_SIZE);

    tgvk_graphics_pipeline_create_info blur_pipeline_create_info = { 0 };
    blur_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/screen_quad.vert")->shader;
    blur_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/ssao_blur.frag")->shader;
    blur_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    blur_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    blur_pipeline_create_info.depth_test_enable = VK_FALSE;
    blur_pipeline_create_info.depth_write_enable = VK_FALSE;
    blur_pipeline_create_info.blend_enable = VK_FALSE;
    blur_pipeline_create_info.render_pass = shared_render_resources.ssao_blur_render_pass;
    blur_pipeline_create_info.viewport_size.x = (f32)TG_SSAO_MAP_SIZE;
    blur_pipeline_create_info.viewport_size.y = (f32)TG_SSAO_MAP_SIZE;
    blur_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->ssao_pass.blur_graphics_pipeline = tgvk_pipeline_create_graphics(&blur_pipeline_create_info);
    h_renderer->ssao_pass.blur_descriptor_set = tgvk_descriptor_set_create(&h_renderer->ssao_pass.blur_graphics_pipeline);
    tgvk_descriptor_set_update_image(h_renderer->ssao_pass.blur_descriptor_set.descriptor_set, &h_renderer->ssao_pass.ssao_attachment, 0);



    h_renderer->ssao_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgvk_command_buffer_begin(&h_renderer->ssao_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        tgvk_command_buffer_cmd_transition_color_image_layout(&h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.ssao_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        tgvk_command_buffer_cmd_transition_color_image_layout(&h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.ssao_noise_image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        tgvk_command_buffer_cmd_copy_buffer_to_color_image(&h_renderer->ssao_pass.command_buffer, staging_buffer.buffer, &h_renderer->ssao_pass.ssao_noise_image);
        tgvk_command_buffer_cmd_transition_color_image_layout(&h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.ssao_noise_image, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        tgvk_command_buffer_cmd_transition_color_image_layout(&h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.blur_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    tgvk_command_buffer_end_and_submit(&h_renderer->ssao_pass.command_buffer);
    tgvk_buffer_destroy(&staging_buffer);

    tgvk_command_buffer_begin(&h_renderer->ssao_pass.command_buffer, 0);
    {
        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->ssao_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }

        const VkDeviceSize vertex_buffer_offset = 0;

        vkCmdBindPipeline(h_renderer->ssao_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ssao_pass.ssao_graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(h_renderer->ssao_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->ssao_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->ssao_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->ssao_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ssao_pass.ssao_graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->ssao_pass.ssao_descriptor_set.descriptor_set, 0, TG_NULL);
        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->ssao_pass.command_buffer, shared_render_resources.ssao_render_pass, &h_renderer->ssao_pass.ssao_framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->ssao_pass.command_buffer.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->ssao_pass.command_buffer.command_buffer);

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.ssao_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->ssao_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ssao_pass.blur_graphics_pipeline.pipeline);
        vkCmdBindIndexBuffer(h_renderer->ssao_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->ssao_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->ssao_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->ssao_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->ssao_pass.blur_graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->ssao_pass.blur_descriptor_set.descriptor_set, 0, TG_NULL);
        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->ssao_pass.command_buffer, shared_render_resources.ssao_blur_render_pass, &h_renderer->ssao_pass.blur_framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->ssao_pass.command_buffer.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->ssao_pass.command_buffer.command_buffer);

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->ssao_pass.command_buffer, &h_renderer->ssao_pass.ssao_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->ssao_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );
        }
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->ssao_pass.command_buffer.command_buffer));
}

static void tg__init_shading_pass(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    h_renderer->shading_pass.shading_info_ubo = tgvk_buffer_create(sizeof(tg_shading_info), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    tg_shading_info* p_shading_info = h_renderer->shading_pass.shading_info_ubo.memory.p_mapped_device_memory;
    p_shading_info->directional_light_count = 0;
    p_shading_info->point_light_count = 0;

    h_renderer->shading_pass.color_attachment = tgvk_color_image_create(swapchain_extent.width, swapchain_extent.height, TG_SHADING_COLOR_ATTACHMENT_FORMAT, TG_NULL);

    tgvk_command_buffer_begin(&global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_command_buffer_cmd_transition_color_image_layout(&global_graphics_command_buffer, &h_renderer->shading_pass.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgvk_command_buffer_end_and_submit(&global_graphics_command_buffer);

    h_renderer->shading_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.shading_render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_renderer->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/screen_quad.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/shading.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.blend_enable = VK_FALSE;
    graphics_pipeline_create_info.render_pass = shared_render_resources.shading_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->shading_pass.framebuffer.width;
    graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->shading_pass.framebuffer.height;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->shading_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_renderer->shading_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->shading_pass.graphics_pipeline);
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tgvk_descriptor_set_update_image(h_renderer->shading_pass.descriptor_set.descriptor_set, &h_renderer->geometry_pass.p_color_attachments[i], i);
    }

    tgvk_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.descriptor_set.descriptor_set, h_renderer->shading_pass.shading_info_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT);
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        tgvk_descriptor_set_update_image_array(h_renderer->shading_pass.descriptor_set.descriptor_set, &h_renderer->shadow_pass.p_shadow_maps[i], TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1, i);
    }
    tgvk_descriptor_set_update_image(h_renderer->shading_pass.descriptor_set.descriptor_set, &h_renderer->ssao_pass.blur_attachment, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 2);

    h_renderer->shading_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_command_buffer_begin(&h_renderer->shading_pass.command_buffer, 0);
    {
        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }
        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->shading_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }
        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->shading_pass.command_buffer,
            &h_renderer->ssao_pass.blur_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(h_renderer->shading_pass.command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->shading_pass.command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->shading_pass.command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->shading_pass.command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->shading_pass.descriptor_set.descriptor_set, 0, TG_NULL);

        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->shading_pass.command_buffer, shared_render_resources.shading_render_pass, &h_renderer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->shading_pass.command_buffer.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->shading_pass.command_buffer.command_buffer);

        // TODO: skydome!!! then remove the clearing of all color images
        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );
            tgvk_command_buffer_cmd_clear_color_image(&h_renderer->shading_pass.command_buffer, &h_renderer->geometry_pass.p_color_attachments[i]);
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );
        }
        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->shading_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
            );
        }
        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->shading_pass.command_buffer,
            &h_renderer->ssao_pass.blur_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->shading_pass.command_buffer.command_buffer));
}

static void tg__init_forward_pass(tg_renderer_h h_renderer)
{
    const VkImageView p_image_views[2] = {
        h_renderer->shading_pass.color_attachment.image_view,
        h_renderer->render_target.depth_attachment.image_view
    };
    h_renderer->forward_pass.framebuffer = tgvk_framebuffer_create(shared_render_resources.forward_render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);
    h_renderer->forward_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

static void tg__init_tone_mapping_pass(tg_renderer_h h_renderer)
{
    const VkDeviceSize exposure_sum_buffer_size = 2 * sizeof(u32);

    h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer = tgvk_buffer_create(exposure_sum_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    tgvk_buffer staging_buffer = tgvk_buffer_create(exposure_sum_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ((u32*)staging_buffer.memory.p_mapped_device_memory)[0] = 0;
    ((u32*)staging_buffer.memory.p_mapped_device_memory)[1] = 0;
    tgvk_buffer_copy(exposure_sum_buffer_size, &staging_buffer, &h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer);
    tgvk_buffer_destroy(&staging_buffer);

    h_renderer->tone_mapping_pass.acquire_exposure_compute_shader = tg_compute_shader_get("shaders/renderer/acquire_exposure.comp")->shader;
    h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline = tgvk_pipeline_create_compute(&h_renderer->tone_mapping_pass.acquire_exposure_compute_shader);
    h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline);
    h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer = tgvk_buffer_create(exposure_sum_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);



    h_renderer->tone_mapping_pass.finalize_exposure_compute_shader = tg_compute_shader_get("shaders/renderer/finalize_exposure.comp")->shader;
    h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline = tgvk_pipeline_create_compute(&h_renderer->tone_mapping_pass.finalize_exposure_compute_shader);
    h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline);
    h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer = tgvk_buffer_create(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo = tgvk_buffer_create(sizeof(f32), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);



    h_renderer->tone_mapping_pass.adapt_exposure_framebuffer = tgvk_framebuffer_create(shared_render_resources.tone_mapping_render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_renderer->render_target.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tgvk_graphics_pipeline_create_info exposure_graphics_pipeline_create_info = { 0 };
    exposure_graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/screen_quad.vert")->shader;
    exposure_graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/adapt_exposure.frag")->shader;
    exposure_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    exposure_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    exposure_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    exposure_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    exposure_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    exposure_graphics_pipeline_create_info.render_pass = shared_render_resources.tone_mapping_render_pass;
    exposure_graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->tone_mapping_pass.adapt_exposure_framebuffer.width;
    exposure_graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->tone_mapping_pass.adapt_exposure_framebuffer.height;
    exposure_graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline = tgvk_pipeline_create_graphics(&exposure_graphics_pipeline_create_info);
    h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set = tgvk_descriptor_set_create(&h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline);
    h_renderer->tone_mapping_pass.adapt_exposure_command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);



    tgvk_descriptor_set_update_image(h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.descriptor_set, &h_renderer->shading_pass.color_attachment, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.descriptor_set, h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer.buffer, 1);

    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.descriptor_set, h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer.buffer, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.descriptor_set, h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer.buffer, 1);
    tgvk_descriptor_set_update_uniform_buffer(h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.descriptor_set, h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo.buffer, 2);

    tgvk_descriptor_set_update_image(h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.descriptor_set, &h_renderer->shading_pass.color_attachment, 0);
    tgvk_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.descriptor_set, h_renderer->tone_mapping_pass.finalize_exposure_storage_buffer.buffer, 1);

    tgvk_command_buffer_begin(&h_renderer->tone_mapping_pass.adapt_exposure_command_buffer, 0);
    {
        tgvk_command_buffer_cmd_copy_buffer(&h_renderer->tone_mapping_pass.adapt_exposure_command_buffer, exposure_sum_buffer_size, &h_renderer->tone_mapping_pass.acquire_exposure_clear_storage_buffer, &h_renderer->tone_mapping_pass.acquire_exposure_storage_buffer);

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->tone_mapping_pass.adapt_exposure_command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.acquire_exposure_descriptor_set.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, 64, 32, 1);

        VkMemoryBarrier memory_barrier = { 0 };
        memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memory_barrier.pNext = TG_NULL;
        memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &memory_barrier, 0, TG_NULL, 0, TG_NULL
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.finalize_exposure_compute_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.finalize_exposure_descriptor_set.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, 1, 1, 1);

        vkCmdPipelineBarrier(
            h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &memory_barrier, 0, TG_NULL, 0, TG_NULL
        );

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->tone_mapping_pass.adapt_exposure_command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindIndexBuffer(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindVertexBuffers(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindVertexBuffers(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.adapt_exposure_graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.adapt_exposure_descriptor_set.descriptor_set, 0, TG_NULL);

        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->tone_mapping_pass.adapt_exposure_command_buffer, shared_render_resources.tone_mapping_render_pass, &h_renderer->tone_mapping_pass.adapt_exposure_framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer);

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->tone_mapping_pass.adapt_exposure_command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer));
}

static void tg__init_blit_pass(tg_renderer_h h_renderer)
{
    h_renderer->blit_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgvk_command_buffer_begin(&h_renderer->blit_pass.command_buffer, 0);
    {
        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.color_attachment_copy,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.depth_attachment_copy,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

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

        tgvk_command_buffer_cmd_blit_image(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.color_attachment, &h_renderer->render_target.color_attachment_copy, &color_image_blit);

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

        tgvk_command_buffer_cmd_blit_image(&h_renderer->blit_pass.command_buffer, &h_renderer->render_target.depth_attachment, &h_renderer->render_target.depth_attachment_copy, &depth_image_blit);

        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.color_attachment,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        tgvk_command_buffer_cmd_transition_color_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.color_attachment_copy,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->blit_pass.command_buffer,
            &h_renderer->render_target.depth_attachment_copy,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->blit_pass.command_buffer.command_buffer));
}

static void tg__init_clear_pass(tg_renderer_h h_renderer)
{
    h_renderer->clear_pass.command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgvk_command_buffer_begin(&h_renderer->clear_pass.command_buffer, 0);
    {
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->clear_pass.command_buffer,
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_command_buffer_cmd_clear_depth_image(&h_renderer->clear_pass.command_buffer, &h_renderer->render_target.depth_attachment);
        tgvk_command_buffer_cmd_transition_depth_image_layout(
            &h_renderer->clear_pass.command_buffer,
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );

        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->clear_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );
            tgvk_command_buffer_cmd_clear_depth_image(&h_renderer->clear_pass.command_buffer, &h_renderer->shadow_pass.p_shadow_maps[i]);
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->clear_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
            );
        }
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->clear_pass.command_buffer.command_buffer));
}

static void tg__init_present_pass(tg_renderer_h h_renderer)
{
    tgvk_buffer staging_buffer = { 0 };

    h_renderer->present_pass.image_acquired_semaphore = tgvk_semaphore_create();

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        h_renderer->present_pass.p_framebuffers[i] = tgvk_framebuffer_create(shared_render_resources.present_render_pass, 1, &p_swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/screen_quad.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/present.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.blend_enable = VK_FALSE;
    graphics_pipeline_create_info.render_pass = shared_render_resources.present_render_pass;
    graphics_pipeline_create_info.viewport_size.x = (f32)swapchain_extent.width;
    graphics_pipeline_create_info.viewport_size.y = (f32)swapchain_extent.height;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_renderer->present_pass.graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
    h_renderer->present_pass.descriptor_set = tgvk_descriptor_set_create(&h_renderer->present_pass.graphics_pipeline);

    tgvk_command_buffers_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_MAX_SWAPCHAIN_IMAGES, h_renderer->present_pass.p_command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_renderer->render_target.color_attachment.sampler;
    descriptor_image_info.imageView = h_renderer->render_target.color_attachment.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_renderer->present_pass.descriptor_set.descriptor_set;
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
            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->present_pass.p_command_buffers[i],
                &h_renderer->render_target.color_attachment,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
            );
            vkCmdBindPipeline(h_renderer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.pipeline);
            vkCmdBindIndexBuffer(h_renderer->present_pass.p_command_buffers[i].command_buffer, shared_render_resources.screen_quad_indices.buffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindVertexBuffers(h_renderer->present_pass.p_command_buffers[i].command_buffer, 0, 1, &shared_render_resources.screen_quad_positions_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindVertexBuffers(h_renderer->present_pass.p_command_buffers[i].command_buffer, 1, 1, &shared_render_resources.screen_quad_uvs_buffer.buffer, &vertex_buffer_offset);
            vkCmdBindDescriptorSets(h_renderer->present_pass.p_command_buffers[i].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->present_pass.descriptor_set.descriptor_set, 0, TG_NULL);

            tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->present_pass.p_command_buffers[i], shared_render_resources.present_render_pass, &h_renderer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            vkCmdDrawIndexed(h_renderer->present_pass.p_command_buffers[i].command_buffer, 6, 1, 0, 0, 0);
            vkCmdEndRenderPass(h_renderer->present_pass.p_command_buffers[i].command_buffer);

            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->present_pass.p_command_buffers[i],
                &h_renderer->render_target.color_attachment,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->present_pass.p_command_buffers[i],
                &h_renderer->render_target.depth_attachment,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );

            tgvk_command_buffer_cmd_clear_color_image(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment);
            tgvk_command_buffer_cmd_clear_depth_image(&h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.depth_attachment);

            tgvk_command_buffer_cmd_transition_color_image_layout(
                &h_renderer->present_pass.p_command_buffers[i],
                &h_renderer->render_target.color_attachment,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );
            tgvk_command_buffer_cmd_transition_depth_image_layout(
                &h_renderer->present_pass.p_command_buffers[i],
                &h_renderer->render_target.depth_attachment,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
            );
        }
        VK_CALL(vkEndCommandBuffer(h_renderer->present_pass.p_command_buffers[i].command_buffer));
    }
}



void tgvk_renderer_init_shared_resources()
{
    const u16 p_indices[6] = { 0, 1, 2, 2, 3, 0 };

    v2 p_positions[4] = { 0 };
    p_positions[0] = (v2){ -1.0f,  1.0f };
    p_positions[1] = (v2){ 1.0f,  1.0f };
    p_positions[2] = (v2){ 1.0f, -1.0f };
    p_positions[3] = (v2){ -1.0f, -1.0f };

    v2 p_uvs[4] = { 0 };
    p_uvs[0] = (v2){ 0.0f,  1.0f };
    p_uvs[1] = (v2){ 1.0f,  1.0f };
    p_uvs[2] = (v2){ 1.0f,  0.0f };
    p_uvs[3] = (v2){ 0.0f,  0.0f };

    tgvk_buffer staging_buffer = tgvk_buffer_create(tgm_u64_max(sizeof(p_indices), tgm_u64_max(sizeof(p_positions), sizeof(p_uvs))), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    tgvk_buffer_flush_mapped_memory(&staging_buffer);
    shared_render_resources.screen_quad_indices = tgvk_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgvk_buffer_copy(sizeof(p_indices), &staging_buffer, &shared_render_resources.screen_quad_indices);

    tg_memory_copy(sizeof(p_positions), p_positions, staging_buffer.memory.p_mapped_device_memory);
    tgvk_buffer_flush_mapped_memory(&staging_buffer);
    shared_render_resources.screen_quad_positions_buffer = tgvk_buffer_create(sizeof(p_positions), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgvk_buffer_copy(sizeof(p_positions), &staging_buffer, &shared_render_resources.screen_quad_positions_buffer);

    tg_memory_copy(sizeof(p_uvs), p_uvs, staging_buffer.memory.p_mapped_device_memory);
    tgvk_buffer_flush_mapped_memory(&staging_buffer);
    shared_render_resources.screen_quad_uvs_buffer = tgvk_buffer_create(sizeof(p_uvs), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgvk_buffer_copy(sizeof(p_uvs), &staging_buffer, &shared_render_resources.screen_quad_uvs_buffer);

    tgvk_buffer_destroy(&staging_buffer);

    // shadow pass
    {
        VkAttachmentDescription attachment_description = { 0 };
        attachment_description.flags = 0;
        attachment_description.format = VK_FORMAT_D32_SFLOAT;
        attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference attachment_reference = { 0 };
        attachment_reference.attachment = 0;
        attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = { 0 };
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = TG_NULL;
        subpass_description.colorAttachmentCount = 0;
        subpass_description.pColorAttachments = TG_NULL;
        subpass_description.pResolveAttachments = TG_NULL;
        subpass_description.pDepthStencilAttachment = &attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = TG_NULL;

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        shared_render_resources.shadow_render_pass = tgvk_render_pass_create(1, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    }

    // geometry pass
    {
        TG_ASSERT(TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT == 4);
        shared_render_resources.p_color_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
        shared_render_resources.p_color_attachment_formats[1] = VK_FORMAT_R16G16B16A16_SFLOAT;
        shared_render_resources.p_color_attachment_formats[2] = VK_FORMAT_R16G16B16A16_SFLOAT;
        shared_render_resources.p_color_attachment_formats[3] = VK_FORMAT_R16G16B16A16_SFLOAT;

        VkAttachmentDescription p_attachment_descriptions[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };

        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            p_attachment_descriptions[i].flags = 0;
            p_attachment_descriptions[i].format = shared_render_resources.p_color_attachment_formats[i];
            p_attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
            p_attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            p_attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            p_attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            p_attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            p_attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            p_attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].flags = 0;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].format = VK_FORMAT_D32_SFLOAT;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference p_color_attachment_references[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT] = { 0 };

        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            p_color_attachment_references[i].attachment = i;
            p_color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference depth_buffer_attachment_reference = { 0 };
        depth_buffer_attachment_reference.attachment = TG_GEOMETRY_DEPTH_ATTACHMENT;
        depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = { 0 };
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = TG_NULL;
        subpass_description.colorAttachmentCount = TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT;
        subpass_description.pColorAttachments = p_color_attachment_references;
        subpass_description.pResolveAttachments = TG_NULL;
        subpass_description.pDepthStencilAttachment = &depth_buffer_attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = TG_NULL;

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        shared_render_resources.geometry_render_pass = tgvk_render_pass_create(TG_GEOMETRY_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);
    }

    // ssao pass
    {
        VkAttachmentDescription attachment_description = { 0 };
        attachment_description.flags = 0;
        attachment_description.format = VK_FORMAT_R32_SFLOAT;
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

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        shared_render_resources.ssao_render_pass = tgvk_render_pass_create(1, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
        shared_render_resources.ssao_blur_render_pass = tgvk_render_pass_create(1, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    }

    // shading pass
    {
        VkAttachmentDescription attachment_description = { 0 };
        attachment_description.flags = 0;
        attachment_description.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
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

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        shared_render_resources.shading_render_pass = tgvk_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    }

    // tone mapping pass
    {
        VkAttachmentDescription attachment_description = { 0 };
        attachment_description.flags = 0;
        attachment_description.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
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

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        shared_render_resources.tone_mapping_render_pass = tgvk_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    }

    // forward pass
    {
        VkAttachmentDescription p_attachment_descriptions[2] = { 0 };

        p_attachment_descriptions[0].flags = 0;
        p_attachment_descriptions[0].format = TG_RENDER_TARGET_FORMAT;
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

        VkSubpassDependency subpass_dependency = { 0 };
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        shared_render_resources.forward_render_pass = tgvk_render_pass_create(2, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);
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

        shared_render_resources.present_render_pass = tgvk_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);
    }
}

tg_renderer_h tg_renderer_create(tg_camera* p_camera)
{
    TG_ASSERT(p_camera);

    tg_renderer_h h_renderer = tgvk_handle_take(TG_STRUCTURE_TYPE_RENDERER);
    h_renderer->p_camera = p_camera;
    h_renderer->view_projection_ubo = tgvk_buffer_create(2 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // TODO: not the latter one!

    h_renderer->render_target = tgvk_render_target_create(
        swapchain_extent.width, swapchain_extent.height, TG_RENDER_TARGET_FORMAT, TG_NULL,
        swapchain_extent.width, swapchain_extent.height, VK_FORMAT_D32_SFLOAT, TG_NULL,
        VK_FENCE_CREATE_SIGNALED_BIT
    );
    h_renderer->render_command_count = 0;

    h_renderer->semaphore = tgvk_semaphore_create();

    tg__init_shadow_pass(h_renderer);
    tg__init_geometry_pass(h_renderer);
    tg__init_ssao_pass(h_renderer);
    tg__init_shading_pass(h_renderer);
    tg__init_forward_pass(h_renderer);
    tg__init_tone_mapping_pass(h_renderer);
    tg__init_blit_pass(h_renderer);
    tg__init_clear_pass(h_renderer);
    tg__init_present_pass(h_renderer);


#if TG_ENABLE_DEBUG_TOOLS == 1
    h_renderer->DEBUG.p_cubes = TG_MEMORY_ALLOC(TG_DEBUG_MAX_CUBES * sizeof(*h_renderer->DEBUG.p_cubes));

    tgvk_buffer staging_buffer = tgvk_buffer_create(24 * sizeof(v3), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    v3* p_pit = (v3*)staging_buffer.memory.p_mapped_device_memory;

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

    tgvk_buffer_flush_mapped_memory(&staging_buffer);
    h_renderer->DEBUG.cube_position_buffer = tgvk_buffer_create(24 * sizeof(v3), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgvk_buffer_copy(24 * sizeof(v3), &staging_buffer, &h_renderer->DEBUG.cube_position_buffer);

    u16* p_iit = (u16*)staging_buffer.memory.p_mapped_device_memory;
    for (u8 i = 0; i < 6; i++)
    {
        *p_iit++ = 4 * i + 0;
        *p_iit++ = 4 * i + 1;
        *p_iit++ = 4 * i + 2;
        *p_iit++ = 4 * i + 2;
        *p_iit++ = 4 * i + 3;
        *p_iit++ = 4 * i + 0;
    }

    tgvk_buffer_flush_mapped_memory(&staging_buffer);
    h_renderer->DEBUG.cube_index_buffer = tgvk_buffer_create(36 * sizeof(u16), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgvk_buffer_copy(36 * sizeof(u16), &staging_buffer, &h_renderer->DEBUG.cube_index_buffer);
    tgvk_buffer_destroy(&staging_buffer);
#endif

    return h_renderer;
}

void tg_renderer_destroy(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    // TODO: this
    // TG_INVALID_CODEPATH();
}

void tg_renderer_enable_shadows(tg_renderer_h h_renderer, b32 enable)
{
    TG_ASSERT(h_renderer);

    h_renderer->shadow_pass.enabled = enable;
}

void tg_renderer_begin(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    h_renderer->render_command_count = 0;
#if TG_ENABLE_DEBUG_TOOLS == 1
    h_renderer->DEBUG.cube_count = 0;
#endif
}

void tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(direction) && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_shading_info* p_light_setup = (tg_shading_info*)h_renderer->shading_pass.shading_info_ubo.memory.p_mapped_device_memory;

    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].x = direction.x;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].y = direction.y;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].z = direction.z;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].w = 0.0f;

    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].x = color.x;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].y = color.y;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].z = color.z;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].w = 1.0f;

    p_light_setup->directional_light_count++;
}

void tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_shading_info* p_light_setup = (tg_shading_info*)h_renderer->shading_pass.shading_info_ubo.memory.p_mapped_device_memory;

    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].x = position.x;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].y = position.y;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].z = position.z;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].w = 0.0f;

    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].x = color.x;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].y = color.y;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].z = color.z;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].w = 1.0f;

    p_light_setup->point_light_count++;
}

void tg_renderer_exec(tg_renderer_h h_renderer, tg_render_command_h h_render_command)
{
    TG_ASSERT(h_renderer && h_render_command);

    tgvk_render_command_renderer_info* p_renderer_info = TG_NULL;
    for (u32 i = 0; i < h_render_command->renderer_info_count; i++)
    {
        if (h_render_command->p_renderer_infos[i].h_renderer == h_renderer)
        {
            p_renderer_info = &h_render_command->p_renderer_infos[i];
            break;
        }
    }

    // TODO: this crashes, when a render command is created before the renderer. that
    // way, the renderer info is not set up for that specific renderer. think of
    // threading in this case!
    TG_ASSERT(p_renderer_info);
    TG_ASSERT(h_renderer->render_command_count < TG_MAX_RENDER_COMMANDS);

    h_renderer->ph_render_commands[h_renderer->render_command_count++] = h_render_command;
}

void tg_renderer_end(tg_renderer_h h_renderer, f32 dt, b32 present)
{
    TG_ASSERT(h_renderer);
    
    const tg_camera* p_cam = h_renderer->p_camera;
    TG_CAMERA_VIEW(h_renderer->view_projection_ubo) = tgm_m4_mul(tgm_m4_inverse(tgm_m4_euler(p_cam->pitch, p_cam->yaw, p_cam->roll)), tgm_m4_translate(tgm_v3_neg(p_cam->position)));
    if (h_renderer->p_camera->type == TG_CAMERA_TYPE_ORTHOGRAPHIC)
    {
        TG_CAMERA_PROJ(h_renderer->view_projection_ubo) = tgm_m4_orthographic(p_cam->ortho.l, p_cam->ortho.r, p_cam->ortho.b, p_cam->ortho.t, p_cam->ortho.f, p_cam->ortho.n);
    }
    else
    {
        TG_CAMERA_PROJ(h_renderer->view_projection_ubo) = tgm_m4_perspective(p_cam->persp.fov_y_in_radians, p_cam->persp.aspect, p_cam->persp.n, p_cam->persp.f);
    }

    ((tg_ssao_info*)h_renderer->ssao_pass.ssao_ubo.memory.p_mapped_device_memory)->view = TG_CAMERA_VIEW(h_renderer->view_projection_ubo);
    ((tg_ssao_info*)h_renderer->ssao_pass.ssao_ubo.memory.p_mapped_device_memory)->projection = TG_CAMERA_PROJ(h_renderer->view_projection_ubo);

    tg_shading_info* p_shading_info = (tg_shading_info*)h_renderer->shading_pass.shading_info_ubo.memory.p_mapped_device_memory;
    p_shading_info->camera_position = h_renderer->p_camera->position;

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = TG_NULL;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    command_buffer_begin_info.pInheritanceInfo = TG_NULL;

    if (h_renderer->shadow_pass.enabled)
    {
        vkBeginCommandBuffer(h_renderer->shadow_pass.command_buffer.command_buffer, &command_buffer_begin_info);
        {
            const f32 p_percentiles[TG_CASCADED_SHADOW_MAPS + 1] = { 0.0f, 0.003f, 0.01f, 0.06f };

            for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
            {
                tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->shadow_pass.command_buffer, shared_render_resources.shadow_render_pass, &h_renderer->shadow_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

                // TODO: decide, which lights cast/receive shadows!
                const v4 p_corners[8] = {
                    { -1.0f,  1.0f,  0.0f,  1.0f },
                    {  1.0f,  1.0f,  0.0f,  1.0f },
                    { -1.0f, -1.0f,  0.0f,  1.0f },
                    {  1.0f, -1.0f,  0.0f,  1.0f },
                    { -1.0f,  1.0f,  1.0f,  1.0f },
                    {  1.0f,  1.0f,  1.0f,  1.0f },
                    { -1.0f, -1.0f,  1.0f,  1.0f },
                    {  1.0f, -1.0f,  1.0f,  1.0f }
                };

                const m4 ivp = tgm_m4_inverse(tgm_m4_mul(TG_CAMERA_PROJ(h_renderer->view_projection_ubo), TG_CAMERA_VIEW(h_renderer->view_projection_ubo)));
                v3 p_corners_ws[8] = { 0 };
                for (u8 i = 0; i < 8; i++)
                {
                    p_corners_ws[i] = tgm_v4_to_v3(tgm_m4_mulv4(ivp, p_corners[i]));
                }

                const v3 v04 = tgm_v3_sub(p_corners_ws[4], p_corners_ws[0]);
                const f32 d = tgm_v3_mag(v04);
                const f32 d0 = d * p_percentiles[i];
                const f32 d1 = d * p_percentiles[i + 1];

                const u32 major0 = i / 4;
                const u32 minor0 = i % 4;
                const u32 major1 = (i + 1) / 4;
                const u32 minor1 = (i + 1) % 4;
                p_shading_info->p_shadow_distances[major0].p_data[minor0] = d0;
                p_shading_info->p_shadow_distances[major1].p_data[minor1] = d1;

                for (u8 i = 0; i < 4; i++)
                {
                    const v3 ndir = tgm_v3_normalized(tgm_v3_sub(p_corners_ws[i + 4], p_corners_ws[i]));
                    p_corners_ws[i + 4] = tgm_v3_add(p_corners_ws[i], tgm_v3_mulf(ndir, d1));
                    p_corners_ws[i] = tgm_v3_add(p_corners_ws[i], tgm_v3_mulf(ndir, d0));
                }

                v3 center; f32 radius;
                tgm_enclosing_sphere(8, p_corners_ws, &center, &radius);

                const m4 t = tgm_m4_translate(tgm_v3_neg(center));
                const m4 r = tgm_m4_inverse(tgm_m4_euler(-TG_PI * 0.4f, -TG_PI * 0.4f, 0.0f));
                const m4 v = tgm_m4_mul(r, t);
                const m4 p = tgm_m4_orthographic(-radius, radius, -radius, radius, -radius, radius);
                const m4 vp = tgm_m4_mul(p, v);

                *((m4*)h_renderer->shadow_pass.p_lightspace_ubos[i].memory.p_mapped_device_memory) = vp;

                for (u32 j = 0; j < h_renderer->render_command_count; j++)
                {
                    tg_render_command_h h_render_command = h_renderer->ph_render_commands[j];
                    if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
                    {
                        for (u32 k = 0; k < h_render_command->renderer_info_count; k++)
                        {
                            tgvk_render_command_renderer_info* p_renderer_info = &h_render_command->p_renderer_infos[k];
                            if (p_renderer_info->h_renderer == h_renderer)
                            {
                                vkCmdExecuteCommands(h_renderer->shadow_pass.command_buffer.command_buffer, 1, &p_renderer_info->p_shadow_command_buffers[i].command_buffer);
                                break;
                            }
                        }
                    }
                }

                vkCmdEndRenderPass(h_renderer->shadow_pass.command_buffer.command_buffer);
            }

            for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
            {
                p_shading_info->p_lightspace_matrices[i] = *(m4*)h_renderer->shadow_pass.p_lightspace_ubos[i].memory.p_mapped_device_memory;
            }

            tgvk_buffer_flush_mapped_memory(&h_renderer->shading_pass.shading_info_ubo);
        }
        VK_CALL(vkEndCommandBuffer(h_renderer->shadow_pass.command_buffer.command_buffer));

        VkSubmitInfo submit_info = { 0 };
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &h_renderer->shadow_pass.command_buffer.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;

        tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);
    }
    else
    {
        tgvk_buffer_flush_mapped_memory(&h_renderer->shading_pass.shading_info_ubo);
    }



    VK_CALL(vkBeginCommandBuffer(h_renderer->geometry_pass.command_buffer.command_buffer, &command_buffer_begin_info));
    {
        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->geometry_pass.command_buffer, shared_render_resources.geometry_render_pass, &h_renderer->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        for (u32 i = 0; i < h_renderer->render_command_count; i++)
        {
            tg_render_command_h h_render_command = h_renderer->ph_render_commands[i];
            if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
            {
                for (u32 j = 0; j < h_render_command->renderer_info_count; j++)
                {
                    tgvk_render_command_renderer_info* p_renderer_info = &h_render_command->p_renderer_infos[j];
                    if (p_renderer_info->h_renderer == h_renderer)
                    {
                        vkCmdExecuteCommands(h_renderer->geometry_pass.command_buffer.command_buffer, 1, &p_renderer_info->command_buffer.command_buffer);
                        // TODO: lod
                        //{
                        //    const tg_bounds* p_bounds = &h_render_command->ph_lod_meshes[0]->bounds;
                        //
                        //    v4 p_points[8] = { 0 };
                        //    p_points[0] = (v4) { p_bounds->min.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                        //    p_points[1] = (v4) { p_bounds->min.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                        //    p_points[2] = (v4) { p_bounds->min.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                        //    p_points[3] = (v4) { p_bounds->min.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                        //    p_points[4] = (v4) { p_bounds->max.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                        //    p_points[5] = (v4) { p_bounds->max.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                        //    p_points[6] = (v4) { p_bounds->max.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                        //    p_points[7] = (v4) { p_bounds->max.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                        //
                        //    v4 p_homogenous_points_clip_space[8] = { 0 };
                        //    v3 p_cartesian_points_clip_space[8] = { 0 };
                        //    for (u8 i = 0; i < 8; i++)
                        //    {
                        //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(h_render_command), p_points[i]);
                        //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_VIEW(h_primary_camera), p_homogenous_points_clip_space[i]);
                        //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_PROJ(h_primary_camera), p_homogenous_points_clip_space[i]);// TODO: SIMD
                        //        p_cartesian_points_clip_space[i] = tgm_v4_to_v3(p_homogenous_points_clip_space[i]);
                        //    }
                        //
                        //    v3 min = p_cartesian_points_clip_space[0];
                        //    v3 max = p_cartesian_points_clip_space[0];
                        //    for (u8 i = 1; i < 8; i++)
                        //    {
                        //        min = tgm_v3_min(min, p_cartesian_points_clip_space[i]);
                        //        max = tgm_v3_max(max, p_cartesian_points_clip_space[i]);
                        //    }
                        //
                        //    const b32 cull = min.x >= 1.0f || min.y >= 1.0f || max.x <= -1.0f || max.y <= -1.0f || min.z >= 1.0f || max.z <= 0.0f;
                        //    if (!cull)
                        //    {
                        //        const v3 clamp_min = { -1.0f, -1.0f, 0.0f };
                        //        const v3 clamp_max = { 1.0f,  1.0f, 0.0f };
                        //        const v3 clamped_min = tgm_v3_clamp(min, clamp_min, clamp_max);
                        //        const v3 clamped_max = tgm_v3_clamp(max, clamp_min, clamp_max);
                        //        const f32 area = (clamped_max.x - clamped_min.x) * (clamped_max.y - clamped_min.y);
                        //
                        //        if (area >= 0.9f)
                        //        {
                        //            tgvk_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[2]);
                        //        }
                        //        else
                        //        {
                        //            tgvk_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[3]);
                        //        }
                        //    }
                        //}
                        break;
                    }
                }
            }
        }

        vkCmdEndRenderPass(h_renderer->geometry_pass.command_buffer.command_buffer);
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->geometry_pass.command_buffer.command_buffer));

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

    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);

    const VkPipelineStageFlags p_pipeline_stage_flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo ssao_submit_info = { 0 };
    ssao_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    ssao_submit_info.pNext = TG_NULL;
    ssao_submit_info.waitSemaphoreCount = 1;
    ssao_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    ssao_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    ssao_submit_info.commandBufferCount = 1;
    ssao_submit_info.pCommandBuffers = &h_renderer->ssao_pass.command_buffer.command_buffer;
    ssao_submit_info.signalSemaphoreCount = 1;
    ssao_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &ssao_submit_info, VK_NULL_HANDLE);

    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    shading_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &h_renderer->shading_pass.command_buffer.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);

    vkBeginCommandBuffer(h_renderer->forward_pass.command_buffer.command_buffer, &command_buffer_begin_info);
    {
        tgvk_command_buffer_cmd_begin_render_pass(&h_renderer->forward_pass.command_buffer, shared_render_resources.forward_render_pass, &h_renderer->forward_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

        for (u32 i = 0; i < h_renderer->render_command_count; i++)
        {
            tg_render_command_h h_render_command = h_renderer->ph_render_commands[i];
            if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD)
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
            tgvk_descriptor_set_update_uniform_buffer(h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, h_renderer->DEBUG.p_cubes[i].ubo.buffer, 0);
            tgvk_descriptor_set_update_uniform_buffer(h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, h_renderer->view_projection_ubo.buffer, 1);

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
            vkBeginCommandBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, &command_buffer_begin_info);
            {
                command_buffer_begin_info.pInheritanceInfo = TG_NULL;

                vkCmdBindPipeline(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->DEBUG.p_cubes[i].graphics_pipeline.pipeline);
                const VkDeviceSize DEBUG_vertex_buffer_offset = 0;
                vkCmdBindIndexBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, h_renderer->DEBUG.cube_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindVertexBuffers(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, 0, 1, &h_renderer->DEBUG.cube_position_buffer.buffer, &DEBUG_vertex_buffer_offset);
                vkCmdBindDescriptorSets(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->DEBUG.p_cubes[i].graphics_pipeline.layout.pipeline_layout, 0, 1, &h_renderer->DEBUG.p_cubes[i].descriptor_set.descriptor_set, 0, TG_NULL);
                vkCmdDrawIndexed(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer, 36, 1, 0, 0, 0);
            }
            VK_CALL(vkEndCommandBuffer(h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer));
            vkCmdExecuteCommands(h_renderer->forward_pass.command_buffer.command_buffer, 1, &h_renderer->DEBUG.p_cubes[i].command_buffer.command_buffer);
        }
#endif

        vkCmdEndRenderPass(h_renderer->forward_pass.command_buffer.command_buffer);
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->forward_pass.command_buffer.command_buffer));

    VkSubmitInfo forward_submit_info = { 0 };
    forward_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    forward_submit_info.pNext = TG_NULL;
    forward_submit_info.waitSemaphoreCount = 1;
    forward_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    forward_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    forward_submit_info.commandBufferCount = 1;
    forward_submit_info.pCommandBuffers = &h_renderer->forward_pass.command_buffer.command_buffer;
    forward_submit_info.signalSemaphoreCount = 1;
    forward_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &forward_submit_info, VK_NULL_HANDLE);

    VkSubmitInfo tone_mapping_submit_info = { 0 };
    tone_mapping_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    tone_mapping_submit_info.pNext = TG_NULL;
    tone_mapping_submit_info.waitSemaphoreCount = 1;
    tone_mapping_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    tone_mapping_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    tone_mapping_submit_info.commandBufferCount = 1;
    tone_mapping_submit_info.pCommandBuffers = &h_renderer->tone_mapping_pass.adapt_exposure_command_buffer.command_buffer;
    tone_mapping_submit_info.signalSemaphoreCount = 1;
    tone_mapping_submit_info.pSignalSemaphores = &h_renderer->semaphore;

    *(f32*)h_renderer->tone_mapping_pass.finalize_exposure_dt_ubo.memory.p_mapped_device_memory = dt;
    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &tone_mapping_submit_info, VK_NULL_HANDLE);

    VkSubmitInfo blit_submit_info = { 0 };
    blit_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    blit_submit_info.pNext = TG_NULL;
    blit_submit_info.waitSemaphoreCount = 1;
    blit_submit_info.pWaitSemaphores = &h_renderer->semaphore;
    blit_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    blit_submit_info.commandBufferCount = 1;
    blit_submit_info.pCommandBuffers = &h_renderer->blit_pass.command_buffer.command_buffer;

    if (!present)
    {
        blit_submit_info.signalSemaphoreCount = 0;
        blit_submit_info.pSignalSemaphores = TG_NULL;

        tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &blit_submit_info, h_renderer->render_target.fence);
    }
    else
    {
        blit_submit_info.signalSemaphoreCount = 1;
        blit_submit_info.pSignalSemaphores = &h_renderer->semaphore;

        tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &blit_submit_info, VK_NULL_HANDLE);

        u32 current_image;
        VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, h_renderer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

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

        tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &draw_submit_info, h_renderer->render_target.fence);

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

    tgvk_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_renderer->render_target.fence);

    // TODO: forward renderer
    tg_shading_info* p_light_setup = (tg_shading_info*)h_renderer->shading_pass.shading_info_ubo.memory.p_mapped_device_memory;

    p_light_setup->directional_light_count = 0;
    p_light_setup->point_light_count = 0;
}

tg_render_target_h tg_renderer_get_render_target(tg_renderer_h h_renderer)
{
    return &h_renderer->render_target;
}

#if TG_ENABLE_DEBUG_TOOLS == 1
void tg_renderer_draw_cube_DEBUG(tg_renderer_h h_renderer, v3 position, v3 scale, v4 color)
{
    TG_ASSERT(h_renderer && h_renderer->DEBUG.cube_count < TG_DEBUG_MAX_CUBES);

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.buffer == VK_NULL_HANDLE)
    {
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo = tgvk_buffer_create(sizeof(m4) + sizeof(v4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    *((m4*)h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.memory.p_mapped_device_memory) = tgm_m4_mul(tgm_m4_translate(position), tgm_m4_scale(scale));
    *(v4*)(((m4*)h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo.memory.p_mapped_device_memory) + 1) = color;
    tgvk_buffer_flush_mapped_memory(&h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].ubo);

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline.pipeline == VK_NULL_HANDLE)
    {
        tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
        graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/renderer/debug/forward.vert")->shader;
        graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/renderer/debug/forward.frag")->shader;
        graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
        graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        graphics_pipeline_create_info.depth_test_enable = TG_TRUE;
        graphics_pipeline_create_info.depth_write_enable = TG_TRUE;
        graphics_pipeline_create_info.blend_enable = VK_TRUE;
        graphics_pipeline_create_info.render_pass = shared_render_resources.forward_render_pass;
        graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->forward_pass.framebuffer.width;
        graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->forward_pass.framebuffer.height;
        graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_LINE;

        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].descriptor_set = tgvk_descriptor_set_create(&h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].graphics_pipeline);
    }

    if (h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].command_buffer.command_buffer == VK_NULL_HANDLE)
    {
        h_renderer->DEBUG.p_cubes[h_renderer->DEBUG.cube_count].command_buffer = tgvk_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    }

    h_renderer->DEBUG.cube_count++;
}
#endif

#endif
