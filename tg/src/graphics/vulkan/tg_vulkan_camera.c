#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"



#define TG_SHADOW_MAP_WIDTH                             1024
#define TG_SHADOW_MAP_HEIGHT                            1024



static void tg__init_shadow_pass(tg_camera_h h_camera)
{
    tg_vulkan_depth_image_create_info depth_image_create_info = { 0 };
    depth_image_create_info.width = TG_SHADOW_MAP_WIDTH;
    depth_image_create_info.height = TG_SHADOW_MAP_HEIGHT;
    depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
    depth_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    h_camera->shadow_pass.shadow_map = tg_vulkan_depth_image_create(&depth_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: global! also for other ones in this file!
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, &h_camera->shadow_pass.shadow_map, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

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

    h_camera->shadow_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    h_camera->shadow_pass.framebuffer = tg_vulkan_framebuffer_create(h_camera->shadow_pass.render_pass, 1, &h_camera->shadow_pass.shadow_map.image_view, TG_SHADOW_MAP_WIDTH, TG_SHADOW_MAP_HEIGHT);
    h_camera->shadow_pass.lightspace_ubo = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

static void tg__init_capture_pass(tg_camera_h h_camera)
{
    h_camera->capture_pass.h_deferred_renderer = tg_vulkan_deferred_renderer_create(h_camera);
    h_camera->capture_pass.h_forward_renderer = tg_vulkan_forward_renderer_create(h_camera);
}

static void tg__init_clear_pass(tg_camera_h h_camera)
{
    h_camera->clear_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(h_camera->clear_pass.command_buffer, 0, TG_NULL);

    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_camera->clear_pass.command_buffer,
        &h_camera->render_target.color_attachment,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_color_image(h_camera->clear_pass.command_buffer, &h_camera->render_target.color_attachment);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_camera->clear_pass.command_buffer,
        &h_camera->render_target.color_attachment,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_camera->clear_pass.command_buffer,
        &h_camera->render_target.depth_attachment,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_depth_image(h_camera->clear_pass.command_buffer, &h_camera->render_target.depth_attachment);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_camera->clear_pass.command_buffer,
        &h_camera->render_target.depth_attachment,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    );

    vkEndCommandBuffer(h_camera->clear_pass.command_buffer);
}

static void tg__init_present_pass(tg_camera_h h_camera)
{
    tg_vulkan_buffer staging_buffer = { 0 };

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };

    p_vertices[0].position.x = -1.0f;
    p_vertices[0].position.y = 1.0f;
    p_vertices[0].uv.x = 0.0f;
    p_vertices[0].uv.y = 1.0f;

    p_vertices[1].position.x = 1.0f;
    p_vertices[1].position.y = 1.0f;
    p_vertices[1].uv.x = 1.0f;
    p_vertices[1].uv.y = 1.0f;

    p_vertices[2].position.x = 1.0f;
    p_vertices[2].position.y = -1.0f;
    p_vertices[2].uv.x = 1.0f;
    p_vertices[2].uv.y = 0.0f;

    p_vertices[3].position.x = -1.0f;
    p_vertices[3].position.y = -1.0f;
    p_vertices[3].uv.x = 0.0f;
    p_vertices[3].uv.y = 0.0f;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_vertices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
    h_camera->present_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, h_camera->present_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };

    p_indices[0] = 0;
    p_indices[1] = 1;
    p_indices[2] = 2;
    p_indices[3] = 2;
    p_indices[4] = 3;
    p_indices[5] = 0;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    h_camera->present_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, h_camera->present_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    h_camera->present_pass.image_acquired_semaphore = tg_vulkan_semaphore_create();
    h_camera->present_pass.semaphore = tg_vulkan_semaphore_create();

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

    h_camera->present_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        h_camera->present_pass.p_framebuffers[i] = tg_vulkan_framebuffer_create(h_camera->present_pass.render_pass, 1, &p_swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    h_camera->present_pass.vertex_shader = ((tg_vertex_shader_h)tg_assets_get_asset("shaders/present.vert"))->vulkan_shader;
    h_camera->present_pass.fragment_shader = ((tg_fragment_shader_h)tg_assets_get_asset("shaders/present.frag"))->vulkan_shader;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.vertex_shader = h_camera->present_pass.vertex_shader;
    vulkan_graphics_pipeline_create_info.fragment_shader = h_camera->present_pass.fragment_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.render_pass = h_camera->present_pass.render_pass;

    h_camera->present_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);

    tg_vulkan_command_buffers_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_camera->render_target.color_attachment.sampler;
    descriptor_image_info.imageView = h_camera->render_target.color_attachment.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_camera->present_pass.graphics_pipeline.descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        tg_vulkan_command_buffer_begin(h_camera->present_pass.p_command_buffers[i], 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_camera->present_pass.p_command_buffers[i],
            &h_camera->render_target.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        vkCmdBindPipeline(h_camera->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_camera->present_pass.graphics_pipeline.pipeline);
        vkCmdBindVertexBuffers(h_camera->present_pass.p_command_buffers[i], 0, 1, &h_camera->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_camera->present_pass.p_command_buffers[i], h_camera->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_camera->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_camera->present_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_camera->present_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_camera->present_pass.p_command_buffers[i], h_camera->present_pass.render_pass, &h_camera->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_camera->present_pass.p_command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_camera->present_pass.p_command_buffers[i]);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_camera->present_pass.p_command_buffers[i],
            &h_camera->render_target.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_camera->present_pass.p_command_buffers[i],
            &h_camera->render_target.depth_attachment,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        tg_vulkan_command_buffer_cmd_clear_color_image(h_camera->present_pass.p_command_buffers[i], &h_camera->render_target.color_attachment);
        tg_vulkan_command_buffer_cmd_clear_depth_image(h_camera->present_pass.p_command_buffers[i], &h_camera->render_target.depth_attachment);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_camera->present_pass.p_command_buffers[i],
            &h_camera->render_target.color_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_camera->present_pass.p_command_buffers[i],
            &h_camera->render_target.depth_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );

        VK_CALL(vkEndCommandBuffer(h_camera->present_pass.p_command_buffers[i]));
    }
}

static tg_camera* tg__init_available()
{
    tg_camera* p_camera = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_cameras, p_camera);

    p_camera->type = TG_HANDLE_TYPE_CAMERA;
    p_camera->view_projection_ubo = tg_vulkan_buffer_create(2 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    vulkan_depth_image_create_info.width = swapchain_extent.width;
    vulkan_depth_image_create_info.height = swapchain_extent.height;
    vulkan_depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
    vulkan_depth_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    p_camera->render_target = tg_vulkan_render_target_create(&vulkan_color_image_create_info, &vulkan_depth_image_create_info, VK_FENCE_CREATE_SIGNALED_BIT);
    p_camera->render_command_count = 0;

    tg__init_shadow_pass(p_camera);
    tg__init_capture_pass(p_camera);
    tg__init_present_pass(p_camera);
    tg__init_clear_pass(p_camera);

    return p_camera;
}

static void tg__destroy_capture_pass(tg_camera_h h_camera)
{
    tg_vulkan_forward_renderer_destroy(h_camera->capture_pass.h_forward_renderer);
    tg_vulkan_deferred_renderer_destroy(h_camera->capture_pass.h_deferred_renderer);
}

static void tg__destroy_clear_pass(tg_camera_h h_camera)
{
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_camera->clear_pass.command_buffer);
}

static void tg__destroy_present_pass(tg_camera_h h_camera)
{
    tg_vulkan_command_buffers_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_command_buffers);
    tg_vulkan_pipeline_destroy(&h_camera->present_pass.graphics_pipeline);
    tg_vulkan_framebuffers_destroy(TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_framebuffers);
    tg_vulkan_render_pass_destroy(h_camera->present_pass.render_pass);
    tg_vulkan_semaphore_destroy(h_camera->present_pass.semaphore);
    tg_vulkan_semaphore_destroy(h_camera->present_pass.image_acquired_semaphore);
    tg_vulkan_buffer_destroy(&h_camera->present_pass.ibo);
    tg_vulkan_buffer_destroy(&h_camera->present_pass.vbo);
}





tg_camera_h tg_camera_create_orthographic(v3 position, f32 pitch, f32 yaw, f32 roll, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(left != right && bottom != top && far != near);

	tg_camera_h h_camera = tg__init_available();
	tg_camera_set_view(h_camera, position, pitch, yaw, roll);
	tg_camera_set_orthographic_projection(h_camera, left, right, bottom, top, far, near);
	return h_camera;
}

tg_camera_h tg_camera_create_perspective(v3 position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(near < 0 && far < 0 && near > far);

    tg_camera_h h_camera = tg__init_available();
	tg_camera_set_view(h_camera, position, pitch, yaw, roll);
	tg_camera_set_perspective_projection(h_camera, fov_y, near, far);
	return h_camera;
}

void tg_camera_destroy(tg_camera_h h_camera)
{
	TG_ASSERT(h_camera);

    tg__destroy_clear_pass(h_camera);
    tg__destroy_present_pass(h_camera);
    tg__destroy_capture_pass(h_camera);
    tg_vulkan_render_target_destroy(&h_camera->render_target);
    tg_vulkan_buffer_destroy(&h_camera->view_projection_ubo);
    TG_VULKAN_RELEASE_HANDLE(h_camera);
}



void tg_camera_begin(tg_camera_h h_camera)
{
    TG_ASSERT(h_camera);

    h_camera->render_command_count = 0;
}

void tg_camera_push_directional_light(tg_camera_h h_camera, v3 direction, v3 color)
{
    // TODO: forward renderer
    tg_vulkan_deferred_renderer_push_directional_light(h_camera->capture_pass.h_deferred_renderer, direction, color);
}

void tg_camera_push_point_light(tg_camera_h h_camera, v3 position, v3 color)
{
    // TODO: forward renderer
    tg_vulkan_deferred_renderer_push_point_light(h_camera->capture_pass.h_deferred_renderer, position, color);
}

void tg_camera_execute(tg_camera_h h_camera, tg_render_command_h h_render_command)
{
    TG_ASSERT(h_camera && h_render_command);

    tg_vulkan_render_command_camera_info* p_camera_info = TG_NULL;
    for (u32 i = 0; i < h_render_command->camera_info_count; i++)
    {
        if (h_render_command->p_camera_infos[i].h_camera == h_camera)
        {
            p_camera_info = &h_render_command->p_camera_infos[i];
            break;
        }
    }

    // TODO: this crashes, when a render command is created before the camera. that way,
    // the camera info is not set up for that specific camera. think of threading in
    // this case!
    TG_ASSERT(p_camera_info);
    TG_ASSERT(h_camera->render_command_count < TG_MAX_RENDER_COMMANDS);

    h_camera->ph_render_commands[h_camera->render_command_count++] = h_render_command;
}

void tg_camera_end(tg_camera_h h_camera)
{
    tg_vulkan_command_buffer_begin(h_camera->shadow_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(h_camera->shadow_pass.command_buffer, h_camera->shadow_pass.render_pass, &h_camera->shadow_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // TODO: decide, which lights cast shadows!
    const m4 light_projection = tgm_m4_orthographic(-10.0f, 10.0f, -10.0f, 10.0f, 256.0f, 1.0f);
    const m4 light_view = tgm_m4_look_at((v3) { 128.0f + 4.0f, 143.0f, 112.0f }, (v3) { 128.0f, 143.0f, 112.0f }, (v3) { 0.0f, 1.0f, 0.0f });
    const m4 lightspace = tgm_m4_mul(light_projection, light_view);
    *((m4*)h_camera->shadow_pass.lightspace_ubo.memory.p_mapped_device_memory) = lightspace;

    for (u32 i = 0; i < h_camera->render_command_count; i++)
    {
        tg_render_command_h h_render_command = h_camera->ph_render_commands[i];
        if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
        {
            for (u32 j = 0; j < h_render_command->camera_info_count; j++)
            {
                tg_vulkan_render_command_camera_info* p_camera_info = &h_render_command->p_camera_infos[j];
                if (p_camera_info->h_camera == h_camera)
                {
                    vkCmdExecuteCommands(h_camera->shadow_pass.command_buffer, 1, &p_camera_info->shadow_command_buffer);
                    break;
                }
            }
        }
    }

    vkCmdEndRenderPass(h_camera->shadow_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(h_camera->shadow_pass.command_buffer));








    tg_vulkan_deferred_renderer_begin(h_camera->capture_pass.h_deferred_renderer);
    for (u32 i = 0; i < h_camera->render_command_count; i++)
    {
        tg_render_command_h h_render_command = h_camera->ph_render_commands[i];
        if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
        {
            for (u32 j = 0; j < h_render_command->camera_info_count; j++)
            {
                tg_vulkan_render_command_camera_info* p_camera_info = &h_render_command->p_camera_infos[j];
                if (p_camera_info->h_camera == h_camera)
                {
                    tg_vulkan_deferred_renderer_execute(h_camera->capture_pass.h_deferred_renderer, p_camera_info->command_buffer);

                    // TODO: lod
                    //{
                    //    const tg_bounds* p_bounds = &h_render_command->ph_lod_meshes[0]->bounds;
                    //
                    //    v4 p_points[8] = { 0 };
                    //    p_points[0] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                    //    p_points[1] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                    //    p_points[2] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                    //    p_points[3] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                    //    p_points[4] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                    //    p_points[5] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                    //    p_points[6] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                    //    p_points[7] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
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
                    //            tg_vulkan_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[2]);
                    //        }
                    //        else
                    //        {
                    //            tg_vulkan_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[3]);
                    //        }
                    //    }
                    //}
                    break;
                }
            }
        }
    }
    tg_vulkan_deferred_renderer_end(h_camera->capture_pass.h_deferred_renderer);

    tg_vulkan_forward_renderer_begin(h_camera->capture_pass.h_forward_renderer);
    for (u32 i = 0; i < h_camera->render_command_count; i++)
    {
        if (h_camera->ph_render_commands[i]->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD)
        {
            tg_vulkan_forward_renderer_execute(h_camera->capture_pass.h_forward_renderer, h_camera->ph_render_commands[i]);
        }
    }
    tg_vulkan_forward_renderer_end(h_camera->capture_pass.h_forward_renderer);
}

void tg_camera_present(tg_camera_h h_camera)
{
    TG_ASSERT(h_camera);

    u32 current_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, h_camera->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    tg_vulkan_fence_wait(h_camera->render_target.fence);
    tg_vulkan_fence_reset(h_camera->render_target.fence);

    const VkSemaphore p_wait_semaphores[1] = { h_camera->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[1] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

    VkSubmitInfo draw_submit_info = { 0 };
    draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    draw_submit_info.pNext = TG_NULL;
    draw_submit_info.waitSemaphoreCount = 1;
    draw_submit_info.pWaitSemaphores = p_wait_semaphores;
    draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
    draw_submit_info.commandBufferCount = 1;
    draw_submit_info.pCommandBuffers = &h_camera->present_pass.p_command_buffers[current_image];
    draw_submit_info.signalSemaphoreCount = 1;
    draw_submit_info.pSignalSemaphores = &h_camera->present_pass.semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &draw_submit_info, h_camera->render_target.fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &h_camera->present_pass.semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    tg_vulkan_queue_present(&present_info);
}

void tg_camera_clear(tg_camera_h h_camera) // TODO: should this be combined with begin?
{
    TG_ASSERT(h_camera);

    tg_vulkan_fence_wait(h_camera->render_target.fence);
    tg_vulkan_fence_reset(h_camera->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_camera->clear_pass.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_camera->render_target.fence);

    // TODO: forward renderer
    tg_vulkan_deferred_renderer_pop_all_lights(h_camera->capture_pass.h_deferred_renderer);
}



v3 tg_camera_get_position(tg_camera_h h_camera)
{
    TG_ASSERT(h_camera);

    return h_camera->position;
}

tg_render_target_h tg_camera_get_render_target(tg_camera_h h_camera)
{
    TG_ASSERT(h_camera);

    return &h_camera->render_target;
}

void tg_camera_set_orthographic_projection(tg_camera_h h_camera, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(h_camera && left != right && bottom != top && far != near);

	TG_CAMERA_PROJ(h_camera) = tgm_m4_orthographic(left, right, bottom, top, far, near); // TODO: flush
}

void tg_camera_set_perspective_projection(tg_camera_h h_camera, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(h_camera && near < 0 && far < 0 && near > far);

    TG_CAMERA_PROJ(h_camera) = tgm_m4_perspective(fov_y, tg_platform_get_window_aspect_ratio(), near, far);
}

void tg_camera_set_view(tg_camera_h h_camera, v3 position, f32 pitch, f32 yaw, f32 roll)
{
	TG_ASSERT(h_camera);

    h_camera->position = position;
	const m4 inverse_rotation = tgm_m4_inverse(tgm_m4_euler(pitch, yaw, roll));
	const m4 inverse_translation = tgm_m4_translate(tgm_v3_neg(position));
    TG_CAMERA_VIEW(h_camera) = tgm_m4_mul(inverse_rotation, inverse_translation);
}

#endif
