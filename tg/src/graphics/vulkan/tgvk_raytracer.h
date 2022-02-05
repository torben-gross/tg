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
    // TODO: implement below
    //tgvk_buffer            color_ids;            // 8 bits per voxel
    //u8                     p_color_lut[3 * 256]; // Three 8 bit components per color, 256 colors // TODO: optionally less colors, less memory. put LUT into one huge array and only reference pointer/index to location in here?
} tg_obj;

typedef struct tg_raytracer_objs
{
    u32        capacity;
    u32        count;
    tg_obj*    p_objs;
} tg_raytracer_objs;

typedef struct tg_raytracer_visibility_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          pipeline;
    tgvk_descriptor_set    descriptor_set;

                                                   // 0: tg_raytracer.instance_data_ssbo. Data of the instance required to fetch its voxels
    tgvk_buffer            view_projection_ubo;    // 1: View and projection matrices for vertex shader, as defined by raytracer
    tgvk_buffer            raytracer_data_ubo;     // 2: Data required for voxel raytracing in fragment shader
    tgvk_buffer            voxel_data_ssbo;        // 3: Bitmap of voxels (either 1 or 0) in order x, y, z
    tgvk_buffer            visibility_buffer_ssbo; // 4: Output
    
    tgvk_framebuffer       framebuffer;
    
    tgvk_buffer            instance_id_vbo;        // 0: Instance IDs in ascending order (0, 1, ..., MAX_INSTANCES - 1)
    tgvk_buffer            cube_ibo;               // 1
    tgvk_buffer            cube_vbo_p;             // 2
    tgvk_buffer            cube_vbo_n;             // 3
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
    tgvk_buffer                     instance_data_ssbo;

    tg_raytracer_objs               objs;
    tg_raytracer_visibility_pass    visibility_pass;
    tg_raytracer_shading_pass       shading_pass;
    tg_raytracer_blit_pass          blit_pass;
    tg_raytracer_present_pass       present_pass;
    tg_raytracer_clear_pass         clear_pass;
} tg_raytracer;



void    tg_raytracer_create(const tg_camera* p_camera, u32 max_object_count, TG_OUT tg_raytracer* p_raytracer);
void    tg_raytracer_destroy(tg_raytracer* p_raytracer);
void    tg_raytracer_create_obj(tg_raytracer* p_raytracer, u32 grid_width, u32 grid_height, u32 grid_depth, f32 center_x, f32 center_y, f32 center_z);
void    tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);

#endif
