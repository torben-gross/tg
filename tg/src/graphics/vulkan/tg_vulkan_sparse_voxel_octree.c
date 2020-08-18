#include "graphics/tg_sparse_voxel_octree.h"



void tg_voxelizer_create(tg_voxelizer* p_voxelizer)
{
    TG_ASSERT(p_voxelizer);

    p_voxelizer->fence = tg_vulkan_fence_create(0);

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = TG_NULL;
    subpass_description.colorAttachmentCount = 0;
    subpass_description.pColorAttachments = TG_NULL;
    subpass_description.pResolveAttachments = 0;
    subpass_description.pDepthStencilAttachment = TG_NULL;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = TG_NULL;

    p_voxelizer->render_pass = tg_vulkan_render_pass_create(0, TG_NULL, 1, &subpass_description, 0, TG_NULL);
    p_voxelizer->framebuffer = tg_vulkan_framebuffer_create(p_voxelizer->render_pass, 0, TG_NULL, TG_SVO_DIMS, TG_SVO_DIMS);

    const tg_vulkan_shader* pp_vulkan_shaders[2] = {
        &tg_vertex_shader_get("shaders/voxelize.vert")->vulkan_shader,
        &tg_fragment_shader_get("shaders/voxelize.frag")->vulkan_shader
    };

    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };
    tg_vulkan_pipeline_shader_stage_create_infos_create(pp_vulkan_shaders[0], pp_vulkan_shaders[1], p_pipeline_shader_stage_create_infos);
    const VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = tg_vulkan_pipeline_input_assembly_state_create_info_create();
    const VkViewport viewport = tg_vulkan_viewport_create(TG_SVO_DIMS, TG_SVO_DIMS);
    const VkRect2D scissors = { (VkOffset2D) { 0, 0 }, (VkExtent2D) { TG_SVO_DIMS, TG_SVO_DIMS } };
    const VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = tg_vulkan_pipeline_viewport_state_create_info_create(&viewport, &scissors);

    const VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = tg_vulkan_pipeline_rasterization_state_create_info_create(TG_NULL, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
    const VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = tg_vulkan_pipeline_multisample_state_create_info_create(1, VK_FALSE, 0.0f);

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = 12;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_description = { 0 };
    vertex_input_attribute_description.location = 0;
    vertex_input_attribute_description.binding = 0;
    vertex_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description.offset = 0;

    const VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = tg_vulkan_pipeline_vertex_input_state_create_info_create(1, &vertex_input_binding_description, 1, &vertex_input_attribute_description);

    p_voxelizer->descriptor_set_layout = tg_vulkan_descriptor_set_layout_create(2, pp_vulkan_shaders);
    p_voxelizer->graphics_pipeline_layout = tg_vulkan_pipeline_layout_create(p_voxelizer->descriptor_set_layout.descriptor_set_layout, 0, TG_NULL);

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = TG_NULL;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = TG_NULL;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = TG_NULL;
    graphics_pipeline_create_info.pColorBlendState = TG_NULL;
    graphics_pipeline_create_info.pDynamicState = TG_NULL;
    graphics_pipeline_create_info.layout = p_voxelizer->graphics_pipeline_layout;
    graphics_pipeline_create_info.renderPass = p_voxelizer->render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = -1;

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, TG_NULL, &p_voxelizer->graphics_pipeline);

    p_voxelizer->view_projection_ubo = tg_vulkan_buffer_create(3 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    p_voxelizer->image_3d = tg_vulkan_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R8_UNORM, TG_NULL);
    tg_vulkan_command_buffer_begin(global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_3d_layout(
        global_graphics_command_buffer,
        &p_voxelizer->image_3d,
        0,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tg_vulkan_command_buffer_end_and_submit(global_graphics_command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);

    p_voxelizer->voxel_buffer = tg_vulkan_buffer_create(TG_SVO_DIMS3 * sizeof(u8), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    p_voxelizer->command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void tg_voxelizer_begin(tg_voxelizer* p_voxelizer, v3i index_3d)
{
    TG_ASSERT(p_voxelizer);

    p_voxelizer->descriptor_set_count = 0;

    tg_vulkan_command_buffer_begin(p_voxelizer->command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_3d_layout(
        p_voxelizer->command_buffer,
        &p_voxelizer->image_3d,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_color_image_3d(p_voxelizer->command_buffer, &p_voxelizer->image_3d);
    tg_vulkan_command_buffer_cmd_transition_color_image_3d_layout(
        p_voxelizer->command_buffer,
        &p_voxelizer->image_3d,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tg_vulkan_command_buffer_end_and_submit(p_voxelizer->command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);

    for (u32 i = 0; i < TG_SVO_DIMS3; i++)
    {
        ((u8*)p_voxelizer->voxel_buffer.memory.p_mapped_device_memory)[i] = 0;
    }
    tg_vulkan_buffer_flush_mapped_memory(&p_voxelizer->voxel_buffer);

    tg_vulkan_command_buffer_begin(p_voxelizer->command_buffer, 0, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(p_voxelizer->command_buffer, p_voxelizer->render_pass, &p_voxelizer->framebuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void tg_voxelizer_exec(tg_voxelizer* p_voxelizer, tg_render_command_h h_render_command)
{
    TG_ASSERT(p_voxelizer && h_render_command);
    TG_ASSERT(p_voxelizer->descriptor_set_count < TG_MAX_RENDER_COMMANDS);

    if (p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count].descriptor_pool == VK_NULL_HANDLE)
    {
        p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count] = tg_vulkan_descriptor_set_create(&p_voxelizer->descriptor_set_layout);
    }
    tg_vulkan_descriptor_set* p_vulkan_descriptor_set = &p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count++];

    tg_vulkan_descriptor_set_update_uniform_buffer(p_vulkan_descriptor_set->descriptor_set, h_render_command->model_ubo.buffer, 0);
    tg_vulkan_descriptor_set_update_uniform_buffer(p_vulkan_descriptor_set->descriptor_set, p_voxelizer->view_projection_ubo.buffer, 1);
    tg_vulkan_descriptor_set_update_image_3d(p_vulkan_descriptor_set->descriptor_set, &p_voxelizer->image_3d, 2);

    vkCmdBindPipeline(p_voxelizer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_voxelizer->graphics_pipeline);
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(p_voxelizer->command_buffer, 0, 1, &h_render_command->p_mesh->positions_buffer.buffer, &offset);
    //vkCmdBindVertexBuffers(p_voxelizer->command_buffer, 1, 1, &h_render_command->p_mesh->normals_buffer.buffer, &offset); // TODO: normals
    vkCmdBindDescriptorSets(p_voxelizer->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_voxelizer->graphics_pipeline_layout, 0, 1 /*for now? can this be 0?*/, &p_vulkan_descriptor_set->descriptor_set, 0, TG_NULL);
    vkCmdDraw(p_voxelizer->command_buffer, h_render_command->p_mesh->position_count, 1, 0, 0);
}

void tg_voxelizer_end(tg_voxelizer* p_voxelizer, tg_voxel* p_voxels)
{
    TG_ASSERT(p_voxelizer);

    vkCmdEndRenderPass(p_voxelizer->command_buffer);
    vkEndCommandBuffer(p_voxelizer->command_buffer);
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[0] = tgm_m4_translate(tgm_v3_neg((v3) { 108.0f + TG_SVO_DIMS / 2.0f, 138.0f + TG_SVO_DIMS / 2.0f, 116.0f + TG_SVO_DIMS / 2.0f }));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(0.0f, 0.0f, 0.0f));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[2] = tgm_m4_orthographic(-TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f);
    tg_vulkan_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_voxelizer->command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tg_vulkan_fence_wait(p_voxelizer->fence);
    tg_vulkan_fence_reset(p_voxelizer->fence);

    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[0] = tgm_m4_translate(tgm_v3_neg((v3) { 108.0f + TG_SVO_DIMS / 2.0f, 138.0f + TG_SVO_DIMS / 2.0f, 116.0f + TG_SVO_DIMS / 2.0f }));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(0.0f, TG_TO_RADIANS(90.0f), 0.0f));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[2] = tgm_m4_orthographic(-TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f);
    tg_vulkan_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tg_vulkan_fence_wait(p_voxelizer->fence);
    tg_vulkan_fence_reset(p_voxelizer->fence);

    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[0] = tgm_m4_translate(tgm_v3_neg((v3) { 108.0f + TG_SVO_DIMS / 2.0f, 138.0f + TG_SVO_DIMS / 2.0f, 116.0f + TG_SVO_DIMS / 2.0f }));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(TG_TO_RADIANS(90.0f), 0.0f, 0.0f));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[2] = tgm_m4_orthographic(-TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f, -TG_SVO_DIMS / 2.0f, TG_SVO_DIMS / 2.0f);
    tg_vulkan_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tg_vulkan_fence_wait(p_voxelizer->fence);
    tg_vulkan_fence_reset(p_voxelizer->fence);

    tg_vulkan_command_buffer_begin(global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_3d_layout(
        global_graphics_command_buffer,
        &p_voxelizer->image_3d,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_copy_color_image_3d_to_buffer(global_graphics_command_buffer, &p_voxelizer->image_3d, p_voxelizer->voxel_buffer.buffer);
    tg_vulkan_command_buffer_cmd_transition_color_image_3d_layout(
        global_graphics_command_buffer,
        &p_voxelizer->image_3d,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tg_vulkan_command_buffer_end_and_submit(global_graphics_command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);

    tg_vulkan_buffer_flush_mapped_memory(&p_voxelizer->voxel_buffer);
    for (u32 i = 0; i < TG_SVO_DIMS3; i++)
    {
        p_voxels[i].albedo = ((u8*)p_voxelizer->voxel_buffer.memory.p_mapped_device_memory)[i];
    }
}

void tg_voxelizer_destroy(tg_voxelizer* p_voxelizer)
{
    TG_ASSERT(p_voxelizer);

    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, p_voxelizer->command_buffer);
    tg_vulkan_buffer_destroy(&p_voxelizer->voxel_buffer);
    tg_vulkan_color_image_3d_destroy(&p_voxelizer->image_3d);
    tg_vulkan_buffer_destroy(&p_voxelizer->view_projection_ubo);
    vkDestroyPipeline(device, p_voxelizer->graphics_pipeline, TG_NULL);
    vkDestroyPipelineLayout(device, p_voxelizer->graphics_pipeline_layout, TG_NULL);
    for (u32 i = 0; i < TG_MAX_RENDER_COMMANDS; i++)
    {
        if (p_voxelizer->p_descriptor_sets[i].descriptor_pool != VK_NULL_HANDLE)
        {
            tg_vulkan_descriptor_set_destroy(&p_voxelizer->p_descriptor_sets[i]);
        }
        else
        {
            break;
        }
    }
    tg_vulkan_descriptor_set_layout_destroy(&p_voxelizer->descriptor_set_layout);
    tg_vulkan_framebuffer_destroy(&p_voxelizer->framebuffer);
    tg_vulkan_render_pass_destroy(p_voxelizer->render_pass);
    tg_vulkan_fence_destroy(p_voxelizer->fence);
}
