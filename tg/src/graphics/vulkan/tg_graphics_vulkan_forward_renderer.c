#include "graphics/vulkan/tg_graphics_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"



void tg_forward_renderer_internal_destroy_shading_pass(tg_forward_renderer_h forward_renderer_h)
{
    tg_vulkan_render_pass_destroy(forward_renderer_h->shading_pass.render_pass);
    tg_vulkan_framebuffer_destroy(forward_renderer_h->shading_pass.framebuffer);
    tg_vulkan_command_buffer_free(graphics_command_pool, forward_renderer_h->shading_pass.command_buffer);
}

void tg_forward_renderer_internal_init_shading_pass(tg_forward_renderer_h forward_renderer_h)
{

    VkAttachmentDescription p_attachment_descriptions[2] = { 0 };

    p_attachment_descriptions[0].flags = 0;
    p_attachment_descriptions[0].format = forward_renderer_h->camera_h->render_target.color_attachment.format;
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

    forward_renderer_h->shading_pass.render_pass = tg_vulkan_render_pass_create(2, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);
    const VkImageView p_image_views[2] = {
        forward_renderer_h->camera_h->render_target.color_attachment.image_view,
        forward_renderer_h->camera_h->render_target.depth_attachment.image_view
    };
    forward_renderer_h->shading_pass.framebuffer = tg_vulkan_framebuffer_create(forward_renderer_h->shading_pass.render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);
    forward_renderer_h->shading_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}



void tg_forward_renderer_begin(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    tg_vulkan_command_buffer_begin(forward_renderer_h->shading_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);

    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = TG_NULL;
    render_pass_begin_info.renderPass = forward_renderer_h->shading_pass.render_pass;
    render_pass_begin_info.framebuffer = forward_renderer_h->shading_pass.framebuffer;
    render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    render_pass_begin_info.renderArea.extent = swapchain_extent;
    render_pass_begin_info.clearValueCount = 0;
    render_pass_begin_info.pClearValues = TG_NULL;

    vkCmdBeginRenderPass(forward_renderer_h->shading_pass.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

tg_forward_renderer_h tg_forward_renderer_create(const tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

    tg_forward_renderer_h forward_renderer_h = TG_MEMORY_ALLOC(sizeof(*forward_renderer_h));
    forward_renderer_h->camera_h = camera_h;
    tg_forward_renderer_internal_init_shading_pass(forward_renderer_h);
    return forward_renderer_h;
}

void tg_forward_renderer_destroy(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    tg_forward_renderer_internal_destroy_shading_pass(forward_renderer_h);
    TG_MEMORY_FREE(forward_renderer_h);
}

void tg_forward_renderer_draw(tg_forward_renderer_h forward_renderer_h, tg_entity* p_entity, u32 entity_graphics_data_ptr_index)
{
    TG_ASSERT(forward_renderer_h && p_entity && p_entity->graphics_data_ptr_h->p_entity_scene_infos[entity_graphics_data_ptr_index].renderer_h == forward_renderer_h);

    vkCmdExecuteCommands(forward_renderer_h->shading_pass.command_buffer, 1, &p_entity->graphics_data_ptr_h->p_entity_scene_infos[entity_graphics_data_ptr_index].command_buffer);
}

void tg_forward_renderer_end(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    vkCmdEndRenderPass(forward_renderer_h->shading_pass.command_buffer);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.color_attachment,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.color_attachment_copy,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.depth_attachment,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.depth_attachment_copy,
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
    color_image_blit.srcOffsets[0].y = forward_renderer_h->camera_h->render_target.color_attachment.height;
    color_image_blit.srcOffsets[0].z = 0;
    color_image_blit.srcOffsets[1].x = forward_renderer_h->camera_h->render_target.color_attachment.width;
    color_image_blit.srcOffsets[1].y = 0;
    color_image_blit.srcOffsets[1].z = 1;
    color_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_blit.dstSubresource.mipLevel = 0;
    color_image_blit.dstSubresource.baseArrayLayer = 0;
    color_image_blit.dstSubresource.layerCount = 1;
    color_image_blit.dstOffsets[0].x = 0;
    color_image_blit.dstOffsets[0].y = 0;
    color_image_blit.dstOffsets[0].z = 0;
    color_image_blit.dstOffsets[1].x = forward_renderer_h->camera_h->render_target.color_attachment.width;
    color_image_blit.dstOffsets[1].y = forward_renderer_h->camera_h->render_target.color_attachment.height;
    color_image_blit.dstOffsets[1].z = 1;

    tg_vulkan_command_buffer_cmd_blit_color_image(forward_renderer_h->shading_pass.command_buffer, &forward_renderer_h->camera_h->render_target.color_attachment, &forward_renderer_h->camera_h->render_target.color_attachment_copy, &color_image_blit);

    VkImageBlit depth_image_blit = { 0 };
    depth_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_blit.srcSubresource.mipLevel = 0;
    depth_image_blit.srcSubresource.baseArrayLayer = 0;
    depth_image_blit.srcSubresource.layerCount = 1;
    depth_image_blit.srcOffsets[0].x = 0;
    depth_image_blit.srcOffsets[0].y = forward_renderer_h->camera_h->render_target.depth_attachment.height;
    depth_image_blit.srcOffsets[0].z = 0;
    depth_image_blit.srcOffsets[1].x = forward_renderer_h->camera_h->render_target.depth_attachment.width;
    depth_image_blit.srcOffsets[1].y = 0;
    depth_image_blit.srcOffsets[1].z = 1;
    depth_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_blit.dstSubresource.mipLevel = 0;
    depth_image_blit.dstSubresource.baseArrayLayer = 0;
    depth_image_blit.dstSubresource.layerCount = 1;
    depth_image_blit.dstOffsets[0].x = 0;
    depth_image_blit.dstOffsets[0].y = 0;
    depth_image_blit.dstOffsets[0].z = 0;
    depth_image_blit.dstOffsets[1].x = forward_renderer_h->camera_h->render_target.depth_attachment.width;
    depth_image_blit.dstOffsets[1].y = forward_renderer_h->camera_h->render_target.depth_attachment.height;
    depth_image_blit.dstOffsets[1].z = 1;

    tg_vulkan_command_buffer_cmd_blit_depth_image(forward_renderer_h->shading_pass.command_buffer, &forward_renderer_h->camera_h->render_target.depth_attachment, &forward_renderer_h->camera_h->render_target.depth_attachment_copy, &depth_image_blit);

    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.color_attachment,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.color_attachment_copy,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.depth_attachment,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        forward_renderer_h->shading_pass.command_buffer,
        &forward_renderer_h->camera_h->render_target.depth_attachment_copy,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    VK_CALL(vkEndCommandBuffer(forward_renderer_h->shading_pass.command_buffer));

    tg_vulkan_fence_wait(forward_renderer_h->camera_h->render_target.fence);
    tg_vulkan_fence_reset(forward_renderer_h->camera_h->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &forward_renderer_h->shading_pass.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, forward_renderer_h->camera_h->render_target.fence));
}

void tg_forward_renderer_on_window_resize(tg_forward_renderer_h forward_renderer_h, u32 width, u32 height)
{
	TG_ASSERT(forward_renderer_h && width && height);
}

#endif
