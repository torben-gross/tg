#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/math/tg_math.h"
#include "tg/platform/tg_allocator.h"

VkDescriptorPool         renderer_2d_descriptor_pool;
VkDescriptorSet          renderer_2d_descriptor_set;
VkCommandBuffer          renderer_2d_command_buffer;
VkFence                  renderer_2d_rendering_finished_fence;
VkSemaphore              renderer_2d_rendering_finished_semaphore;
VkSemaphore              renderer_2d_image_acquired_semaphore;



#define QUAD_COUNT       10000

#define VBO_VERTEX_COUNT (4 * QUAD_COUNT)
#define VBO_SIZE         (QUAD_COUNT * sizeof(tg_vertex))
VkBuffer                 vbo;
VkDeviceMemory           vbo_memory;

#define IBO_INDEX_COUNT  (6 * QUAD_COUNT)
#define IBO_SIZE         (IBO_INDEX_COUNT * sizeof(ui16))
VkBuffer                 ibo;
VkDeviceMemory           ibo_memory;

VkBuffer                 renderer_2d_ubo;
VkDeviceMemory           renderer_2d_ubo_memory;



tg_vertex*               vertex_data = NULL;
ui32                     current_quad_index = 0;



void tg_graphics_renderer_2d_init_synchronization_primitives()
{
    VkFenceCreateInfo fence_create_info = { 0 };
    {
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VK_CALL(vkCreateFence(device, &fence_create_info, NULL, &renderer_2d_rendering_finished_fence));

    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    {
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = NULL;
        semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &renderer_2d_rendering_finished_semaphore));
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &renderer_2d_image_acquired_semaphore));
}
void tg_graphics_renderer_2d_init_descriptors()
{
    VkDescriptorPoolSize descriptor_pool_sizes[2] = { 0 };
    {
        descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_pool_sizes[0].descriptorCount = 1;
        descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_sizes[1].descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = NULL;
        descriptor_pool_create_info.flags = 0;
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(*descriptor_pool_sizes);
        descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &renderer_2d_descriptor_pool));



    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[2] = { 0 };
    {
        descriptor_set_layout_bindings[0].binding = 0;
        descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_layout_bindings[0].descriptorCount = 1;
        descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_layout_bindings[0].pImmutableSamplers = NULL;
        descriptor_set_layout_bindings[1].binding = 1;
        descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_bindings[1].descriptorCount = 1;
        descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_bindings[1].pImmutableSamplers = NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = sizeof(descriptor_set_layout_bindings) / sizeof(*descriptor_set_layout_bindings);
        descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout));



    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = renderer_2d_descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &renderer_2d_descriptor_set));



    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = renderer_2d_ubo;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(tg_uniform_buffer_object);
    }
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = image_h->sampler;
        descriptor_image_info.imageView = image_h->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_sets[2] = { 0 };
    {
        write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[0].pNext = NULL;
        write_descriptor_sets[0].dstSet = renderer_2d_descriptor_set;
        write_descriptor_sets[0].dstBinding = 0;
        write_descriptor_sets[0].dstArrayElement = 0;
        write_descriptor_sets[0].descriptorCount = 1;
        write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_sets[0].pImageInfo = NULL;
        write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;
        write_descriptor_sets[0].pTexelBufferView = NULL;
        write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[1].pNext = NULL;
        write_descriptor_sets[1].dstSet = renderer_2d_descriptor_set;
        write_descriptor_sets[1].dstBinding = 1;
        write_descriptor_sets[1].dstArrayElement = 0;
        write_descriptor_sets[1].descriptorCount = 1;
        write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_sets[1].pImageInfo = &descriptor_image_info;
        write_descriptor_sets[1].pBufferInfo = NULL;
        write_descriptor_sets[1].pTexelBufferView = NULL;
    }
    vkUpdateDescriptorSets(device, sizeof(write_descriptor_sets) / sizeof(*write_descriptor_sets), write_descriptor_sets, 0, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
}
void tg_graphics_renderer_2d_init_index_buffer()
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tg_graphics_vulkan_buffer_create(VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

    ui16* indices = tg_allocator_allocate(IBO_SIZE);
    for (ui32 i = 0; i < QUAD_COUNT; i++)
    {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;
    }

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, IBO_SIZE, 0, &data);
    memcpy(data, indices, IBO_SIZE);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_graphics_vulkan_buffer_create(VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ibo, &ibo_memory);
    tg_graphics_vulkan_buffer_copy(VBO_SIZE, &staging_buffer, &ibo);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
    tg_allocator_free(indices);
}
void tg_graphics_renderer_2d_init()
{
    // TODO: is vulkan initialized?

    tg_graphics_vulkan_buffer_create(sizeof(tg_uniform_buffer_object), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer_2d_ubo, &renderer_2d_ubo_memory);
    tg_graphics_renderer_2d_init_synchronization_primitives();
    tg_graphics_renderer_2d_init_descriptors();
    tg_graphics_renderer_2d_init_index_buffer();
    tg_graphics_vulkan_buffer_create(VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vbo, &vbo_memory);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
    }

    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &renderer_2d_command_buffer));
}
void tg_graphics_renderer_2d_begin()
{
    current_quad_index = 0;
    VK_CALL(vkMapMemory(device, vbo_memory, 0, VBO_SIZE, 0, &vertex_data));
}
void tg_graphics_renderer_2d_draw_sprite(f32 x, f32 y, f32 z, f32 w, f32 h, tg_image_h image)
{
    vertex_data[4 * current_quad_index + 0].position.x = x - w / 2.0f;
    vertex_data[4 * current_quad_index + 0].position.y = y - h / 2.0f;
    vertex_data[4 * current_quad_index + 0].position.z = z;
    vertex_data[4 * current_quad_index + 0].uv.x = 0.0f;
    vertex_data[4 * current_quad_index + 0].uv.y = 0.0f;

    vertex_data[4 * current_quad_index + 1].position.x = x + w / 2.0f;
    vertex_data[4 * current_quad_index + 1].position.y = y - h / 2.0f;
    vertex_data[4 * current_quad_index + 1].position.z = z;
    vertex_data[4 * current_quad_index + 1].uv.x = 1.0f;
    vertex_data[4 * current_quad_index + 1].uv.y = 0.0f;

    vertex_data[4 * current_quad_index + 2].position.x = x + w / 2.0f;
    vertex_data[4 * current_quad_index + 2].position.y = y + h / 2.0f;
    vertex_data[4 * current_quad_index + 2].position.z = z;
    vertex_data[4 * current_quad_index + 2].uv.x = 1.0f;
    vertex_data[4 * current_quad_index + 2].uv.y = 1.0f;

    vertex_data[4 * current_quad_index + 3].position.x = x - w / 2.0f;
    vertex_data[4 * current_quad_index + 3].position.y = y + h / 2.0f;
    vertex_data[4 * current_quad_index + 3].position.z = z;
    vertex_data[4 * current_quad_index + 3].uv.x = 0.0f;
    vertex_data[4 * current_quad_index + 3].uv.y = 1.0f;

    current_quad_index++;
}
void tg_graphics_renderer_2d_end()
{
    vkUnmapMemory(device, vbo_memory);

    ui32 current_image;
    VK_CALL(vkWaitForFences(device, 1, &renderer_2d_rendering_finished_fence, VK_TRUE, UINT64_MAX));
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, renderer_2d_image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
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
        render_pass_begin_info.pNext = NULL;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = framebuffers[current_image];
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values;
    }
    const VkDeviceSize vertex_buffer_offsets[1] = { 0 };
    const ui32 dynamic_offsets[1] = { 0 };

    VK_CALL(vkBeginCommandBuffer(renderer_2d_command_buffer, &command_buffer_begin_info));
    vkCmdBindPipeline(renderer_2d_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdBindVertexBuffers(renderer_2d_command_buffer, 0, sizeof(vertex_buffer_offsets) / sizeof(*vertex_buffer_offsets), &vbo, vertex_buffer_offsets);
    vkCmdBindIndexBuffer(renderer_2d_command_buffer, ibo, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(renderer_2d_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &renderer_2d_descriptor_set, 1, dynamic_offsets);
    vkCmdBeginRenderPass(renderer_2d_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(renderer_2d_command_buffer, 6 * current_quad_index, 1, 0, 0, 0);
    vkCmdEndRenderPass(renderer_2d_command_buffer);
    VK_CALL(vkEndCommandBuffer(renderer_2d_command_buffer));





    tgm_vec3f from = { -1.0f, 1.0f, 1.0f };
    tgm_vec3f to = { 0.0f, 0.0f, -2.0f };
    tgm_vec3f up = { 0.0f, 1.0f, 0.0f };
    const tgm_vec3f translation_vector = { 0.0f, 0.0f, -2.0f };
    const f32 fov_y = TGM_TO_DEGREES(70.0f);
    const f32 aspect = (f32)swapchain_extent.width / (f32)swapchain_extent.height;
    const f32 n = -0.1f;
    const f32 f = -1000.0f;
    tg_uniform_buffer_object uniform_buffer_object = { 0 };
    tgm_m4f_translate(&uniform_buffer_object.model, &translation_vector);
    tgm_m4f_look_at(&uniform_buffer_object.view, &from, &to, &up);
    tgm_m4f_perspective(&uniform_buffer_object.projection, fov_y, aspect, n, f);

    void* ubo_data;
    VK_CALL(vkMapMemory(device, renderer_2d_ubo_memory, 0, sizeof(tg_uniform_buffer_object), 0, &ubo_data));
    memcpy(ubo_data, &uniform_buffer_object, sizeof(tg_uniform_buffer_object));
    vkUnmapMemory(device, renderer_2d_ubo_memory);





    const VkPipelineStageFlags pipeline_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &renderer_2d_image_acquired_semaphore;
        submit_info.pWaitDstStageMask = pipeline_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &renderer_2d_command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &renderer_2d_rendering_finished_semaphore;
    }
    VK_CALL(vkResetFences(device, 1, &renderer_2d_rendering_finished_fence));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, renderer_2d_rendering_finished_fence));

    VkPresentInfoKHR present_info = { 0 };
    {
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &renderer_2d_rendering_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = NULL;
    }
    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
}
void tg_graphics_renderer_2d_shutdown()
{
    vkDeviceWaitIdle(device);

    vkDestroyBuffer(device, renderer_2d_ubo, NULL);
    vkFreeMemory(device, renderer_2d_ubo_memory, NULL);

    vkDestroyBuffer(device, ibo, NULL);
    vkFreeMemory(device, ibo_memory, NULL);
    vkDestroyBuffer(device, vbo, NULL);
    vkFreeMemory(device, vbo_memory, NULL);

    vkDestroyDescriptorPool(device, renderer_2d_descriptor_pool, NULL);

    vkDestroySemaphore(device, renderer_2d_image_acquired_semaphore, NULL);
    vkDestroySemaphore(device, renderer_2d_rendering_finished_semaphore, NULL);
    vkDestroyFence(device, renderer_2d_rendering_finished_fence, NULL);
}

#endif
