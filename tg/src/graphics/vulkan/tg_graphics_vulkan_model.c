#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

tg_model_h tg_graphics_model_create(tg_mesh_h mesh_h, tg_material_h material_h)
{
	TG_ASSERT(mesh_h && material_h);

	tg_model_h model_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*model_h));
    model_h->mesh = mesh_h;
    model_h->material = material_h;

    VkCommandPoolCreateInfo command_pool_create_info = { 0 };
    {
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = TG_NULL;
        command_pool_create_info.flags = 0;
        command_pool_create_info.queueFamilyIndex = graphics_queue.index;
    }
    VK_CALL(vkCreateCommandPool(device, &command_pool_create_info, TG_NULL, &model_h->command_pool));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = TG_NULL;
        command_buffer_allocate_info.commandPool = model_h->command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &model_h->command_buffer));

    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = model_h->material->ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(tg_uniform_buffer_object);
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = TG_NULL;
        write_descriptor_set.dstSet = model_h->material->descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_set.pImageInfo = TG_NULL;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = TG_NULL;
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = TG_NULL;
    }
    VK_CALL(vkBeginCommandBuffer(model_h->command_buffer, &command_buffer_begin_info));
    vkCmdBindPipeline(model_h->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model_h->material->pipeline);

    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(model_h->command_buffer, 0, 1, &model_h->mesh->vbo.buffer, &vertex_buffer_offset);
    if (model_h->mesh->ibo.index_count != 0)
    {
        vkCmdBindIndexBuffer(model_h->command_buffer, model_h->mesh->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    }

    const u32 dynamic_offset = 0;
    vkCmdBindDescriptorSets(model_h->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model_h->material->pipeline_layout, 0, 1, &model_h->material->descriptor_set, 1, &dynamic_offset);

    VkClearValue clear_values[2] = { 0 };
    {
        clear_values[0].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 1.0f };
        clear_values[0].depthStencil = (VkClearDepthStencilValue){ 0.0f, 0 };
        clear_values[1].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 0.0f };
        clear_values[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };
    }
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    {
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        tg_graphics_vulkan_renderer_3d_get_geometry_render_pass(&render_pass_begin_info.renderPass);
        tg_graphics_vulkan_renderer_3d_get_geometry_framebuffer(&render_pass_begin_info.framebuffer);
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values; // TODO: TG_NULL?
    }
    vkCmdBeginRenderPass(model_h->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    if (model_h->mesh->ibo.index_count != 0)
    {
        vkCmdDrawIndexed(model_h->command_buffer, model_h->mesh->ibo.index_count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(model_h->command_buffer, model_h->mesh->vbo.vertex_count, 1, 0, 0);
    }
    vkCmdEndRenderPass(model_h->command_buffer);
    VK_CALL(vkEndCommandBuffer(model_h->command_buffer));

    return model_h;
}

void tg_graphics_model_destroy(tg_model_h model_h)
{
	TG_ASSERT(model_h);

    vkDestroyCommandPool(device, model_h->command_pool, TG_NULL);
	TG_MEMORY_ALLOCATOR_FREE(model_h);
}

#endif
