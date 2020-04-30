#ifndef TG_GRAPHICS_VULKAN_FORWARD_RENDERER_H
#define TG_GRAPHICS_VULKAN_FORWARD_RENDERER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_forward_renderer
{
    struct
    {
        const tg_camera*             p_camera;

        tg_render_target             color_attachment;
        tg_depth_image               depth_attachment;

        tg_vulkan_buffer             view_projection_ubo;

        VkRenderPass                 render_pass;
        VkFramebuffer                framebuffer;

        VkCommandBuffer              command_buffer;
    } shading_pass;

    struct
    {
        tg_vulkan_buffer         vbo;
        tg_vulkan_buffer         ibo;

        VkSemaphore              image_acquired_semaphore;
        VkSemaphore              semaphore;

        VkRenderPass             render_pass;
        VkFramebuffer            framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];

        tg_vulkan_descriptor     descriptor;

        VkShaderModule           vertex_shader_h;
        VkShaderModule           fragment_shader_h;
        VkPipelineLayout         pipeline_layout;
        VkPipeline               graphics_pipeline;

        VkCommandBuffer          command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
    } present_pass;
} tg_forward_renderer;

#endif

#endif
