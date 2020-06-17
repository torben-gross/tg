#ifndef TG_GRAPHICS_VULKAN_FORWARD_RENDERER_H
#define TG_GRAPHICS_VULKAN_FORWARD_RENDERER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_forward_renderer
{
    tg_camera_h                  h_camera;
    struct
    {
        VkRenderPass             render_pass;
        VkFramebuffer            framebuffer;
        VkCommandBuffer          command_buffer;
    } shading_pass;
} tg_forward_renderer;

#endif

#endif
