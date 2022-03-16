#ifndef TGVK_RAYTRACER_H
#define TGVK_RAYTRACER_H

#include "graphics/tg_sparse_voxel_octree.h"
#include "graphics/vulkan/tgvk_core.h"
#include "graphics/vulkan/tgvk_render_target.h"

#include "graphics/vulkan/tgvk_font.h"
#include "util/tg_hashmap.h"



#define TGGUI_MAX_N_DRAW_CALLS      64
#define TGGUI_TEMP_BUFFER_SIZE      64
#define TGGUI_DEFAULT_FORMAT_F32    "%.3f"



typedef enum tggui_color_type
{
    TGGUI_COLOR_TEXT,
    TGGUI_COLOR_WINDOW_BG,
    TGGUI_COLOR_FRAME,          // Checkbox, slider, text input
    TGGUI_COLOR_FRAME_HOVERED,
    TGGUI_COLOR_FRAME_ACTIVE,
    TGGUI_COLOR_BUTTON,
    TGGUI_COLOR_BUTTON_HOVERED,
    TGGUI_COLOR_BUTTON_ACTIVE,
    TGGUI_COLOR_SEPARATOR,
    TGGUI_COLOR_TITLE_BG,
    TGGUI_COLOR_CHECKMARK,
    TGGUI_COLOR_TYPE_COUNT
} tggui_color_type;

typedef struct tggui_temp
{
    v2      window_next_position;
    v2      window_next_size;

    v2      window_position;
    v2      window_size;
    f32     window_content_base_position_x;
    v2      last_line_end_offset;
    v2      offset;
    u32     active_id;
    char    p_temp_buffer[TGGUI_TEMP_BUFFER_SIZE];
} tggui_temp;

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

    v2             viewport_size;
    tggui_temp     temp;
} tggui_context;



typedef enum tg_debug_show
{
    TG_DEBUG_SHOW_NONE               = 0,
    TG_DEBUG_SHOW_OBJECT_INDEX       = 1,
    TG_DEBUG_SHOW_DEPTH              = 2,
    TG_DEBUG_SHOW_CLUSTER_INDEX      = 3,
    TG_DEBUG_SHOW_VOXEL_INDEX        = 4,
    TG_DEBUG_SHOW_BLOCKS             = 5,
    TG_DEBUG_SHOW_COLOR_LUT_INDEX    = 6,
    TG_DEBUG_SHOW_COLOR              = 7,
    TG_DEBUG_SHOW_COUNT
} tg_debug_show;

typedef struct tg_scene
{
    u32                 object_capacity;             // The number of allocated objects
    u32                 n_objects;                   // The number of utilized objects
    tg_voxel_object*    p_objects;                   // All objects as a contiguous array
    u32                 n_available_object_indices;  // The number of available object indices on the stacl
    u32*                p_available_object_indices;  // The available object indices as a stack

    u32                 cluster_pointer_capacity;    // The number of allocated cluster pointers TODO: We may later have more cluster pointers than actual clusters and need a second member for that
    u32                 n_cluster_pointers;          // The number of utilized cluster pointers
    u32*                p_cluster_pointers;          // All cluster pointers as a contiguous array. Each element is an index, that points to a cluster
    u32                 n_available_cluster_indices; // The number of available cluster indices on the stack
    u32*                p_available_cluster_indices; // The available cluster indices as a stack

    u32*                p_voxel_cluster_data;        // Data of the clusters storing the voxels (1 bit per voxel)
    u32*                p_cluster_idx_to_object_idx; // Maps every cluster index to its object index
    
    tg_svo              svo;
} tg_scene;

typedef struct tg_raytracer_data
{
    tgvk_buffer            idx_vbo;                         // Mainly used for the indices of the clusters during instanced rendering for look-up. Also used for debug instanced rendering
    tgvk_buffer            cube_ibo;
    tgvk_buffer            cube_vbo;
    tgvk_buffer            quad_vbo;

    tgvk_buffer            cluster_pointer_ssbo;            // [cluster pointer]            Maps cluster pointers to clusters
    tgvk_buffer            cluster_idx_to_object_idx_ssbo;  // [cluster idx]                Maps cluster idx to its object idx
    tgvk_buffer            object_data_ssbo;                // [object idx]                 Metrics and first cluster idx
    tgvk_buffer            object_color_lut_ssbo;           // [object idx]                 Color LUT
    tgvk_buffer            voxel_cluster_ssbo;              // [cluster idx + voxel idx]    Cluster of voxels            (1 bit per voxel)
    tgvk_buffer            color_lut_idx_cluster_ssbo;      // [cluster idx + voxel idx]    Cluster of color LUT indices (8 bit per voxel)

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
    tgvk_buffer            debug_visualization_type_ubo;
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

typedef struct tg_raytracer_debug
{
    b32 show_object_bounds;
    b32 show_svo;
    b32 show_svo_leaves_only;
} tg_raytracer_debug;

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

    tg_raytracer_debug              debug;
} tg_raytracer;



void    tg_raytracer_create(const tg_camera* p_camera, u32 max_n_objects, u32 max_n_clusters, TG_OUT tg_raytracer* p_raytracer);
void    tg_raytracer_destroy(tg_raytracer* p_raytracer);
void    tg_raytracer_set_debug_visualization(tg_raytracer* p_raytracer, tg_debug_show type);
void    tg_raytracer_create_object(tg_raytracer* p_raytracer, v3 center, v3u extent);
void    tg_raytracer_destroy_object(tg_raytracer* p_raytracer, u32 object_idx);
b32     tg_object_is_initialized(const tg_scene* p_scene, u32 object_idx);
void    tg_raytracer_push_debug_cuboid(tg_raytracer* p_raytracer, m4 transformation_matrix, v3 color); // Original cube's extent is 1^3 and position is centered at origin
void    tg_raytracer_push_debug_line(tg_raytracer* p_raytracer, v3 src, v3 dst, v3 color);
void    tg_raytracer_color_lut_set(tg_raytracer* p_raytracer, u8 index, f32 r, f32 g, f32 b);
void    tg_raytracer_render(tg_raytracer* p_raytracer);
void    tg_raytracer_clear(tg_raytracer* p_raytracer);
b32     tg_raytracer_get_hovered_voxel(tg_raytracer* p_raytracer, u32 screen_x, u32 screen_y, TG_OUT f32* p_depth, TG_OUT u32* p_cluster_idx, TG_OUT u32* p_voxel_idx);

void    tggui_set_context(tggui_context* p_context);
void    tggui_set_viewport_size(f32 viewport_width, f32 viewport_height);
b32     tggui_is_in_focus(void);
void    tggui_window_set_next_position(f32 position_x, f32 position_y);                    // Anchor is in top left corner
void    tggui_window_set_next_size(f32 size_x, f32 size_y);
void    tggui_window_begin(const char* p_window_name);
void    tggui_window_end(void);
void    tggui_new_line(void);
void    tggui_same_line(void);
void    tggui_separator(void);
b32     tggui_button(const char* p_label);
b32     tggui_checkbox(const char* p_label, b32* p_value);
b32     tggui_input_f32(const char* p_label, f32 width, f32* p_value);                     // Returns 'true' on value changed
b32     tggui_input_text(const char* p_label, f32 width, u32 buffer_size, char* p_buffer); // Returns 'true' on value changed
void    tggui_text(const char* p_format, ...);


#endif
