#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_vulkan_transvoxel_terrain.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"



void tg_vulkan_camera_info_clear(tg_vulkan_camera_info* p_vulkan_camera_info)
{
    TG_ASSERT(p_vulkan_camera_info);

    tg_vulkan_command_buffers_free(graphics_command_pool, TG_VULKAN_MAX_LOD_COUNT, p_vulkan_camera_info->p_command_buffers);
    tg_vulkan_graphics_pipeline_destroy(p_vulkan_camera_info->graphics_pipeline);
    tg_vulkan_pipeline_layout_destroy(p_vulkan_camera_info->pipeline_layout);
    tg_vulkan_descriptor_destroy(&p_vulkan_camera_info->descriptor);
}



void tg_camera_internal_destroy_capture_pass(tg_camera_h h_camera)
{
    tg_forward_renderer_destroy(h_camera->capture_pass.h_forward_renderer);
    tg_deferred_renderer_destroy(h_camera->capture_pass.h_deferred_renderer);
}

void tg_camera_internal_destroy_clear_pass(tg_camera_h h_camera)
{
    tg_vulkan_command_buffer_free(graphics_command_pool, h_camera->clear_pass.command_buffer);
}

void tg_camera_internal_destroy_present_pass(tg_camera_h h_camera)
{
    tg_vulkan_command_buffers_free(graphics_command_pool, TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_command_buffers);
    tg_vulkan_graphics_pipeline_destroy(h_camera->present_pass.graphics_pipeline);
    tg_vulkan_pipeline_layout_destroy(h_camera->present_pass.pipeline_layout);
    tg_vulkan_descriptor_destroy(&h_camera->present_pass.descriptor);
    tg_vulkan_framebuffers_destroy(TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_framebuffers);
    tg_vulkan_render_pass_destroy(h_camera->present_pass.render_pass);
    tg_vulkan_semaphore_destroy(h_camera->present_pass.semaphore);
    tg_vulkan_semaphore_destroy(h_camera->present_pass.image_acquired_semaphore);
    tg_vulkan_buffer_destroy(&h_camera->present_pass.ibo);
    tg_vulkan_buffer_destroy(&h_camera->present_pass.vbo);
}

void tg_camera_internal_init_capture_pass(tg_camera_h h_camera)
{
    h_camera->capture_pass.h_deferred_renderer = tg_deferred_renderer_create(h_camera);
    h_camera->capture_pass.h_forward_renderer = tg_forward_renderer_create(h_camera);
}

void tg_camera_internal_init_clear_pass(tg_camera_h h_camera)
{
    h_camera->clear_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
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

void tg_camera_internal_init_present_pass(tg_camera_h h_camera)
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
    memcpy(staging_buffer.memory.p_mapped_device_memory, p_vertices, sizeof(p_vertices));
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
    memcpy(staging_buffer.memory.p_mapped_device_memory, p_indices, sizeof(p_indices));
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

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    descriptor_set_layout_binding.binding = 0;
    descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_binding.descriptorCount = 1;
    descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;

    h_camera->present_pass.descriptor = tg_vulkan_descriptor_create(1, &descriptor_set_layout_binding);

    h_camera->present_pass.vertex_shader = ((tg_vertex_shader_h)tg_assets_get_asset("shaders/present.vert"))->vulkan_shader;
    h_camera->present_pass.fragment_shader = ((tg_fragment_shader_h)tg_assets_get_asset("shaders/present.frag"))->vulkan_shader;

    h_camera->present_pass.pipeline_layout = tg_vulkan_pipeline_layout_create(1, &h_camera->present_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(tg_vulkan_screen_vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };

    p_vertex_input_attribute_descriptions[0].binding = 0;
    p_vertex_input_attribute_descriptions[0].location = 0;
    p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_vulkan_screen_vertex, position);

    p_vertex_input_attribute_descriptions[1].binding = 0;
    p_vertex_input_attribute_descriptions[1].location = 1;
    p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_vulkan_screen_vertex, uv);

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.vertex_shader = h_camera->present_pass.vertex_shader;
    vulkan_graphics_pipeline_create_info.fragment_shader = h_camera->present_pass.fragment_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.attachment_count = 1;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
    vulkan_graphics_pipeline_create_info.pipeline_layout = h_camera->present_pass.pipeline_layout;
    vulkan_graphics_pipeline_create_info.render_pass = h_camera->present_pass.render_pass;

    h_camera->present_pass.graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);
    tg_vulkan_command_buffers_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, h_camera->present_pass.p_command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_camera->render_target.color_attachment.sampler;
    descriptor_image_info.imageView = h_camera->render_target.color_attachment.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_camera->present_pass.descriptor.descriptor_set;
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
        vkCmdBindPipeline(h_camera->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_camera->present_pass.graphics_pipeline);
        vkCmdBindVertexBuffers(h_camera->present_pass.p_command_buffers[i], 0, 1, &h_camera->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_camera->present_pass.p_command_buffers[i], h_camera->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_camera->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_camera->present_pass.pipeline_layout, 0, 1, &h_camera->present_pass.descriptor.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        render_pass_begin_info.renderPass = h_camera->present_pass.render_pass;
        render_pass_begin_info.framebuffer = h_camera->present_pass.p_framebuffers[i];
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = 0;
        render_pass_begin_info.pClearValues = TG_NULL;

        vkCmdBeginRenderPass(h_camera->present_pass.p_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
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

void tg_camera_internal_init(tg_camera_h h_camera)
{
    h_camera->type = TG_HANDLE_TYPE_CAMERA;
    h_camera->view_projection_ubo = tg_vulkan_buffer_create(2 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

    h_camera->render_target = tg_vulkan_render_target_create(&vulkan_color_image_create_info, &vulkan_depth_image_create_info, VK_FENCE_CREATE_SIGNALED_BIT);
    h_camera->captured_entity_count = 0;

    tg_camera_internal_init_capture_pass(h_camera);
    tg_camera_internal_init_present_pass(h_camera);
    tg_camera_internal_init_clear_pass(h_camera);
}

void tg_camera_internal_register_entity(tg_camera_h h_camera, tg_vulkan_camera_info* p_vulkan_camera_info, tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr)
{
    p_vulkan_camera_info = &h_entity_graphics_data_ptr->p_camera_infos[h_entity_graphics_data_ptr->camera_info_count++];

    p_vulkan_camera_info->h_camera = h_camera;

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };

    p_descriptor_set_layout_bindings[0].binding = 0;
    p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_descriptor_set_layout_bindings[0].descriptorCount = 1;
    p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

    p_descriptor_set_layout_bindings[1].binding = 1;
    p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_descriptor_set_layout_bindings[1].descriptorCount = 1;
    p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;

    p_vulkan_camera_info->descriptor = tg_vulkan_descriptor_create(2, p_descriptor_set_layout_bindings);
    const b32 has_custom_descriptor_set = h_entity_graphics_data_ptr->h_material->descriptor.descriptor_pool != VK_NULL_HANDLE;

    if (has_custom_descriptor_set)
    {
        const VkDescriptorSetLayout p_descriptor_set_layouts[2] = { p_vulkan_camera_info->descriptor.descriptor_set_layout, h_entity_graphics_data_ptr->h_material->descriptor.descriptor_set_layout };
        p_vulkan_camera_info->pipeline_layout = tg_vulkan_pipeline_layout_create(2, p_descriptor_set_layouts, 0, TG_NULL);
    }
    else
    {
        p_vulkan_camera_info->pipeline_layout = tg_vulkan_pipeline_layout_create(1, &p_vulkan_camera_info->descriptor.descriptor_set_layout, 0, TG_NULL);
    }

    const tg_spirv_layout* p_layout = &h_entity_graphics_data_ptr->h_material->h_vertex_shader->vulkan_shader.layout;

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = p_layout->vertex_stride;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    const u64 vertex_input_attribute_descriptions_size = (u64)p_layout->input_resource_count * sizeof(VkVertexInputAttributeDescription);
    VkVertexInputAttributeDescription* p_vertex_input_attribute_descriptions = TG_MEMORY_STACK_ALLOC(vertex_input_attribute_descriptions_size);
    for (u8 i = 0; i < p_layout->input_resource_count; i++)
    {
        p_vertex_input_attribute_descriptions[i].binding = 0;
        p_vertex_input_attribute_descriptions[i].location = p_layout->p_input_resources[i].location;
        p_vertex_input_attribute_descriptions[i].format = p_layout->p_input_resources[i].format;
        p_vertex_input_attribute_descriptions[i].offset = p_layout->p_input_resources[i].offset;
    }

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = (u32)p_layout->input_resource_count;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.vertex_shader = h_entity_graphics_data_ptr->h_material->h_vertex_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.fragment_shader = h_entity_graphics_data_ptr->h_material->h_fragment_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_TRUE;

    switch (h_entity_graphics_data_ptr->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        vulkan_graphics_pipeline_create_info.attachment_count = TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        vulkan_graphics_pipeline_create_info.attachment_count = 1;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_TRUE;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
    vulkan_graphics_pipeline_create_info.pipeline_layout = p_vulkan_camera_info->pipeline_layout;

    switch (h_entity_graphics_data_ptr->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    p_vulkan_camera_info->graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);
    TG_MEMORY_STACK_FREE(vertex_input_attribute_descriptions_size);

    VkDescriptorBufferInfo model_descriptor_buffer_info = { 0 };
    model_descriptor_buffer_info.buffer = h_entity_graphics_data_ptr->model_ubo.buffer;
    model_descriptor_buffer_info.offset = 0;
    model_descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo view_projection_descriptor_buffer_info = { 0 };

    switch (h_entity_graphics_data_ptr->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        view_projection_descriptor_buffer_info.buffer = h_camera->view_projection_ubo.buffer;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        view_projection_descriptor_buffer_info.buffer = h_camera->view_projection_ubo.buffer;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    view_projection_descriptor_buffer_info.offset = 0;
    view_projection_descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };

    p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    p_write_descriptor_sets[0].pNext = TG_NULL;
    p_write_descriptor_sets[0].dstSet = p_vulkan_camera_info->descriptor.descriptor_set;
    p_write_descriptor_sets[0].dstBinding = 0;
    p_write_descriptor_sets[0].dstArrayElement = 0;
    p_write_descriptor_sets[0].descriptorCount = 1;
    p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_write_descriptor_sets[0].pImageInfo = TG_NULL;
    p_write_descriptor_sets[0].pBufferInfo = &model_descriptor_buffer_info;
    p_write_descriptor_sets[0].pTexelBufferView = TG_NULL;

    p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    p_write_descriptor_sets[1].pNext = TG_NULL;
    p_write_descriptor_sets[1].dstSet = p_vulkan_camera_info->descriptor.descriptor_set;
    p_write_descriptor_sets[1].dstBinding = 1;
    p_write_descriptor_sets[1].dstArrayElement = 0;
    p_write_descriptor_sets[1].descriptorCount = 1;
    p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_write_descriptor_sets[1].pImageInfo = TG_NULL;
    p_write_descriptor_sets[1].pBufferInfo = &view_projection_descriptor_buffer_info;
    p_write_descriptor_sets[1].pTexelBufferView = TG_NULL;

    tg_vulkan_descriptor_sets_update(2, p_write_descriptor_sets);
    tg_vulkan_command_buffers_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, TG_VULKAN_MAX_LOD_COUNT, p_vulkan_camera_info->p_command_buffers);

    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.pNext = TG_NULL;

    switch (h_entity_graphics_data_ptr->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    command_buffer_inheritance_info.subpass = 0;

    switch (h_entity_graphics_data_ptr->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_deferred_renderer->geometry_pass.framebuffer;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_forward_renderer->shading_pass.framebuffer;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
    command_buffer_inheritance_info.queryFlags = 0;
    command_buffer_inheritance_info.pipelineStatistics = 0;

    for (u32 i = 0; i < h_entity_graphics_data_ptr->lod_count; i++)
    {
        tg_vulkan_command_buffer_begin(p_vulkan_camera_info->p_command_buffers[i], VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &command_buffer_inheritance_info);
        {
            vkCmdBindPipeline(p_vulkan_camera_info->p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, p_vulkan_camera_info->graphics_pipeline);

            const VkDeviceSize vertex_buffer_offset = 0;
            vkCmdBindVertexBuffers(p_vulkan_camera_info->p_command_buffers[i], 0, 1, &h_entity_graphics_data_ptr->ph_lod_meshes[i]->vbo.buffer, &vertex_buffer_offset);
            if (h_entity_graphics_data_ptr->ph_lod_meshes[i]->ibo.size != 0)
            {
                vkCmdBindIndexBuffer(p_vulkan_camera_info->p_command_buffers[i], h_entity_graphics_data_ptr->ph_lod_meshes[i]->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
            }

            const u32 descriptor_set_count = has_custom_descriptor_set ? 2 : 1;
            VkDescriptorSet p_descriptor_sets[2] = { 0 };
            {
                p_descriptor_sets[0] = p_vulkan_camera_info->descriptor.descriptor_set;
                if (has_custom_descriptor_set)
                {
                    p_descriptor_sets[1] = h_entity_graphics_data_ptr->h_material->descriptor.descriptor_set;
                }
            }
            vkCmdBindDescriptorSets(p_vulkan_camera_info->p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, p_vulkan_camera_info->pipeline_layout, 0, descriptor_set_count, p_descriptor_sets, 0, TG_NULL);

            if (h_entity_graphics_data_ptr->ph_lod_meshes[i]->index_count != 0)
            {
                vkCmdDrawIndexed(p_vulkan_camera_info->p_command_buffers[i], (u32)(h_entity_graphics_data_ptr->ph_lod_meshes[i]->index_count), 1, 0, 0, 0); // TODO: u16
            }
            else
            {
                vkCmdDraw(p_vulkan_camera_info->p_command_buffers[i], (u32)(h_entity_graphics_data_ptr->ph_lod_meshes[i]->vertex_count), 1, 0, 0);
            }
        }
        VK_CALL(vkEndCommandBuffer(p_vulkan_camera_info->p_command_buffers[i]));
    }
}



void tg_camera_begin(tg_camera_h h_camera)
{
    h_camera->captured_entity_count = 0;
}

void tg_camera_capture(tg_camera_h h_camera, tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr)
{
    TG_ASSERT(h_camera && h_entity_graphics_data_ptr);

    tg_vulkan_camera_info* p_vulkan_camera_info = TG_NULL;
    for (u32 i = 0; i < h_entity_graphics_data_ptr->camera_info_count; i++)
    {
        if (h_entity_graphics_data_ptr->p_camera_infos[i].h_camera == h_camera)
        {
            p_vulkan_camera_info = &h_entity_graphics_data_ptr->p_camera_infos[i];
            break;
        }
    }
    
    TG_ASSERT(p_vulkan_camera_info || h_entity_graphics_data_ptr->camera_info_count < TG_VULKAN_MAX_CAMERAS_PER_ENTITY);
    
    if (!p_vulkan_camera_info)
    {
        tg_camera_internal_register_entity(h_camera, p_vulkan_camera_info, h_entity_graphics_data_ptr);
    }

    TG_ASSERT(h_camera->captured_entity_count < TG_VULKAN_MAX_ENTITIES_PER_CAMERA);

    h_camera->ph_captured_entities[h_camera->captured_entity_count++] = h_entity_graphics_data_ptr;
}

// TODO: this should obviously not stay this way!
void tgi_capture_transvoxel_terrain(tg_camera_h h_camera, tg_transvoxel_node* p_node)
{
    if (p_node->pp_children[0] == TG_NULL)
    {
        if (p_node->node.entity.h_graphics_data_ptr)
        {
            tg_camera_capture(h_camera, p_node->node.entity.h_graphics_data_ptr);
        }
        for (u8 i = 0; i < 6; i++)
        {
            if (p_node->node.p_transition_entities[i].h_graphics_data_ptr)
            {
                tg_camera_capture(h_camera, p_node->node.p_transition_entities[i].h_graphics_data_ptr);
            }
        }
    }
    else
    {
        for (u8 i = 0; i < 8; i++)
        {
            tgi_capture_transvoxel_terrain(h_camera, p_node->pp_children[i]);
        }
    }
}

void tg_camera_capture_transvoxel_terrain(tg_camera_h h_camera, tg_transvoxel_terrain_h h_terrain)
{
    TG_ASSERT(h_camera && h_terrain);

    tgi_capture_transvoxel_terrain(h_camera, &h_terrain->octree.root);
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

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, h_camera->render_target.fence));
}

tg_camera_h tg_camera_create_orthographic(v3 position, f32 pitch, f32 yaw, f32 roll, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(left != right && bottom != top && far != near);

	tg_camera_h h_camera = TG_MEMORY_ALLOC(sizeof(*h_camera));
    tg_camera_internal_init(h_camera);
	tg_camera_set_view(h_camera, position, pitch, yaw, roll);
	tg_camera_set_orthographic_projection(h_camera, left, right, bottom, top, far, near);
	return h_camera;
}

tg_camera_h tg_camera_create_perspective(v3 position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(near < 0 && far < 0 && near > far);

	tg_camera_h h_camera = TG_MEMORY_ALLOC(sizeof(*h_camera));
    tg_camera_internal_init(h_camera);
	tg_camera_set_view(h_camera, position, pitch, yaw, roll);
	tg_camera_set_perspective_projection(h_camera, fov_y, near, far);
	return h_camera;
}

void tg_camera_destroy(tg_camera_h h_camera)
{
	TG_ASSERT(h_camera);

    tg_camera_internal_destroy_clear_pass(h_camera);
    tg_camera_internal_destroy_present_pass(h_camera);
    tg_camera_internal_destroy_capture_pass(h_camera);
    tg_vulkan_render_target_destroy(&h_camera->render_target);
    tg_vulkan_buffer_destroy(&h_camera->view_projection_ubo);
	TG_MEMORY_FREE(h_camera);
}

void tg_camera_end(tg_camera_h h_camera)
{
    tg_deferred_renderer_begin(h_camera->capture_pass.h_deferred_renderer);
    for (u32 e = 0; e < h_camera->captured_entity_count; e++)
    {
        tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr = h_camera->ph_captured_entities[e];
        if (h_entity_graphics_data_ptr->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
        {
            for (u32 c = 0; c < h_entity_graphics_data_ptr->camera_info_count; c++)
            {
                tg_vulkan_camera_info* p_vulkan_camera_info = &h_entity_graphics_data_ptr->p_camera_infos[c];
                if (p_vulkan_camera_info->h_camera == h_camera)
                {
                    if (h_entity_graphics_data_ptr->lod_count == 1)
                    {
                        tg_deferred_renderer_execute(h_camera->capture_pass.h_deferred_renderer, p_vulkan_camera_info->p_command_buffers[0]);
                    }
                    else
                    {
                        const tg_bounds* p_bounds = &h_entity_graphics_data_ptr->ph_lod_meshes[0]->bounds;

                        v4 p_points[8] = { 0 };
                        p_points[0] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                        p_points[1] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                        p_points[2] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                        p_points[3] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                        p_points[4] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                        p_points[5] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                        p_points[6] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                        p_points[7] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->max.z, 1.0f };

                        v4 p_homogenous_points_clip_space[8] = { 0 };
                        v3 p_cartesian_points_clip_space[8] = { 0 };
                        for (u8 i = 0; i < 8; i++)
                        {
                            p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(h_entity_graphics_data_ptr), p_points[i]);
                            p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_VIEW(h_camera), p_homogenous_points_clip_space[i]);
                            p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_PROJ(h_camera), p_homogenous_points_clip_space[i]);// TODO: SIMD
                            p_cartesian_points_clip_space[i] = tgm_v4_to_v3(p_homogenous_points_clip_space[i]);
                        }

                        v3 min = p_cartesian_points_clip_space[0];
                        v3 max = p_cartesian_points_clip_space[0];
                        for (u8 i = 1; i < 8; i++)
                        {
                            min = tgm_v3_min(min, p_cartesian_points_clip_space[i]);
                            max = tgm_v3_max(max, p_cartesian_points_clip_space[i]);
                        }

                        const b32 cull = min.x >= 1.0f || min.y >= 1.0f || max.x <= -1.0f || max.y <= -1.0f || min.z >= 1.0f || max.z <= 0.0f;
                        if (!cull)
                        {
                            const v3 clamp_min = { -1.0f, -1.0f, 0.0f };
                            const v3 clamp_max = { 1.0f,  1.0f, 0.0f };
                            const v3 clamped_min = tgm_v3_clamp(min, clamp_min, clamp_max);
                            const v3 clamped_max = tgm_v3_clamp(max, clamp_min, clamp_max);
                            const f32 area = (clamped_max.x - clamped_min.x) * (clamped_max.y - clamped_min.y);

                            if (area >= 0.9f)
                            {
                                tg_deferred_renderer_execute(h_camera->capture_pass.h_deferred_renderer, p_vulkan_camera_info->p_command_buffers[2]);
                            }
                            else
                            {
                                tg_deferred_renderer_execute(h_camera->capture_pass.h_deferred_renderer, p_vulkan_camera_info->p_command_buffers[3]);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    tg_deferred_renderer_end(h_camera->capture_pass.h_deferred_renderer);

    tg_forward_renderer_begin(h_camera->capture_pass.h_forward_renderer);
    for (u32 i = 0; i < h_camera->captured_entity_count; i++)
    {
        if (h_camera->ph_captured_entities[i]->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD)
        {
            tg_forward_renderer_draw(h_camera->capture_pass.h_forward_renderer, h_camera->ph_captured_entities[i]);
        }
    }
    tg_forward_renderer_end(h_camera->capture_pass.h_forward_renderer);
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

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &draw_submit_info, h_camera->render_target.fence));

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &h_camera->present_pass.semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
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
