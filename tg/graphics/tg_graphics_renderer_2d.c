#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#define MAX_QUAD_COUNT 100

VkDescriptorSetLayout ds_layout;
VkDescriptorPool descr_pool;
VkCommandBuffer primary_command_buffers[SURFACE_IMAGE_COUNT];

#define VBO_VERTEX_COUNT (4 * MAX_QUAD_COUNT)
#define VBO_SIZE (MAX_QUAD_COUNT * sizeof(tg_vertex))
tg_vertex_buffer vbo;

#define IBO_INDEX_COUNT (6 * MAX_QUAD_COUNT)
#define IBO_SIZE (IBO_INDEX_COUNT * sizeof(ui16))
tg_index_buffer_h ibo;

void tg_graphics_renderer_2d_init()
{
    // TODO: is vulkan initialized?



    ui16 idcs[IBO_INDEX_COUNT] = { 0 };
    for (ui32 i = 0; i < MAX_QUAD_COUNT; i++)
    {
        idcs[6 * i + 0] = 4 * i + 0;
        idcs[6 * i + 1] = 4 * i + 1;
        idcs[6 * i + 2] = 4 * i + 2;
        idcs[6 * i + 3] = 4 * i + 2;
        idcs[6 * i + 4] = 4 * i + 3;
        idcs[6 * i + 5] = 4 * i + 0;
    }
    tg_graphics_index_buffer_create(IBO_INDEX_COUNT, idcs, &ibo);
    tg_graphics_vulkan_buffer_create(
        VBO_SIZE,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &vbo.buffer,
        &vbo.device_memory);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = SURFACE_IMAGE_COUNT;

    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, primary_command_buffers));

    images_in_flight[0] = in_flight_fences[0];
    images_in_flight[1] = in_flight_fences[1];
    images_in_flight[2] = in_flight_fences[0];
}



void* data = NULL;
ui32 quad_index;
void tg_graphics_renderer_2d_begin()
{
    quad_index = 0;
    VK_CALL(vkMapMemory(device, vbo.device_memory, 0, VBO_SIZE, 0, &data));
}



void tg_graphics_renderer_2d_draw_sprite(f32 x, f32 y, f32 w, f32 h, tg_image_h image)
{
    ((tg_vertex*)data)[4 * quad_index + 0].position.x = x - w / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 0].position.y = y - h / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 0].uv.x = 0.0f;
    ((tg_vertex*)data)[4 * quad_index + 0].uv.y = 0.0f;

    ((tg_vertex*)data)[4 * quad_index + 1].position.x = x + w / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 1].position.y = y - h / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 1].uv.x = 1.0f;
    ((tg_vertex*)data)[4 * quad_index + 1].uv.y = 0.0f;

    ((tg_vertex*)data)[4 * quad_index + 2].position.x = x + w / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 2].position.y = y + h / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 2].uv.x = 1.0f;
    ((tg_vertex*)data)[4 * quad_index + 2].uv.y = 1.0f;

    ((tg_vertex*)data)[4 * quad_index + 3].position.x = x - w / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 3].position.y = y + h / 2.0f;
    ((tg_vertex*)data)[4 * quad_index + 3].uv.x = 0.0f;
    ((tg_vertex*)data)[4 * quad_index + 3].uv.y = 1.0f;

    quad_index++;
}



void tg_graphics_renderer_2d_end()
{
    vkUnmapMemory(device, vbo.device_memory);



    VK_CALL(vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));

    ui32 next_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &next_image));
    images_in_flight[next_image] = in_flight_fences[current_frame];



    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = NULL;

    VkClearValue clear_values[2] = { 0 };
    clear_values[0].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 1.0f };
    clear_values[0].depthStencil = (VkClearDepthStencilValue){ 0.0f, 0 };
    clear_values[1].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 0.0f };
    clear_values[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = NULL;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = framebuffers[next_image];
    render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    render_pass_begin_info.renderArea.extent = swapchain_extent;
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    const VkDeviceSize vertex_buffer_offsets[1] = { 0 };
    const ui32 dynamic_offsets[1] = { 0 };

    VK_CALL(vkBeginCommandBuffer(primary_command_buffers[next_image], &command_buffer_begin_info));
    vkCmdBindPipeline(primary_command_buffers[next_image], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdBindVertexBuffers(primary_command_buffers[next_image], 0, sizeof(vertex_buffer_offsets) / sizeof(*vertex_buffer_offsets), &vbo.buffer, vertex_buffer_offsets);
    vkCmdBindIndexBuffer(primary_command_buffers[next_image], ibo->buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(primary_command_buffers[next_image], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[next_image], 1, dynamic_offsets);
    vkCmdBeginRenderPass(primary_command_buffers[next_image], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(primary_command_buffers[next_image], 6 * quad_index, 1, 0, 0, 0);
    vkCmdEndRenderPass(primary_command_buffers[next_image]);
    VK_CALL(vkEndCommandBuffer(primary_command_buffers[next_image]));



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

    void* data;
    VK_CALL(vkMapMemory(device, uniform_buffers_h[next_image]->device_memory, 0, sizeof(tg_uniform_buffer_object), 0, &data));
    memcpy(data, &uniform_buffer_object, sizeof(tg_uniform_buffer_object));
    vkUnmapMemory(device, uniform_buffers_h[next_image]->device_memory);

    const VkPipelineStageFlags msk[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[current_frame];
    submit_info.pWaitDstStageMask = msk;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &primary_command_buffers[next_image];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &rendering_finished_semaphores[current_frame];

    VK_CALL(vkResetFences(device, 1, in_flight_fences + current_frame));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, in_flight_fences[current_frame]));

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &rendering_finished_semaphores[current_frame];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &next_image;
    present_info.pResults = NULL;

    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
    current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;
}
void tg_graphics_renderer_2d_shutdown()
{
    vkDeviceWaitIdle(device);

    tg_graphics_index_buffer_destroy(ibo);
}

#endif
