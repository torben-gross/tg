#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_render_target.h"



// TODO:
// 32 bits for depth (Karis et al. use 30 bits: http://advances.realtimerendering.com/s2021/Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf p. 84)
// 32 bits for voxel ids
typedef struct tg_obj
{
    u32                    flags;           // TODO: will hold flags like static
    v3                     center;
    u32                    first_voxel_id;
    u16                    packed_log2_whd; // 5 bits each for log2_w, log2_h, log2_d. One bit unused. TODO: we only need to ensure division by 16 for 3 lods, so just not use lower 3 bits? this member would need to become bigger, though.. but for lod down to w,h,d = 2, we need 2^n?
    u16                    pad;             // TODO: padding
    tgvk_buffer            ubo;             // tg_obj_ubo
    tgvk_descriptor_set    descriptor_set;
    tgvk_buffer            voxels;          // (w/(L+1) * h/(L+1)) bits for lod L >= 0, L < TG_MAX_LODS. Data is packed contiguous
    // TODO: implement below
    //tgvk_buffer            color_ids;            // 8 bits per voxel
    //u8                     p_color_lut[3 * 256]; // Three 8 bit components per color, 256 colors // TODO: optionally less colors, less memory. put LUT into one huge array and only reference pointer/index to location in here?
} tg_obj;

typedef struct tg_raytracer_objs
{
    u32          capacity;
    u32          count;
    tg_obj*    p_objs;
} tg_raytracer_objs;

typedef struct tg_raytracer_visibility_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          pipeline;
    tgvk_buffer            view_projection_ubo;
    tgvk_buffer            raytracing_ubo;
    tgvk_buffer            visibility_buffer; // u32 w; u32 h; u64 data[w * h];
    tgvk_framebuffer       framebuffer;
    tgvk_buffer            cube_ibo;
    tgvk_buffer            cube_vbo_p;
    tgvk_buffer            cube_vbo_n;
} tg_raytracer_visibility_pass;

typedef struct tg_raytracer_shading_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
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
void    tg_raytracer_create_obj(tg_raytracer* p_raytracer, u32 w, u32 h, u32 d);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);

#endif
