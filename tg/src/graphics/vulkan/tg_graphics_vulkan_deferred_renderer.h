#ifndef TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H
#define TG_GRAPHICS_VULKAN_DEFERRED_RENDERER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_deferred_renderer_geometry_pass
{
    tg_color_image               position_attachment;
    tg_color_image               normal_attachment;
    tg_color_image               albedo_attachment;

    tg_vulkan_buffer             view_projection_ubo;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    VkCommandBuffer              command_buffer;
} tg_deferred_renderer_geometry_pass;

typedef struct tg_deferred_renderer_shading_pass
{
    tg_color_image               color_attachment;

    tg_vulkan_buffer             vbo;
    tg_vulkan_buffer             ibo;

    VkSemaphore                  rendering_finished_semaphore;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    tg_vulkan_descriptor         descriptor;

    VkShaderModule               vertex_shader_h;
    VkShaderModule               fragment_shader_h;
    VkPipelineLayout             pipeline_layout;
    VkPipeline                   graphics_pipeline;

    VkCommandBuffer              command_buffer;

    tg_vulkan_buffer             point_lights_ubo;

    struct
    {
        tg_vulkan_compute_shader     find_exposure_compute_shader;
        tg_vulkan_buffer             exposure_compute_buffer;

        VkRenderPass                 render_pass;
        VkFramebuffer                framebuffer;

        tg_vulkan_descriptor         descriptor;

        VkShaderModule               vertex_shader_h;
        VkShaderModule               fragment_shader_h;
        VkPipelineLayout             pipeline_layout;
        VkPipeline                   graphics_pipeline;
    } exposure;
} tg_deferred_renderer_shading_pass;

typedef struct tg_deferred_renderer_present_pass
{
    tg_vulkan_buffer         vbo;
    tg_vulkan_buffer         ibo;

    VkSemaphore              image_acquired_semaphore;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];

    tg_vulkan_descriptor     descriptor;

    VkShaderModule           vertex_shader_h;
    VkShaderModule           fragment_shader_h;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               graphics_pipeline;

    VkCommandBuffer          command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
} tg_deferred_renderer_present_pass;

typedef struct tg_deferred_renderer_clear_pass
{
    VkCommandBuffer    command_buffer;
} tg_deferred_renderer_clear_pass;

typedef struct tg_deferred_renderer
{
    const tg_camera*                      p_camera;
    tg_render_target                      render_target;
    tg_deferred_renderer_geometry_pass    geometry_pass;
    tg_deferred_renderer_shading_pass     shading_pass;
    tg_deferred_renderer_present_pass     present_pass;
    tg_deferred_renderer_clear_pass       clear_pass;
} tg_deferred_renderer;

#endif

#endif