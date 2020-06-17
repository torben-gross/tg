#ifndef TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H
#define TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_deferred_renderer
{
    tg_camera_h                         h_camera;
    struct
    {
        tg_color_image                  position_attachment;
        tg_color_image                  normal_attachment;
        tg_color_image                  albedo_attachment;
        VkRenderPass                    render_pass;
        VkFramebuffer                   framebuffer;
        VkCommandBuffer                 command_buffer;
    } geometry_pass;
    struct
    {
        tg_color_image                  color_attachment;
        tg_vulkan_buffer                vbo;
        tg_vulkan_buffer                ibo;
        VkSemaphore                     rendering_finished_semaphore;
        VkRenderPass                    render_pass;
        VkFramebuffer                   framebuffer;
        tg_vulkan_descriptor            descriptor;
        tg_vulkan_shader                vertex_shader;
        tg_vulkan_shader                fragment_shader;
        VkPipelineLayout                pipeline_layout;
        VkPipeline                      graphics_pipeline;
        VkCommandBuffer                 command_buffer;
        tg_vulkan_buffer                point_lights_ubo;
    } shading_pass;
    struct
    {
        tg_vulkan_shader                acquire_exposure_compute_shader;
        tg_vulkan_descriptor            acquire_exposure_descriptor;
        VkPipelineLayout                acquire_exposure_pipeline_layout;
        VkPipeline                      acquire_exposure_compute_pipeline;
        tg_vulkan_buffer                exposure_storage_buffer;
        VkRenderPass                    render_pass;
        VkFramebuffer                   framebuffer;
        tg_vulkan_descriptor            descriptor;
        tg_vulkan_shader                vertex_shader;
        tg_vulkan_shader                fragment_shader;
        VkPipelineLayout                pipeline_layout;
        VkPipeline                      graphics_pipeline;
        VkCommandBuffer                 command_buffer;
    } tone_mapping_pass;
} tg_deferred_renderer;

#endif

#endif