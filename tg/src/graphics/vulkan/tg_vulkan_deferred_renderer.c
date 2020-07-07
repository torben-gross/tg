#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_assets.h"



#define TG_GEOMETRY_DEPTH_ATTACHMENT                    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT
#define TG_GEOMETRY_ATTACHMENT_COUNT                    (TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1)

#define TG_SHADING_COLOR_ATTACHMENT_COUNT               1
#define TG_SHADING_DEPTH_ATTACHMENT_COUNT               0
#define TG_SHADING_ATTACHMENT_COUNT                     (TG_SHADING_COLOR_ATTACHMENT_COUNT + TG_SHADING_DEPTH_ATTACHMENT_COUNT)
#define TG_SHADING_COLOR_ATTACHMENT_FORMAT              VK_FORMAT_B8G8R8A8_UNORM



typedef struct tg_light_setup
{
    u32    directional_light_count;
    u32    point_light_count;
    u32    padding[2];

    v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];

    v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_light_setup;



static void tg__init_geometry_pass(tg_deferred_renderer_h h_deferred_renderer)
{
    const VkFormat p_color_attachment_formats[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT] = {
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT
    };

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
        vulkan_color_image_create_info.width = swapchain_extent.width;
        vulkan_color_image_create_info.height = swapchain_extent.height;
        vulkan_color_image_create_info.mip_levels = 1;
        vulkan_color_image_create_info.format = p_color_attachment_formats[i];
        vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;
        h_deferred_renderer->geometry_pass.p_color_attachments[i] = tg_vulkan_color_image_create(&vulkan_color_image_create_info);
    }

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_deferred_renderer->geometry_pass.p_color_attachments[i], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    VkAttachmentDescription p_attachment_descriptions[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_attachment_descriptions[i].flags = 0;
        p_attachment_descriptions[i].format = p_color_attachment_formats[i];
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

    h_deferred_renderer->geometry_pass.render_pass = tg_vulkan_render_pass_create(TG_GEOMETRY_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);

    VkImageView p_framebuffer_attachments[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_framebuffer_attachments[i] = h_deferred_renderer->geometry_pass.p_color_attachments[i].image_view;
    }
    p_framebuffer_attachments[TG_GEOMETRY_DEPTH_ATTACHMENT] = h_deferred_renderer->h_camera->render_target.depth_attachment.image_view;

    h_deferred_renderer->geometry_pass.framebuffer = tg_vulkan_framebuffer_create(h_deferred_renderer->geometry_pass.render_pass, TG_GEOMETRY_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);
    h_deferred_renderer->geometry_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

static void tg__init_shading_pass(tg_deferred_renderer_h h_deferred_renderer, tg_depth_image* p_depth_image)
{
    TG_ASSERT(h_deferred_renderer);

    h_deferred_renderer->shading_pass.camera_ubo = tg_vulkan_buffer_create(sizeof(v3), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    *((v3*)h_deferred_renderer->shading_pass.camera_ubo.memory.p_mapped_device_memory) = h_deferred_renderer->h_camera->position;

    h_deferred_renderer->shading_pass.lighting_ubo = tg_vulkan_buffer_create(sizeof(tg_light_setup), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    tg_light_setup* p_light_setup_uniform_buffer = h_deferred_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;
    p_light_setup_uniform_buffer->directional_light_count = 0;
    p_light_setup_uniform_buffer->point_light_count = 0;

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    h_deferred_renderer->shading_pass.color_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_deferred_renderer->shading_pass.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    tg_vulkan_buffer staging_buffer = { 0 };

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };

    // TODO: y is inverted, because image has to be flipped. add projection matrix to present vertex shader?
    p_vertices[0].position.x = -1.0f;
    p_vertices[0].position.y = -1.0f;
    p_vertices[0].uv.x = 0.0f;
    p_vertices[0].uv.y = 0.0f;

    p_vertices[1].position.x = 1.0f;
    p_vertices[1].position.y = -1.0f;
    p_vertices[1].uv.x = 1.0f;
    p_vertices[1].uv.y = 0.0f;

    p_vertices[2].position.x = 1.0f;
    p_vertices[2].position.y = 1.0f;
    p_vertices[2].uv.x = 1.0f;
    p_vertices[2].uv.y = 1.0f;

    p_vertices[3].position.x = -1.0f;
    p_vertices[3].position.y = 1.0f;
    p_vertices[3].uv.x = 0.0f;
    p_vertices[3].uv.y = 1.0f;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_vertices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
    h_deferred_renderer->shading_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, h_deferred_renderer->shading_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    const u16 p_indices[6] = { 0, 1, 2, 2, 3, 0 };

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    h_deferred_renderer->shading_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, h_deferred_renderer->shading_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    h_deferred_renderer->shading_pass.rendering_finished_semaphore = tg_vulkan_semaphore_create();

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

    h_deferred_renderer->shading_pass.render_pass = tg_vulkan_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    h_deferred_renderer->shading_pass.framebuffer = tg_vulkan_framebuffer_create(h_deferred_renderer->shading_pass.render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_deferred_renderer->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/shading.vert"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/shading.frag"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.render_pass = h_deferred_renderer->shading_pass.render_pass;
    vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_deferred_renderer->shading_pass.framebuffer.width;
    vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_deferred_renderer->shading_pass.framebuffer.height;

    h_deferred_renderer->shading_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_descriptor_set_update_color_image(h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, &h_deferred_renderer->geometry_pass.p_color_attachments[i], i);
    }

    tg_vulkan_descriptor_set_update_uniform_buffer(h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, h_deferred_renderer->shading_pass.camera_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT);
    tg_vulkan_descriptor_set_update_uniform_buffer(h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, h_deferred_renderer->shading_pass.lighting_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1);
    tg_vulkan_descriptor_set_update_uniform_buffer(h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, h_deferred_renderer->h_camera->shadow_pass.lightspace_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 2);
    tg_vulkan_descriptor_set_update_depth_image(h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, p_depth_image, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 3);

    h_deferred_renderer->shading_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tg_vulkan_command_buffer_begin(h_deferred_renderer->shading_pass.command_buffer, 0, TG_NULL);
    {
        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_deferred_renderer->shading_pass.command_buffer,
                &h_deferred_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_deferred_renderer->shading_pass.command_buffer,
            p_depth_image,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_deferred_renderer->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_deferred_renderer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(h_deferred_renderer->shading_pass.command_buffer, 0, 1, &h_deferred_renderer->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_deferred_renderer->shading_pass.command_buffer, h_deferred_renderer->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_deferred_renderer->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_deferred_renderer->shading_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_deferred_renderer->shading_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_deferred_renderer->shading_pass.command_buffer, h_deferred_renderer->shading_pass.render_pass, &h_deferred_renderer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_deferred_renderer->shading_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_deferred_renderer->shading_pass.command_buffer);

        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_deferred_renderer->shading_pass.command_buffer,
                &h_deferred_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );
            tg_vulkan_command_buffer_cmd_clear_color_image(h_deferred_renderer->shading_pass.command_buffer, &h_deferred_renderer->geometry_pass.p_color_attachments[i]);
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_deferred_renderer->shading_pass.command_buffer,
                &h_deferred_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );
        }
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_deferred_renderer->shading_pass.command_buffer,
            p_depth_image,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );
    }
    VK_CALL(vkEndCommandBuffer(h_deferred_renderer->shading_pass.command_buffer));
}

static void tg__init_tone_mapping_pass(tg_deferred_renderer_h h_deferred_renderer)
{
    h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_shader = ((tg_compute_shader_h)tg_assets_get_asset("shaders/acquire_exposure.comp"))->vulkan_shader;
    h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline = tg_vulkan_pipeline_create_compute(&h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_shader);
    h_deferred_renderer->tone_mapping_pass.exposure_storage_buffer = tg_vulkan_buffer_create(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

    h_deferred_renderer->tone_mapping_pass.render_pass = tg_vulkan_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    h_deferred_renderer->tone_mapping_pass.framebuffer = tg_vulkan_framebuffer_create(h_deferred_renderer->tone_mapping_pass.render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_deferred_renderer->h_camera->render_target.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tg_vulkan_graphics_pipeline_create_info exposure_vulkan_graphics_pipeline_create_info = { 0 };
    exposure_vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/adapt_exposure.vert"))->vulkan_shader;
    exposure_vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/adapt_exposure.frag"))->vulkan_shader;
    exposure_vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    exposure_vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    exposure_vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.render_pass = h_deferred_renderer->tone_mapping_pass.render_pass;
    exposure_vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_deferred_renderer->tone_mapping_pass.framebuffer.width;
    exposure_vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_deferred_renderer->tone_mapping_pass.framebuffer.height;

    h_deferred_renderer->tone_mapping_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&exposure_vulkan_graphics_pipeline_create_info);

    tg_vulkan_descriptor_set_update_color_image(h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, &h_deferred_renderer->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, h_deferred_renderer->tone_mapping_pass.exposure_storage_buffer.buffer, 1);
    tg_vulkan_descriptor_set_update_color_image(h_deferred_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, &h_deferred_renderer->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(h_deferred_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, h_deferred_renderer->tone_mapping_pass.exposure_storage_buffer.buffer, 1);

    h_deferred_renderer->tone_mapping_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(h_deferred_renderer->tone_mapping_pass.command_buffer, 0, TG_NULL);
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_deferred_renderer->tone_mapping_pass.command_buffer,
            &h_deferred_renderer->shading_pass.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(h_deferred_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_deferred_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline_layout, 0, 1, &h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(h_deferred_renderer->tone_mapping_pass.command_buffer, 1, 1, 1);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_deferred_renderer->tone_mapping_pass.command_buffer,
            &h_deferred_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_deferred_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_deferred_renderer->tone_mapping_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(h_deferred_renderer->tone_mapping_pass.command_buffer, 0, 1, &h_deferred_renderer->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_deferred_renderer->tone_mapping_pass.command_buffer, h_deferred_renderer->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_deferred_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_deferred_renderer->tone_mapping_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_deferred_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_deferred_renderer->tone_mapping_pass.command_buffer, h_deferred_renderer->tone_mapping_pass.render_pass, &h_deferred_renderer->tone_mapping_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_deferred_renderer->tone_mapping_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_deferred_renderer->tone_mapping_pass.command_buffer);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_deferred_renderer->tone_mapping_pass.command_buffer,
            &h_deferred_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    vkEndCommandBuffer(h_deferred_renderer->tone_mapping_pass.command_buffer);
}

static void tg__destroy_geometry_pass(tg_deferred_renderer_h h_deferred_renderer)
{
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_color_image_destroy(&h_deferred_renderer->geometry_pass.p_color_attachments[i]);
    }
    tg_vulkan_render_pass_destroy(h_deferred_renderer->geometry_pass.render_pass);
    tg_vulkan_framebuffer_destroy(&h_deferred_renderer->geometry_pass.framebuffer);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_deferred_renderer->geometry_pass.command_buffer);
}

static void tg__destroy_shading_pass(tg_deferred_renderer_h h_deferred_renderer)
{
    tg_vulkan_color_image_destroy(&h_deferred_renderer->shading_pass.color_attachment);
    tg_vulkan_buffer_destroy(&h_deferred_renderer->shading_pass.vbo);
    tg_vulkan_buffer_destroy(&h_deferred_renderer->shading_pass.ibo);
    tg_vulkan_semaphore_destroy(h_deferred_renderer->shading_pass.rendering_finished_semaphore);
    tg_vulkan_render_pass_destroy(h_deferred_renderer->shading_pass.render_pass);
    tg_vulkan_framebuffer_destroy(&h_deferred_renderer->shading_pass.framebuffer);
    tg_vulkan_pipeline_destroy(&h_deferred_renderer->shading_pass.graphics_pipeline);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_deferred_renderer->shading_pass.command_buffer);
    tg_vulkan_buffer_destroy(&h_deferred_renderer->shading_pass.lighting_ubo);
}

static void tg__destroy_tone_mapping_pass(tg_deferred_renderer_h h_deferred_renderer)
{
    tg_vulkan_pipeline_destroy(&h_deferred_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline);
    tg_vulkan_buffer_destroy(&h_deferred_renderer->tone_mapping_pass.exposure_storage_buffer);
    tg_vulkan_render_pass_destroy(h_deferred_renderer->tone_mapping_pass.render_pass);
    tg_vulkan_framebuffer_destroy(&h_deferred_renderer->tone_mapping_pass.framebuffer);
    tg_vulkan_pipeline_destroy(&h_deferred_renderer->tone_mapping_pass.graphics_pipeline);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_deferred_renderer->tone_mapping_pass.command_buffer);
}





tg_deferred_renderer_h tg_vulkan_deferred_renderer_create(tg_camera_h h_camera, tg_depth_image* p_depth_image)
{
    TG_ASSERT(h_camera);

    tg_deferred_renderer_h h_deferred_renderer = TG_MEMORY_ALLOC(sizeof(*h_deferred_renderer));
    h_deferred_renderer->h_camera = h_camera;
    tg__init_geometry_pass(h_deferred_renderer);
    tg__init_shading_pass(h_deferred_renderer, p_depth_image);
    tg__init_tone_mapping_pass(h_deferred_renderer);
    return h_deferred_renderer;
}

void tg_vulkan_deferred_renderer_destroy(tg_deferred_renderer_h h_deferred_renderer)
{
    TG_ASSERT(h_deferred_renderer);

    tg__destroy_tone_mapping_pass(h_deferred_renderer);
    tg__destroy_shading_pass(h_deferred_renderer);
    tg__destroy_geometry_pass(h_deferred_renderer);
    TG_MEMORY_FREE(h_deferred_renderer);
}



void tg_vulkan_deferred_renderer_begin(tg_deferred_renderer_h h_deferred_renderer)
{
    TG_ASSERT(h_deferred_renderer);

    tg_vulkan_command_buffer_begin(h_deferred_renderer->geometry_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(h_deferred_renderer->geometry_pass.command_buffer, h_deferred_renderer->geometry_pass.render_pass, &h_deferred_renderer->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void tg_vulkan_deferred_renderer_push_directional_light(tg_deferred_renderer_h h_deferred_renderer, v3 direction, v3 color)
{
    tg_light_setup* p_light_setup = (tg_light_setup*)h_deferred_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;
    
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

void tg_vulkan_deferred_renderer_push_point_light(tg_deferred_renderer_h h_deferred_renderer, v3 position, v3 color)
{
    tg_light_setup* p_light_setup = (tg_light_setup*)h_deferred_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;

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

void tg_vulkan_deferred_renderer_execute(tg_deferred_renderer_h h_deferred_renderer, VkCommandBuffer command_buffer)
{
    TG_ASSERT(h_deferred_renderer && command_buffer);

    vkCmdExecuteCommands(h_deferred_renderer->geometry_pass.command_buffer, 1, &command_buffer);
}

void tg_vulkan_deferred_renderer_end(tg_deferred_renderer_h h_deferred_renderer)
{
    TG_ASSERT(h_deferred_renderer);

    vkCmdEndRenderPass(h_deferred_renderer->geometry_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(h_deferred_renderer->geometry_pass.command_buffer));

    tg_vulkan_fence_wait(h_deferred_renderer->h_camera->render_target.fence);
    tg_vulkan_fence_reset(h_deferred_renderer->h_camera->render_target.fence);

    tg_vulkan_buffer_flush_mapped_memory(&h_deferred_renderer->shading_pass.lighting_ubo);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_deferred_renderer->geometry_pass.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &h_deferred_renderer->shading_pass.rendering_finished_semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);

    *((v3*)h_deferred_renderer->shading_pass.camera_ubo.memory.p_mapped_device_memory) = h_deferred_renderer->h_camera->position;

    const VkPipelineStageFlags p_pipeline_stage_flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &h_deferred_renderer->shading_pass.rendering_finished_semaphore;
    shading_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &h_deferred_renderer->shading_pass.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &h_deferred_renderer->shading_pass.rendering_finished_semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);

    VkSubmitInfo tone_mapping_submit_info = { 0 };
    tone_mapping_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    tone_mapping_submit_info.pNext = TG_NULL;
    tone_mapping_submit_info.waitSemaphoreCount = 1;
    tone_mapping_submit_info.pWaitSemaphores = &h_deferred_renderer->shading_pass.rendering_finished_semaphore;
    tone_mapping_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    tone_mapping_submit_info.commandBufferCount = 1;
    tone_mapping_submit_info.pCommandBuffers = &h_deferred_renderer->tone_mapping_pass.command_buffer;
    tone_mapping_submit_info.signalSemaphoreCount = 0;
    tone_mapping_submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &tone_mapping_submit_info, h_deferred_renderer->h_camera->render_target.fence);
}

void tg_vulkan_deferred_renderer_pop_all_lights(tg_deferred_renderer_h h_deferred_renderer)
{
    tg_light_setup* p_light_setup = (tg_light_setup*)h_deferred_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;

    p_light_setup->directional_light_count = 0;
    p_light_setup->point_light_count = 0;
}



void tg_vulkan_deferred_renderer_on_window_resize(tg_deferred_renderer_h h_deferred_renderer, u32 width, u32 height)
{
}

#endif
