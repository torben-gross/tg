#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_render_target.h"



typedef struct tg_raytracer_objs
{
    u32          capacity;
    u32          count;
    tgvk_obj*    p_objs;
} tg_raytracer_objs;

typedef struct tg_raytracer_visibility_pass
{
    tgvk_command_buffer    command_buffer;
    tgvk_pipeline          pipeline;
    tgvk_buffer            view_projection_ubo;
    tgvk_buffer            ray_tracing_ubo;
    tgvk_buffer            visibility_buffer; // u32 w; u32 h; u64 data[w * h];
    tgvk_framebuffer       framebuffer;
} tg_raytracer_visibility_pass;

typedef struct tg_raytracer_shading_pass
{
    tgvk_command_buffer    command_buffer;
    tgvk_shader            fragment_shader;
    tgvk_pipeline          graphics_pipeline;
    tgvk_buffer            ubo;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_shading_pass;

typedef struct tg_raytracer_blit_pass
{
    tgvk_command_buffer    command_buffer;
} tg_raytracer_blit_pass;

typedef struct tg_raytracer_present_pass
{
    tgvk_command_buffer    p_command_buffers[TG_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore            image_acquired_semaphore;
    tgvk_framebuffer       p_framebuffers[TG_MAX_SWAPCHAIN_IMAGES];
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
    tgvk_image                      hdr_color_attachment;
    tg_render_target                render_target;
    VkSemaphore                     semaphore;

    tg_raytracer_objs               objs;
    tg_raytracer_visibility_pass    visibility_pass;
    tg_raytracer_shading_pass       shading_pass;
    tg_raytracer_blit_pass          blit_pass;
    tg_raytracer_present_pass       present_pass;
    tg_raytracer_clear_pass         clear_pass;
} tg_raytracer;



void    tg_raytracer_create(const tg_camera* p_camera, u32 max_object_count, TG_OUT tg_raytracer* p_raytracer);
void    tg_raytracer_destroy(tg_raytracer* p_raytracer);
void    tg_raytracer_create_obj(tg_raytracer* p_raytracer, u32 log2_w, u32 log2_h, u32 log2_d);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);

#endif
