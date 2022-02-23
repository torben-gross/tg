#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/tg_sparse_voxel_octree.h"
#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_render_target.h"



typedef struct tg_raytracer_scene
{
    u32                 object_capacity;
    u32                 object_count;
    tg_voxel_object*    p_objects;

    u32                 cluster_capacity;
    u32                 cluster_count;
    u32*                p_voxel_cluster_data;
    
    tg_svo              svo;
} tg_raytracer_scene;

typedef struct tg_raytracer_buffers
{
    tgvk_buffer            idx_vbo;                         // Mainly used for the indices of the clusters during instanced rendering for look-up. Also used for debug instanced rendering
    tgvk_buffer            cube_ibo;
    tgvk_buffer            cube_vbo;

    tgvk_buffer            cluster_idx_to_object_idx_ssbo;  // [cluster idx]             Maps cluster idx to its object idx
    tgvk_buffer            object_data_ssbo;                // [object idx]              Metrics and first cluster idx
    tgvk_buffer            object_color_lut_ssbo;           // [object idx]              Color LUT
    tgvk_buffer            voxel_cluster_ssbo;              // [cluster idx + voxel idx] Cluster of voxels            (1 bit per voxel)
    tgvk_buffer            color_lut_idx_cluster_ssbo;      // [cluster idx + voxel idx] Cluster of color LUT indices (8 bit per voxel)

    tgvk_buffer            svo_ssbo;
    tgvk_buffer            svo_nodes_ssbo;
    tgvk_buffer            svo_leaf_node_data_ssbo;
    tgvk_buffer            svo_voxel_data_ssbo;

    tgvk_buffer            visibility_buffer_ssbo;
    tgvk_buffer            view_projection_ubo;
    tgvk_buffer            raytracer_data_ubo;
    tgvk_buffer            shading_data_ubo;

    tgvk_buffer            debug_matrices_ssbo;
    tgvk_buffer            debug_colors_ssbo;
} tg_raytracer_buffers;

typedef struct tg_raytracer_visibility_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_visibility_pass;

typedef struct tg_raytracer_svo_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_svo_pass;

typedef struct tg_raytracer_shading_pass
{
    tgvk_command_buffer    command_buffer;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_shading_pass;

typedef struct tg_raytracer_debug_pass
{
    u32                    capacity;
    u32                    count;
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

    tg_raytracer_scene              scene;

    tg_raytracer_buffers            buffers;
    tg_raytracer_visibility_pass    visibility_pass;
    tg_raytracer_svo_pass           svo_pass;
    tg_raytracer_shading_pass       shading_pass;
    tg_raytracer_debug_pass         debug_pass;
    tg_raytracer_blit_pass          blit_pass;
    tg_raytracer_present_pass       present_pass;
    tg_raytracer_clear_pass         clear_pass;
} tg_raytracer;



void    tg_raytracer_create(const tg_camera* p_camera, u32 max_n_instances, u32 max_n_clusters, TG_OUT tg_raytracer* p_raytracer);
void    tg_raytracer_destroy(tg_raytracer* p_raytracer);
void    tg_raytracer_create_object(tg_raytracer* p_raytracer, v3 center, v3u extent);
void    tg_raytracer_push_debug_cuboid(tg_raytracer* p_raytracer, m4 transformation_matrix, v3 color); // Original cube's extent is 1^3 and position is centered at origin
void    tg_raytracer_push_debug_line(tg_raytracer* p_raytracer, v3 src, v3 dst, v3 color);
void    tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);

#endif
