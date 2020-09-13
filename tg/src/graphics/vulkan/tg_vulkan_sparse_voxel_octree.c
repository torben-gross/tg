#include "graphics/tg_sparse_voxel_octree.h"



void tg_voxelizer_create(tg_voxelizer* p_voxelizer)
{
    TG_ASSERT(p_voxelizer);

    p_voxelizer->fence = tgvk_fence_create(0);

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

    p_voxelizer->render_pass = tgvk_render_pass_create(0, TG_NULL, 1, &subpass_description, 0, TG_NULL);
    p_voxelizer->framebuffer = tgvk_framebuffer_create(p_voxelizer->render_pass, 0, TG_NULL, TG_SVO_DIMS, TG_SVO_DIMS);

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/voxelize.vert")->shader;
    graphics_pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/voxelize.frag")->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    graphics_pipeline_create_info.blend_enable = VK_FALSE;
    graphics_pipeline_create_info.render_pass = p_voxelizer->render_pass;
    graphics_pipeline_create_info.viewport_size = (v2){ (f32)TG_SVO_DIMS, (f32)TG_SVO_DIMS };
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    VkVertexInputBindingDescription p_vertex_input_binding_descriptions[2] = { 0 };

    p_vertex_input_binding_descriptions[0].binding = 0;
    p_vertex_input_binding_descriptions[0].stride = 12;
    p_vertex_input_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    p_vertex_input_binding_descriptions[1].binding = 1;
    p_vertex_input_binding_descriptions[1].stride = 12;
    p_vertex_input_binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };
    p_vertex_input_attribute_descriptions[0].location = 0;
    p_vertex_input_attribute_descriptions[0].binding = 0;
    p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    p_vertex_input_attribute_descriptions[0].offset = 0;
    p_vertex_input_attribute_descriptions[1].location = 1;
    p_vertex_input_attribute_descriptions[1].binding = 1;
    p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    p_vertex_input_attribute_descriptions[1].offset = 0;

    p_voxelizer->pipeline = tgvk_pipeline_create_graphics2(&graphics_pipeline_create_info, 2, p_vertex_input_binding_descriptions, p_vertex_input_attribute_descriptions);

    p_voxelizer->descriptor_set_count = 0;
    for (u32 i = 0; i < TG_MAX_RENDER_COMMANDS; i++)
    {
        p_voxelizer->p_descriptor_sets[i] = (tgvk_descriptor_set){ 0 };
    }

    p_voxelizer->view_projection_ubo = tgvk_buffer_create(3 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    p_voxelizer->p_image_3ds[0] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[1] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[2] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[3] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[4] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[5] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[6] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);
    p_voxelizer->p_image_3ds[7] = tgvk_color_image_3d_create(TG_SVO_DIMS, TG_SVO_DIMS, TG_SVO_DIMS, VK_FORMAT_R32_UINT, TG_NULL);

    tgvk_command_buffer_begin(&global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_command_buffer_cmd_transition_color_image_3d_layout(&global_graphics_command_buffer, &p_voxelizer->p_image_3ds[i], 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    tgvk_command_buffer_end_and_submit(&global_graphics_command_buffer);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        const VkDeviceSize format_size = (VkDeviceSize)tg_color_image_format_size((tg_color_image_format)p_voxelizer->p_image_3ds[i].format);
        p_voxelizer->p_voxel_buffers[i] = tgvk_buffer_create(TG_SVO_DIMS3 * format_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    p_voxelizer->command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void tg_voxelizer_begin(tg_voxelizer* p_voxelizer)
{
    TG_ASSERT(p_voxelizer);

    p_voxelizer->descriptor_set_count = 0;

    tgvk_command_buffer_begin(&p_voxelizer->command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_command_buffer_cmd_transition_color_image_3d_layout(&p_voxelizer->command_buffer, &p_voxelizer->p_image_3ds[i], VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        tgvk_command_buffer_cmd_clear_color_image_3d(&p_voxelizer->command_buffer, &p_voxelizer->p_image_3ds[i]);
        tgvk_command_buffer_cmd_transition_color_image_3d_layout(&p_voxelizer->command_buffer, &p_voxelizer->p_image_3ds[i], VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    tgvk_command_buffer_end_and_submit(&p_voxelizer->command_buffer);

    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        u8* p_it = (u8*)p_voxelizer->p_voxel_buffers[i].memory.p_mapped_device_memory;
        const VkDeviceSize format_size = (VkDeviceSize)tg_color_image_format_size((tg_color_image_format)p_voxelizer->p_image_3ds[i].format);
        for (u32 j = 0; j < TG_SVO_DIMS3 * format_size; j++)
        {
            *p_it++ = 0;
        }
        tgvk_buffer_flush_mapped_memory(&p_voxelizer->p_voxel_buffers[i]);
    }

    tgvk_command_buffer_begin(&p_voxelizer->command_buffer, 0);
    tgvk_command_buffer_cmd_begin_render_pass(&p_voxelizer->command_buffer, p_voxelizer->render_pass, &p_voxelizer->framebuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void tg_voxelizer_exec(tg_voxelizer* p_voxelizer, tg_render_command_h h_render_command)
{
    TG_ASSERT(p_voxelizer && h_render_command && h_render_command->h_mesh->position_buffer.buffer && h_render_command->h_mesh->normal_buffer.buffer);
    TG_ASSERT(p_voxelizer->descriptor_set_count < TG_MAX_RENDER_COMMANDS);

    if (p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count].descriptor_pool == VK_NULL_HANDLE)
    {
        p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count] = tgvk_descriptor_set_create(&p_voxelizer->pipeline);
    }
    tgvk_descriptor_set* p_descriptor_set = &p_voxelizer->p_descriptor_sets[p_voxelizer->descriptor_set_count++];

    tgvk_descriptor_set_update_uniform_buffer(p_descriptor_set->descriptor_set, h_render_command->model_ubo.buffer, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_descriptor_set->descriptor_set, p_voxelizer->view_projection_ubo.buffer, 1);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_descriptor_set_update_image_3d(p_descriptor_set->descriptor_set, &p_voxelizer->p_image_3ds[i], 2 + i);
    }

    vkCmdBindPipeline(p_voxelizer->command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_voxelizer->pipeline.pipeline);
    const VkDeviceSize offset = 0;
    if (h_render_command->h_mesh->index_count)
    {
        vkCmdBindIndexBuffer(p_voxelizer->command_buffer.command_buffer, h_render_command->h_mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    }
    vkCmdBindVertexBuffers(p_voxelizer->command_buffer.command_buffer, 0, 1, &h_render_command->h_mesh->position_buffer.buffer, &offset);
    vkCmdBindVertexBuffers(p_voxelizer->command_buffer.command_buffer, 1, 1, &h_render_command->h_mesh->normal_buffer.buffer, &offset);
    vkCmdBindDescriptorSets(p_voxelizer->command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_voxelizer->pipeline.layout.pipeline_layout, 0, 1, &p_descriptor_set->descriptor_set, 0, TG_NULL);
    if (h_render_command->h_mesh->index_count)
    {
        vkCmdDrawIndexed(p_voxelizer->command_buffer.command_buffer, h_render_command->h_mesh->index_count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(p_voxelizer->command_buffer.command_buffer, h_render_command->h_mesh->vertex_count, 1, 0, 0);
    }
}

void tg_voxelizer_end(tg_voxelizer* p_voxelizer, v3i min_corner_index_3d, tg_voxel* p_voxels)
{
    TG_ASSERT(p_voxelizer);

    const f32 dh = (f32)(TG_SVO_DIMS / 2);
    const v3 camera_position = {
        (f32)(min_corner_index_3d.x * TG_SVO_DIMS) + dh,
        (f32)(min_corner_index_3d.y * TG_SVO_DIMS) + dh,
        (f32)(min_corner_index_3d.z * TG_SVO_DIMS) + dh
    };

    vkCmdEndRenderPass(p_voxelizer->command_buffer.command_buffer);
    vkEndCommandBuffer(p_voxelizer->command_buffer.command_buffer);

    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[0] = tgm_m4_translate(tgm_v3_neg(camera_position));
    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[2] = tgm_m4_orthographic(-dh, dh, -dh, dh, -dh, dh);



    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(0.0f, 0.0f, 0.0f));
    tgvk_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_voxelizer->command_buffer.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tgvk_fence_wait(p_voxelizer->fence);
    tgvk_fence_reset(p_voxelizer->fence);



    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(0.0f, TG_TO_RADIANS(90.0f), 0.0f));
    tgvk_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tgvk_fence_wait(p_voxelizer->fence);
    tgvk_fence_reset(p_voxelizer->fence);



    ((m4*)p_voxelizer->view_projection_ubo.memory.p_mapped_device_memory)[1] = tgm_m4_inverse(tgm_m4_euler(TG_TO_RADIANS(90.0f), 0.0f, 0.0f));
    tgvk_buffer_flush_mapped_memory(&p_voxelizer->view_projection_ubo);

    tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_voxelizer->fence);
    tgvk_fence_wait(p_voxelizer->fence);
    tgvk_fence_reset(p_voxelizer->fence);



    tgvk_command_buffer_begin(&global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_command_buffer_cmd_transition_color_image_3d_layout(&global_graphics_command_buffer, &p_voxelizer->p_image_3ds[i], VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        tgvk_command_buffer_cmd_copy_color_image_3d_to_buffer(&global_graphics_command_buffer, &p_voxelizer->p_image_3ds[i], p_voxelizer->p_voxel_buffers[i].buffer);
        tgvk_command_buffer_cmd_transition_color_image_3d_layout(&global_graphics_command_buffer, &p_voxelizer->p_image_3ds[i], VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    tgvk_command_buffer_end_and_submit(&global_graphics_command_buffer);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_buffer_flush_mapped_memory(&p_voxelizer->p_voxel_buffers[i]);
    }
    
    const u32* p_albedo_r_it  = (u32*)p_voxelizer->p_voxel_buffers[0].memory.p_mapped_device_memory;
    const u32* p_albedo_g_it  = (u32*)p_voxelizer->p_voxel_buffers[1].memory.p_mapped_device_memory;
    const u32* p_albedo_b_it  = (u32*)p_voxelizer->p_voxel_buffers[2].memory.p_mapped_device_memory;
    const u32* p_albedo_a_it  = (u32*)p_voxelizer->p_voxel_buffers[3].memory.p_mapped_device_memory;
    const u32* p_metallic_it  = (u32*)p_voxelizer->p_voxel_buffers[4].memory.p_mapped_device_memory;
    const u32* p_roughness_it = (u32*)p_voxelizer->p_voxel_buffers[5].memory.p_mapped_device_memory;
    const u32* p_ao_it        = (u32*)p_voxelizer->p_voxel_buffers[6].memory.p_mapped_device_memory;
    const u32* p_count_it     = (u32*)p_voxelizer->p_voxel_buffers[7].memory.p_mapped_device_memory;

    for (u32 i = 0; i < TG_SVO_DIMS3; i++)
    {
        const f32 denominator = tgm_f32_max(1.0f, 255.0f * (f32)*p_count_it++);

        p_voxels[i].albedo.x  = (f32)*p_albedo_r_it++  / denominator;
        p_voxels[i].albedo.y  = (f32)*p_albedo_g_it++  / denominator;
        p_voxels[i].albedo.z  = (f32)*p_albedo_b_it++  / denominator;
        p_voxels[i].albedo.w  = (f32)*p_albedo_a_it++  / denominator;
        p_voxels[i].metallic  = (f32)*p_metallic_it++  / denominator;
        p_voxels[i].roughness = (f32)*p_roughness_it++ / denominator;
        p_voxels[i].ao        = (f32)*p_ao_it++        / denominator;
    }
}

void tg_voxelizer_destroy(tg_voxelizer* p_voxelizer)
{
    TG_ASSERT(p_voxelizer);

    tgvk_command_buffer_destroy(&p_voxelizer->command_buffer);
    for (u32 i = 0; i < TG_SVO_ATTACHMENTS; i++)
    {
        tgvk_buffer_destroy(&p_voxelizer->p_voxel_buffers[i]);
        tgvk_color_image_3d_destroy(&p_voxelizer->p_image_3ds[i]);
    }
    tgvk_buffer_destroy(&p_voxelizer->view_projection_ubo);
    tgvk_pipeline_destroy(&p_voxelizer->pipeline);
    for (u32 i = 0; i < TG_MAX_RENDER_COMMANDS; i++)
    {
        if (p_voxelizer->p_descriptor_sets[i].descriptor_pool != VK_NULL_HANDLE)
        {
            tgvk_descriptor_set_destroy(&p_voxelizer->p_descriptor_sets[i]);
        }
        else
        {
            break;
        }
    }
    tgvk_framebuffer_destroy(&p_voxelizer->framebuffer);
    tgvk_render_pass_destroy(p_voxelizer->render_pass);
    tgvk_fence_destroy(p_voxelizer->fence);
}





void tg_svo_init(const tg_voxel* p_voxels)
{

}
