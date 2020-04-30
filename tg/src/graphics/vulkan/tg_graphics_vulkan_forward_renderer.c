#include "graphics/vulkan/tg_graphics_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_camera.h"
#include "tg_entity.h"



void tg_forward_renderer_internal_init_clear_pass(tg_forward_renderer_h forward_renderer_h)
{
    forward_renderer_h->clear_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(forward_renderer_h->clear_pass.command_buffer, 0, TG_NULL);

    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->clear_pass.command_buffer,
        &forward_renderer_h->render_target.color_image,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_color_image(forward_renderer_h->clear_pass.command_buffer, &forward_renderer_h->render_target.color_image);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->clear_pass.command_buffer,
        &forward_renderer_h->render_target.color_image,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->clear_pass.command_buffer,
        &forward_renderer_h->render_target.depth_image,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_depth_image(forward_renderer_h->clear_pass.command_buffer, &forward_renderer_h->render_target.depth_image);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->clear_pass.command_buffer,
        &forward_renderer_h->render_target.depth_image,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    );

    vkEndCommandBuffer(forward_renderer_h->clear_pass.command_buffer);
}

void tg_forward_renderer_internal_init_shading_pass(tg_forward_renderer_h forward_renderer_h, const tg_camera* p_camera, tg_render_target_h render_target_h)
{
    forward_renderer_h->shading_pass.p_camera = p_camera;

    if (!render_target_h)
    {
        tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
        {
            vulkan_color_image_create_info.width = swapchain_extent.width;
            vulkan_color_image_create_info.height = swapchain_extent.height;
            vulkan_color_image_create_info.mip_levels = 1;
            vulkan_color_image_create_info.format = TG_FORWARD_RENDERER_COLOR_ATTACHMENT_FORMAT;
            vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
            vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
            vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
        forward_renderer_h->render_target.color_image = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

        tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
        {
            vulkan_depth_image_create_info.width = swapchain_extent.width;
            vulkan_depth_image_create_info.height = swapchain_extent.height;
            vulkan_depth_image_create_info.format = TG_FORWARD_RENDERER_DEPTH_ATTACHMENT_FORMAT;
            vulkan_depth_image_create_info.min_filter = VK_FILTER_LINEAR;
            vulkan_depth_image_create_info.mag_filter = VK_FILTER_LINEAR;
            vulkan_depth_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            vulkan_depth_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            vulkan_depth_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
        forward_renderer_h->render_target.depth_image = tg_vulkan_depth_image_create(&vulkan_depth_image_create_info);

        VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &forward_renderer_h->render_target.color_image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, &forward_renderer_h->render_target.depth_image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

        forward_renderer_h->render_target.fence = tg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);
    }
    else
    {
        forward_renderer_h->render_target = *render_target_h;
    }

    forward_renderer_h->shading_pass.view_projection_ubo = tg_vulkan_buffer_create(sizeof(tg_camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkAttachmentDescription p_attachment_descriptions[2] = { 0 };
    {
        p_attachment_descriptions[0].flags = 0;
        
        if (!render_target_h)
        {
            p_attachment_descriptions[0].format = TG_FORWARD_RENDERER_COLOR_ATTACHMENT_FORMAT;
        }
        else
        {
            p_attachment_descriptions[0].format = render_target_h->color_image.format;
        }
        p_attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        p_attachment_descriptions[1].flags = 0;
        p_attachment_descriptions[1].format = TG_FORWARD_RENDERER_DEPTH_ATTACHMENT_FORMAT;
        p_attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference color_attachment_reference = { 0 };
    {
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    };
    VkAttachmentReference depth_buffer_attachment_reference = { 0 };
    {
        depth_buffer_attachment_reference.attachment = 1;
        depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
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
    }
    VkSubpassDependency subpass_dependency = { 0 };
    {
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    forward_renderer_h->shading_pass.render_pass = tg_vulkan_render_pass_create(2, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);

    VkImageView p_image_views[2] = { forward_renderer_h->render_target.color_image.image_view, forward_renderer_h->render_target.depth_image.image_view };
    forward_renderer_h->shading_pass.framebuffer = tg_vulkan_framebuffer_create(forward_renderer_h->shading_pass.render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);
    forward_renderer_h->shading_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void tg_forward_renderer_internal_init_present_pass(tg_forward_renderer_h forward_renderer_h)
{
    tg_vulkan_buffer staging_buffer = { 0 };

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };
    {
        // TODO: y is inverted, because image has to be flipped. add projection matrix to present vertex shader?
        // TODO: all the other ones in deferred are wrong. also there is no back face culling enabled in those...
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
    }
    staging_buffer = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_vertices, sizeof(p_vertices));
    forward_renderer_h->present_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, forward_renderer_h->present_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };
    {
        p_indices[0] = 0;
        p_indices[1] = 1;
        p_indices[2] = 2;
        p_indices[3] = 2;
        p_indices[4] = 3;
        p_indices[5] = 0;
    }
    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_indices, sizeof(p_indices));
    forward_renderer_h->present_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, forward_renderer_h->present_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    forward_renderer_h->present_pass.image_acquired_semaphore = tg_vulkan_semaphore_create();
    forward_renderer_h->present_pass.semaphore = tg_vulkan_semaphore_create();

    VkAttachmentReference color_attachment_reference = { 0 };
    {
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentDescription attachment_description = { 0 };
    {
        attachment_description.flags = 0;
        attachment_description.format = surface.format.format;
        attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
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
    }
    forward_renderer_h->present_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        forward_renderer_h->present_pass.framebuffers[i] = tg_vulkan_framebuffer_create(forward_renderer_h->present_pass.render_pass, 1, &swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;
    }
    forward_renderer_h->present_pass.descriptor = tg_vulkan_descriptor_create(1, &descriptor_set_layout_binding);

    forward_renderer_h->present_pass.vertex_shader_h = tg_vulkan_shader_module_create("shaders/present.vert");
    forward_renderer_h->present_pass.fragment_shader_h = tg_vulkan_shader_module_create("shaders/present.frag");

    forward_renderer_h->present_pass.pipeline_layout = tg_vulkan_pipeline_layout_create(1, &forward_renderer_h->present_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_vulkan_screen_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };
    {
        p_vertex_input_attribute_descriptions[0].binding = 0;
        p_vertex_input_attribute_descriptions[0].location = 0;
        p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_vulkan_screen_vertex, position);

        p_vertex_input_attribute_descriptions[1].binding = 0;
        p_vertex_input_attribute_descriptions[1].location = 1;
        p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_vulkan_screen_vertex, uv);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = TG_NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;
    }
    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    {
        vulkan_graphics_pipeline_create_info.vertex_shader = forward_renderer_h->present_pass.vertex_shader_h;
        vulkan_graphics_pipeline_create_info.fragment_shader = forward_renderer_h->present_pass.fragment_shader_h;
        vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
        vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.attachment_count = 1;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        vulkan_graphics_pipeline_create_info.pipeline_layout = forward_renderer_h->present_pass.pipeline_layout;
        vulkan_graphics_pipeline_create_info.render_pass = forward_renderer_h->present_pass.render_pass;
    }
    forward_renderer_h->present_pass.graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);

    tg_vulkan_command_buffers_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, forward_renderer_h->present_pass.command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = forward_renderer_h->render_target.color_image.sampler;
        descriptor_image_info.imageView = forward_renderer_h->render_target.color_image.image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = TG_NULL;
        write_descriptor_set.dstSet = forward_renderer_h->present_pass.descriptor.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set.pImageInfo = &descriptor_image_info;
        write_descriptor_set.pBufferInfo = TG_NULL;
        write_descriptor_set.pTexelBufferView = TG_NULL;
    }
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        tg_vulkan_command_buffer_begin(forward_renderer_h->present_pass.command_buffers[i], 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            forward_renderer_h->present_pass.command_buffers[i],
            &forward_renderer_h->render_target.color_image,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        vkCmdBindPipeline(forward_renderer_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, forward_renderer_h->present_pass.graphics_pipeline);
        vkCmdBindVertexBuffers(forward_renderer_h->present_pass.command_buffers[i], 0, 1, &forward_renderer_h->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(forward_renderer_h->present_pass.command_buffers[i], forward_renderer_h->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(forward_renderer_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, forward_renderer_h->present_pass.pipeline_layout, 0, 1, &forward_renderer_h->present_pass.descriptor.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        {
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = TG_NULL;
            render_pass_begin_info.renderPass = forward_renderer_h->present_pass.render_pass;
            render_pass_begin_info.framebuffer = forward_renderer_h->present_pass.framebuffers[i];
            render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
            render_pass_begin_info.renderArea.extent = swapchain_extent;
            render_pass_begin_info.clearValueCount = 0;
            render_pass_begin_info.pClearValues = TG_NULL;
        }
        vkCmdBeginRenderPass(forward_renderer_h->present_pass.command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(forward_renderer_h->present_pass.command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(forward_renderer_h->present_pass.command_buffers[i]);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            forward_renderer_h->present_pass.command_buffers[i],
            &forward_renderer_h->render_target.color_image,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            forward_renderer_h->present_pass.command_buffers[i],
            &forward_renderer_h->render_target.depth_image,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        tg_vulkan_command_buffer_cmd_clear_color_image(forward_renderer_h->present_pass.command_buffers[i], &forward_renderer_h->render_target.color_image);
        tg_vulkan_command_buffer_cmd_clear_depth_image(forward_renderer_h->present_pass.command_buffers[i], &forward_renderer_h->render_target.depth_image);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            forward_renderer_h->present_pass.command_buffers[i],
            &forward_renderer_h->render_target.color_image,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            forward_renderer_h->present_pass.command_buffers[i],
            &forward_renderer_h->render_target.depth_image,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );

        VK_CALL(vkEndCommandBuffer(forward_renderer_h->present_pass.command_buffers[i]));
    }
}



void tg_forward_renderer_begin(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    tg_vulkan_fence_wait(forward_renderer_h->render_target.fence);
    tg_vulkan_fence_reset(forward_renderer_h->render_target.fence);

    ((tg_camera*)forward_renderer_h->shading_pass.view_projection_ubo.p_mapped_device_memory)->view = forward_renderer_h->shading_pass.p_camera->view;
    ((tg_camera*)forward_renderer_h->shading_pass.view_projection_ubo.p_mapped_device_memory)->projection = forward_renderer_h->shading_pass.p_camera->projection;
    // TODO: vkFlushMappedMemoryRanges

    tg_vulkan_command_buffer_begin(forward_renderer_h->shading_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    {
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        render_pass_begin_info.renderPass = forward_renderer_h->shading_pass.render_pass;
        render_pass_begin_info.framebuffer = forward_renderer_h->shading_pass.framebuffer;
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = 0;
        render_pass_begin_info.pClearValues = TG_NULL;
    }
    vkCmdBeginRenderPass(forward_renderer_h->shading_pass.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void tg_forward_renderer_clear(tg_forward_renderer_h forward_renderer_h)
{
    TG_ASSERT(forward_renderer_h);

    tg_vulkan_fence_wait(forward_renderer_h->render_target.fence);
    tg_vulkan_fence_reset(forward_renderer_h->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &forward_renderer_h->clear_pass.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, forward_renderer_h->render_target.fence));
}

tg_forward_renderer_h tg_forward_renderer_create(const tg_camera* p_camera, tg_render_target_h render_target_h)
{
	TG_ASSERT(p_camera);

    tg_forward_renderer_h forward_renderer_h = TG_MEMORY_ALLOC(sizeof(*forward_renderer_h));

    tg_forward_renderer_internal_init_shading_pass(forward_renderer_h, p_camera, render_target_h);
    tg_forward_renderer_internal_init_present_pass(forward_renderer_h);
    tg_forward_renderer_internal_init_clear_pass(forward_renderer_h);

    return forward_renderer_h;
}

void tg_forward_renderer_destroy(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    TG_MEMORY_FREE(forward_renderer_h);
}

void tg_forward_renderer_draw(tg_forward_renderer_h forward_renderer_h, tg_entity* p_entity)
{
	TG_ASSERT(forward_renderer_h && p_entity && p_entity->graphics_data_ptr_h->renderer_h == forward_renderer_h);

    vkCmdExecuteCommands(forward_renderer_h->shading_pass.command_buffer, 1, &p_entity->graphics_data_ptr_h->command_buffer);
}

void tg_forward_renderer_end(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    vkCmdEndRenderPass(forward_renderer_h->shading_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(forward_renderer_h->shading_pass.command_buffer));

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &forward_renderer_h->shading_pass.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, forward_renderer_h->render_target.fence));
}

tg_render_target_h tg_forward_renderer_get_render_target(tg_forward_renderer_h forward_renderer_h)
{
    TG_ASSERT(forward_renderer_h);

    return TG_NULL;
    // TODO: this must be created on start up...
    // actually, this can stay in here on stack and doesnt need to be on the heap
}

void tg_forward_renderer_on_window_resize(tg_forward_renderer_h forward_renderer_h, u32 width, u32 height)
{
	TG_ASSERT(forward_renderer_h && width && height);
}

void tg_forward_renderer_present(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    u32 current_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, forward_renderer_h->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    tg_vulkan_fence_wait(forward_renderer_h->render_target.fence);
    tg_vulkan_fence_reset(forward_renderer_h->render_target.fence);

    const VkSemaphore p_wait_semaphores[1] = { forward_renderer_h->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[1] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
    VkSubmitInfo draw_submit_info = { 0 };
    {
        draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        draw_submit_info.pNext = TG_NULL;
        draw_submit_info.waitSemaphoreCount = 1;
        draw_submit_info.pWaitSemaphores = p_wait_semaphores;
        draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
        draw_submit_info.commandBufferCount = 1;
        draw_submit_info.pCommandBuffers = &forward_renderer_h->present_pass.command_buffers[current_image];
        draw_submit_info.signalSemaphoreCount = 1;
        draw_submit_info.pSignalSemaphores = &forward_renderer_h->present_pass.semaphore;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &draw_submit_info, forward_renderer_h->render_target.fence));

    VkPresentInfoKHR present_info = { 0 };
    {
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = TG_NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &forward_renderer_h->present_pass.semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = TG_NULL;
    }
    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));

    vkQueueWaitIdle(graphics_queue.queue);
}


#endif
