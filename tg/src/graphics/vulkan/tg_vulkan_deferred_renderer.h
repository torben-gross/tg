#ifndef TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H
#define TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef enum tg_deferred_geometry_color_attachment
{
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_POSITION,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_NORMAL,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_ALBEDO,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_METALLIC_ROUGHNESS_AO,

    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT
} tg_deferred_geometry_color_attachment;

typedef struct tg_deferred_renderer
{
    tg_camera_h                         h_camera;
    struct
    {
        tg_color_image                  p_color_attachments[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT];
        VkRenderPass                    render_pass;
        tg_vulkan_framebuffer           framebuffer;
        VkCommandBuffer                 command_buffer;
    } geometry_pass;
    struct
    {
        tg_color_image                  color_attachment;
        tg_vulkan_buffer                vbo;
        tg_vulkan_buffer                ibo;
        VkSemaphore                     rendering_finished_semaphore;
        VkRenderPass                    render_pass;
        tg_vulkan_framebuffer           framebuffer;
        tg_vulkan_shader                vertex_shader;
        tg_vulkan_shader                fragment_shader;
        tg_vulkan_pipeline              graphics_pipeline;
        VkCommandBuffer                 command_buffer;
        tg_vulkan_buffer                camera_ubo;
        tg_vulkan_buffer                lighting_ubo;
    } shading_pass;
    struct
    {
        tg_vulkan_shader                acquire_exposure_compute_shader;
        tg_vulkan_pipeline              acquire_exposure_compute_pipeline;
        tg_vulkan_buffer                exposure_storage_buffer;
        VkRenderPass                    render_pass;
        tg_vulkan_framebuffer           framebuffer;
        tg_vulkan_shader                vertex_shader;
        tg_vulkan_shader                fragment_shader;
        tg_vulkan_pipeline              graphics_pipeline;
        VkCommandBuffer                 command_buffer;
    } tone_mapping_pass;
} tg_deferred_renderer;

#endif

#endif