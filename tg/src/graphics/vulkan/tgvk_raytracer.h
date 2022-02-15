#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_render_target.h"



typedef struct tg_raytracer_instance_buffer
{
    u32             capacity;
    u32             count;
    tg_instance*    p_instances;
} tg_raytracer_instance_buffer;

typedef struct tg_raytracer_visibility_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;

    tgvk_buffer            raytracer_data_ubo;
    tgvk_buffer            voxel_data_ssbo;
    tgvk_buffer            visibility_buffer_ssbo;
    
    tgvk_framebuffer       framebuffer;
    
    tgvk_buffer            instance_id_vbo;
} tg_raytracer_visibility_pass;

typedef struct tg_raytracer_shading_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_buffer            shading_data_ubo;
    tgvk_buffer            color_lut_id_ssbo;
    tgvk_buffer            color_lut_ssbo;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_shading_pass;

typedef struct tg_raytracer_debug_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_debug_pass;

typedef struct tg_raytracer_blit_pass
{
    tgvk_command_buffer    command_buffer;
} tg_raytracer_blit_pass;

typedef struct tg_raytracer_present_pass
{
    tgvk_command_buffer    p_command_buffers[TG_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore            image_acquired_semaphore;
    tgvk_framebuffer       p_framebuffers[TG_MAX_SWAPCHAIN_IMAGES];
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
} tg_raytracer_present_pass;

typedef struct tg_raytracer_clear_pass
{
    tgvk_command_buffer    command_buffer;
    tgvk_pipeline          compute_pipeline;
    tgvk_descriptor_set    descriptor_set;
} tg_raytracer_clear_pass;

typedef struct tg_raytracer
{
    const tg_camera*                p_camera;
    tg_render_target                render_target;
    VkSemaphore                     semaphore;
    tgvk_buffer                     cube_ibo;
    tgvk_buffer                     cube_vbo;

    tgvk_buffer                     view_projection_ubo;
    tgvk_buffer                     instance_data_ssbo;

    tg_raytracer_instance_buffer    instance_buffer;

    tg_raytracer_visibility_pass    visibility_pass;
    tg_raytracer_shading_pass       shading_pass;
    tg_raytracer_debug_pass         debug_pass;
    tg_raytracer_blit_pass          blit_pass;
    tg_raytracer_present_pass       present_pass;
    tg_raytracer_clear_pass         clear_pass;
} tg_raytracer;



void    tg_raytracer_create(const tg_camera* p_camera, u32 max_instance_count, TG_OUT tg_raytracer* p_raytracer);
void    tg_raytracer_destroy(tg_raytracer* p_raytracer);
void    tg_raytracer_create_instance(tg_raytracer* p_raytracer, u32 grid_width, u32 grid_height, u32 grid_depth, f32 center_x, f32 center_y, f32 center_z);
void    tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);

#endif
