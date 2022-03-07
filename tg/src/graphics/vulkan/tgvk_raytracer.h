#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/tg_sparse_voxel_octree.h"
#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_font.h"
#include "graphics/vulkan/tgvk_render_target.h"



#define TGGUI_MAX_N_DRAW_CALLS 8



typedef enum tggui_color_type
{
    TGGUI_COLOR_TEXT,
    TGGUI_COLOR_WINDOW_BG,
    TGGUI_COLOR_BUTTON,
    TGGUI_COLOR_BUTTON_HOVERED,
    TGGUI_COLOR_BUTTON_ACTIVE,
    TGGUI_COLOR_FRAME,          // Checkbox, slider, text input
    TGGUI_COLOR_FRAME_HOVERED,
    TGGUI_COLOR_FRAME_ACTIVE,
    TGGUI_COLOR_CHECKMARK,
    TGGUI_COLOR_TYPE_COUNT
} tggui_color_type;

typedef struct tggui_style
{
    tgvk_font    font;
    v4           p_colors[TGGUI_COLOR_TYPE_COUNT];
} tggui_style;

typedef struct tggui_context
{
    tggui_style    style;

    u32            quad_capacity;
    u32            n_quads;
    tgvk_buffer    transform_ssbo;
    tgvk_buffer    colors_ssbo;
    tgvk_image     white_texture;

    u32            n_draw_calls;
    tgvk_image*    p_textures[TGGUI_MAX_N_DRAW_CALLS];
    u32            p_n_instances_per_draw_call[TGGUI_MAX_N_DRAW_CALLS];

    f32            window_next_position_x;
    f32            window_next_position_y;
    f32            window_next_size_x;
    f32            window_next_size_y;

    f32            offset_x;
    f32            offset_y;
} tggui_context;



typedef struct tg_scene
{
    u32                 object_capacity;
    u32                 n_objects;
    tg_voxel_object*    p_objects;

    u32                 cluster_capacity;
    u32                 n_clusters;
    u32*                p_voxel_cluster_data;
    u32*                p_cluster_idx_to_object_idx;
    
    tg_svo              svo;
} tg_scene;

typedef struct tg_raytracer_data
{
    tgvk_buffer            idx_vbo;                         // Mainly used for the indices of the clusters during instanced rendering for look-up. Also used for debug instanced rendering
    tgvk_buffer            cube_ibo;
    tgvk_buffer            cube_vbo;
    tgvk_buffer            quad_vbo;

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
    tgvk_buffer            camera_ubo;
    tgvk_buffer            environment_ubo;

    tgvk_buffer            debug_matrices_ssbo;
    tgvk_buffer            debug_colors_ssbo;
} tg_raytracer_data;

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

typedef struct tg_raytracer_gui_pass
{
    tgvk_command_buffer    command_buffer;
    VkFence                fence;
    VkRenderPass           render_pass;
    tgvk_pipeline          graphics_pipeline;
    tgvk_descriptor_set    descriptor_set;
    tgvk_framebuffer       framebuffer;
} tg_raytracer_gui_pass;

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

    tg_scene                        scene;
    tggui_context                   gui_context;

    tg_raytracer_data               data;
    tg_raytracer_visibility_pass    visibility_pass;
    tg_raytracer_svo_pass           svo_pass;
    tg_raytracer_shading_pass       shading_pass;
    tg_raytracer_debug_pass         debug_pass;
    tg_raytracer_gui_pass           gui_pass;
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

void    tggui_window_set_next_position(tg_raytracer* p_raytracer, f32 position_x, f32 position_y); // Anchor is in top left corner
void    tggui_window_set_next_size(tg_raytracer* p_raytracer, f32 size_x, f32 size_y);
void    tggui_window_begin(tg_raytracer* p_raytracer, const char* p_window_name);
void    tggui_window_end(tg_raytracer* p_raytracer);
void    tggui_same_line(tg_raytracer* p_raytracer);
b32     tggui_button(tg_raytracer* p_raytracer, const char* p_label);
b32     tggui_checkbox(tg_raytracer* p_raytracer, const char* p_label, b32* p_value);
void    tggui_text(tg_raytracer* p_raytracer, const char* p_format, ...);


#endif
