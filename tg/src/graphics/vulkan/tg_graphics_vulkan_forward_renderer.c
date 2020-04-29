#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



typedef struct tg_forward_renderer
{
    tg_color_image               color_attachment;
    tg_depth_image               depth_attachment;

    VkFence                      fence;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    tg_vulkan_descriptor         descriptor;

    VkShaderModule               vertex_shader_h;
    VkShaderModule               fragment_shader_h;
    VkPipelineLayout             pipeline_layout;
    VkPipeline                   graphics_pipeline;

    VkCommandBuffer              command_buffer;
} tg_forward_renderer;



void tg_forward_renderer_begin(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);
}

tg_forward_renderer_h tg_forward_renderer_create(const tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

    tg_forward_renderer_h forward_renderer_h = TG_MEMORY_ALLOC(sizeof(*forward_renderer_h));

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    {
        vulkan_color_image_create_info.width = swapchain_extent.width;
        vulkan_color_image_create_info.height = swapchain_extent.height;
        vulkan_color_image_create_info.mip_levels = 1;
        vulkan_color_image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    forward_renderer_h->color_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);
    
    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    {
        vulkan_depth_image_create_info.width = swapchain_extent.width;
        vulkan_depth_image_create_info.height = swapchain_extent.height;
        vulkan_depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
        vulkan_depth_image_create_info.min_filter = VK_FILTER_LINEAR;
        vulkan_depth_image_create_info.mag_filter = VK_FILTER_LINEAR;
        vulkan_depth_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_depth_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_depth_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    forward_renderer_h->depth_attachment = tg_vulkan_depth_image_create(&vulkan_depth_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &forward_renderer_h->color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, &forward_renderer_h->depth_attachment, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

    forward_renderer_h->fence = tg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);

    VkAttachmentDescription p_attachment_descriptions[2] = { 0 };
    {
        p_attachment_descriptions[0].flags = 0;
        p_attachment_descriptions[0].format = VK_FORMAT_R8G8B8A8_SRGB;
        p_attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        p_attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        p_attachment_descriptions[1].flags = 0;
        p_attachment_descriptions[1].format = VK_FORMAT_D32_SFLOAT;
        p_attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        p_attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    forward_renderer_h->render_pass = tg_vulkan_render_pass_create(2, p_attachment_descriptions, 0, TG_NULL, 0, TG_NULL);

    VkImageView p_image_views[2] = { forward_renderer_h->color_attachment.image_view, forward_renderer_h->depth_attachment.image_view };
    forward_renderer_h->framebuffer = tg_vulkan_framebuffer_create(forward_renderer_h->render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);

    //tg_vulkan_descriptor_create()
    return forward_renderer_h;
}

void tg_forward_renderer_destroy(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);

    TG_MEMORY_FREE(forward_renderer_h);
}

void tg_forward_renderer_draw(tg_forward_renderer_h forward_renderer_h, tg_entity_h entity_h)
{
	TG_ASSERT(forward_renderer_h && entity_h);
}

void tg_forward_renderer_end(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);
}

void tg_forward_renderer_on_window_resize(tg_forward_renderer_h forward_renderer_h, u32 width, u32 height)
{
	TG_ASSERT(forward_renderer_h && width && height);
}

void tg_forward_renderer_present(tg_forward_renderer_h forward_renderer_h)
{
	TG_ASSERT(forward_renderer_h);
}


#endif
