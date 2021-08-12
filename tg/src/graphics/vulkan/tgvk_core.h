#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include "graphics/tg_spirv.h"
#include "graphics/vulkan/tgvk_common.h"
#include "graphics/vulkan/tgvk_memory_allocator.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_list.h"



#ifdef TG_DEBUG

#define TGVK_BUFFER_CREATE(size, buffer_usage_flags, type) \
    tgvk_buffer_create(size, buffer_usage_flags, type, __LINE__, __FILE__)

#define TGVK_CUBE_MAP_CREATE(dimension, format, p_sampler_create_info) \
    tgvk_cube_map_create(dimension, format, p_sampler_create_info, __LINE__, __FILE__)

#define TGVK_IMAGE_CREATE(type_flags, width, height, format, p_sampler_create_info) \
    tgvk_image_create(type_flags, width, height, format, p_sampler_create_info, __LINE__, __FILE__)

#define TGVK_IMAGE_CREATE2(type, p_filename, p_sampler_create_info) \
    tgvk_image_create2(type, p_filename, p_sampler_create_info, __LINE__, __FILE__)

#define TGVK_IMAGE_DESERIALIZE(p_filename, p_image) \
    tgvk_image_deserialize(p_filename, p_image, __LINE__, __FILE__)

#define TGVK_IMAGE_3D_CREATE(type, width, height, depth, format, p_sampler_create_info) \
    tgvk_image_3d_create(type, width, height, depth, format, p_sampler_create_info, __LINE__, __FILE__)

#define TGVK_LAYERED_IMAGE_CREATE(type, width, height, layers, format, p_sampler_create_info) \
    tgvk_layered_image_create(type, width, height, layers, format, p_sampler_create_info, __LINE__, __FILE__)

#define TGVK_RENDER_TARGET_CREATE(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags) \
    tgvk_render_target_create(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags, __LINE__, __FILE__)

#define TGVK_UNIFORM_BUFFER_CREATE(size) \
    tgvk_uniform_buffer_create(size, __LINE__, __FILE__)

#else

#define TGVK_BUFFER_CREATE(size, buffer_usage_flags, type) \
    tgvk_buffer_create(size, buffer_usage_flags, type)

#define TGVK_CUBE_MAP_CREATE(dimension, format, p_sampler_create_info) \
    tgvk_cube_map_create(dimension, format, p_sampler_create_info)

#define TGVK_IMAGE_CREATE(type_flags, width, height, format, p_sampler_create_info) \
    tgvk_image_create(type_flags, width, height, format, p_sampler_create_info)

#define TGVK_IMAGE_CREATE2(type, p_filename, p_sampler_create_info) \
    tgvk_image_create2(type, p_filename, p_sampler_create_info)

#define TGVK_IMAGE_DESERIALIZE(p_filename, p_image) \
    tgvk_image_deserialize(p_filename, p_image)

#define TGVK_IMAGE_3D_CREATE(type, width, height, depth, format, p_sampler_create_info) \
    tgvk_image_3d_create(type, width, height, depth, format, p_sampler_create_info)

#define TGVK_LAYERED_IMAGE_CREATE(type, width, height, layers, format, p_sampler_create_info) \
    tgvk_layered_image_create(type, width, height, layers, format, p_sampler_create_info)

#define TGVK_RENDER_TARGET_CREATE(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags) \
    tgvk_render_target_create(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags)

#define TGVK_UNIFORM_BUFFER_CREATE(size) \
    tgvk_uniform_buffer_create(size)

#endif



typedef enum tgvk_atmosphere_luminance
{
    TG_LUMINANCE_NONE,
    TG_LUMINANCE_APPROXIMATE,
    TG_LUMINANCE_PRECOMPUTED
} tgvk_atmosphere_luminance;

typedef enum tgvk_command_pool_type
{
    TGVK_COMMAND_POOL_TYPE_COMPUTE,
    TGVK_COMMAND_POOL_TYPE_GRAPHICS,
    TGVK_COMMAND_POOL_TYPE_PRESENT
} tgvk_command_pool_type;

typedef enum tgvk_image_type
{
    TGVK_IMAGE_TYPE_COLOR      = (1 << 0),
    TGVK_IMAGE_TYPE_DEPTH      = (1 << 1),
    TGVK_IMAGE_TYPE_STORAGE    = (1 << 2)
} tgvk_image_type;
typedef u32 tgvk_image_type_flags;

typedef enum tgvk_material_type
{
    TGVK_MATERIAL_TYPE_INVALID = 0,
    TGVK_MATERIAL_TYPE_DEFERRED,
    TGVK_MATERIAL_TYPE_FORWARD
} tgvk_material_type;

typedef enum tgvk_queue_type
{
    TGVK_QUEUE_TYPE_COMPUTE,
    TGVK_QUEUE_TYPE_GRAPHICS,
    TGVK_QUEUE_TYPE_PRESENT,
    TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY,
    TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY,
    // TODO: TGVK_QUEUE_TYPE_TRANSFER,
    TGVK_QUEUE_TYPE_COUNT
} tgvk_queue_type;

typedef enum tgvk_geometry_attachment
{
    TGVK_GEOMETRY_ATTACHMENT_POSITION_3xF32_NORMAL_3xU8_METALLIC_1xU8    = 0,
    TGVK_GEOMETRY_ATTACHMENT_ALBEDO_3xU8_ROUGHNESS_1xU8                  = 1,
    TGVK_GEOMETRY_ATTACHMENT_DEPTH                                       = 2,
    TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT                                 = 2,
    TGVK_GEOMETRY_ATTACHMENT_DEPTH_COUNT                                 = 1,
    TGVK_GEOMETRY_ATTACHMENT_COUNT                                       = TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT + TGVK_GEOMETRY_ATTACHMENT_DEPTH_COUNT
} tgvk_geometry_attachment;



typedef struct tgvk_atmosphere_density_profile_layer
{
	f64    width;
	f64    exp_term;
	f64    exp_scale;
	f64    linear_term;
	f64    constant_term;
} tgvk_atmosphere_density_profile_layer;

typedef struct tgvk_buffer
{
    VkBuffer             buffer;
    tgvk_memory_block    memory;
} tgvk_buffer;

typedef struct tgvk_command_buffer
{
    VkCommandBuffer           command_buffer;
    tgvk_command_pool_type    command_pool_type;
    VkCommandBufferLevel      command_buffer_level;
} tgvk_command_buffer;

typedef struct tgvk_sampler
{
    VkFilter                mag_filter;
    VkFilter                min_filter;
    VkSamplerAddressMode    address_mode_u;
    VkSamplerAddressMode    address_mode_v;
    VkSamplerAddressMode    address_mode_w;
    VkSampler               sampler;
} tgvk_sampler;

typedef struct tgvk_cube_map
{
    u32                  dimension;
    VkFormat             format;
    VkImage              image;
    tgvk_memory_block    memory;
    VkImageView          image_view;
    tgvk_sampler         sampler;
} tgvk_cube_map;

typedef struct tgvk_pipeline_layout
{
    u32                             global_resource_count;
    tg_spirv_global_resource        p_global_resources[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkShaderStageFlags              p_shader_stages[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkDescriptorSetLayoutBinding    p_descriptor_set_layout_bindings[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkDescriptorSetLayout           descriptor_set_layout;
    VkPipelineLayout                pipeline_layout;
} tgvk_pipeline_layout;

typedef struct tgvk_descriptor_set
{
    VkDescriptorPool    descriptor_pool;
    VkDescriptorSet     descriptor_set;
} tgvk_descriptor_set;

typedef struct tgvk_image
{
    tgvk_image_type      type;
    u32                  width;
    u32                  height;
    VkFormat             format;
    VkImage              image;
    tgvk_memory_block    memory;
    VkImageView          image_view;
    tgvk_sampler         sampler;
} tgvk_image;

typedef struct tgvk_image_3d
{
    tgvk_image_type      type;
    u32                  width;
    u32                  height;
    u32                  depth;
    VkFormat             format;
    VkImage              image;
    tgvk_memory_block    memory;
    VkImageView          image_view;
    tgvk_sampler         sampler;
} tgvk_image_3d;

typedef struct tgvk_framebuffer
{
    u32              width;
    u32              height;
    u32              layers;
    VkFramebuffer    framebuffer; // TODO: this should contain its render_pass, as its needed later anyway (tgvk_cmd_begin_render_pass)
} tgvk_framebuffer;

typedef struct tgvk_shader tgvk_shader;
typedef struct tgvk_graphics_pipeline_create_info
{
    const tgvk_shader*      p_vertex_shader;
    const tgvk_shader*      p_geometry_shader;
    const tgvk_shader*      p_fragment_shader;
    VkCullModeFlagBits      cull_mode;
    VkBool32                depth_test_enable;
    VkBool32                depth_write_enable;
    const tg_blend_mode*    p_blend_modes;      // TG_NULL sets blending for every attachment to TG_BLEND_MODE_NONE
    VkRenderPass            render_pass;
    v2                      viewport_size;
    VkPolygonMode           polygon_mode;
} tgvk_graphics_pipeline_create_info;

typedef struct tgvk_image_extent
{
    f32    left;
    f32    bottom;
    f32    right;
    f32    top;
} tgvk_image_extent;

typedef struct tgvk_layered_image
{
    tgvk_image_type      type;
    u32                  width;
    u32                  height;
    u32                  layers;
    VkFormat             format;
    VkImage              image;
    tgvk_memory_block    memory;
    VkImageView          read_image_view;
    VkImageView          write_image_view;
    tgvk_sampler         sampler;
} tgvk_layered_image;

typedef enum tgvk_layout_type
{
    TGVK_LAYOUT_UNDEFINED = 0,
    TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE,
    TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE,
    TGVK_LAYOUT_SHADER_READ_C,
    TGVK_LAYOUT_SHADER_READ_CF,
    TGVK_LAYOUT_SHADER_READ_CFV,
    TGVK_LAYOUT_SHADER_READ_CV,
    TGVK_LAYOUT_SHADER_READ_F,
    TGVK_LAYOUT_SHADER_READ_FV,
    TGVK_LAYOUT_SHADER_READ_V,
    TGVK_LAYOUT_SHADER_WRITE_C,
    TGVK_LAYOUT_SHADER_WRITE_CF,
    TGVK_LAYOUT_SHADER_WRITE_CFV,
    TGVK_LAYOUT_SHADER_WRITE_CV,
    TGVK_LAYOUT_SHADER_WRITE_F,
    TGVK_LAYOUT_SHADER_WRITE_FV,
    TGVK_LAYOUT_SHADER_WRITE_V,
    TGVK_LAYOUT_SHADER_READ_WRITE_C,
    TGVK_LAYOUT_SHADER_READ_WRITE_CF,
    TGVK_LAYOUT_SHADER_READ_WRITE_CFV,
    TGVK_LAYOUT_SHADER_READ_WRITE_CV,
    TGVK_LAYOUT_SHADER_READ_WRITE_F,
    TGVK_LAYOUT_SHADER_READ_WRITE_FV,
    TGVK_LAYOUT_SHADER_READ_WRITE_V,
    TGVK_LAYOUT_TRANSFER_READ,
    TGVK_LAYOUT_TRANSFER_WRITE
} tgvk_layout_type;

typedef struct tgvk_pipeline
{
    b32                     is_graphics_pipeline;
    tgvk_pipeline_layout    layout;
    VkPipeline              pipeline;
} tgvk_pipeline;

typedef struct tgvk_queue
{
    tg_mutex_h    h_mutex;
    u32           queue_family_index;
    u32           queue_index;
    f32           priority;
    VkQueue       queue;
    VkFence       fence;
} tgvk_queue;

typedef struct tgvk_render_command_renderer_info
{
    tg_renderer_h          h_renderer;
    tgvk_descriptor_set    descriptor_set;
    tgvk_command_buffer    command_buffer;
} tgvk_render_command_renderer_info;

typedef struct tgvk_screen_vertex
{
    v2    position;
    v2    uv;
} tgvk_screen_vertex;

typedef struct tgvk_shader
{
    tg_spirv_layout    spirv_layout;
    VkShaderModule     shader_module;
} tgvk_shader;

typedef struct tgvk_shared_render_resources
{
    tgvk_buffer      screen_quad_indices;
    tgvk_buffer      screen_quad_positions_buffer;
    tgvk_buffer      screen_quad_uvs_buffer;

    VkRenderPass     geometry_render_pass;
    VkRenderPass     shading_render_pass;
    VkRenderPass     forward_render_pass;
    VkRenderPass     tone_mapping_render_pass;
    VkRenderPass     ui_render_pass;
    VkRenderPass     present_render_pass;

    struct
    {
        b32                  initialized; // TODO: don't waste 32 bit
        struct
        {
            tgvk_buffer      view_projection_ubo;
            VkRenderPass     render_pass;
            tgvk_pipeline    pipeline;
        } vis;
        tgvk_buffer          cube_ibo;
        tgvk_buffer          cube_vbo_p;
        tgvk_buffer          cube_vbo_n;
        VkRenderPass         render_pass;
        tgvk_pipeline        graphics_pipeline;
    } ray_tracer;
} tgvk_shared_render_resources;

typedef struct tgvk_surface
{
    VkSurfaceKHR          surface;
    VkSurfaceFormatKHR    format;
} tgvk_surface;



typedef struct tg_color_image
{
    tg_structure_type    type;
    tgvk_image           image;
} tg_color_image;

typedef struct tg_compute_shader
{
    tg_structure_type      type;
    tgvk_shader            shader;
    tgvk_pipeline          compute_pipeline;
    tgvk_descriptor_set    descriptor_set;
} tg_compute_shader;

typedef struct tg_cube_map
{
    tg_structure_type     type;
    tgvk_cube_map         cube_map;
} tg_cube_map;

typedef struct tg_depth_image
{
    tg_structure_type    type;
    tgvk_image           image;
} tg_depth_image;

typedef struct tg_font
{
    tg_structure_type    type;
    f32                  max_glyph_height;
	u16                  glyph_count;
	u8                   p_char_to_glyph[256];
	tgvk_image           texture_atlas;
    struct tg_font_glyph
    {
        v2               size;
        v2               uv_min;
        v2               uv_max;
        f32              advance_width;
        f32              left_side_bearing;
        f32              bottom_side_bearing;
        u16              kerning_count;
        struct tg_font_glyph_kerning
        {
            u8           right_glyph_idx;
            f32          kerning;
        }* p_kernings;
    }* p_glyphs;
} tg_font;

typedef struct tg_fragment_shader
{
    tg_structure_type    type;
    tgvk_shader          shader;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_structure_type       type;
    tgvk_material_type      material_type;
    tg_vertex_shader_h      h_vertex_shader;
    tg_fragment_shader_h    h_fragment_shader;
    tgvk_pipeline           pipeline;
} tg_material;

// TODO:
// 32 bits for depth (Karis et al. used 30 bits: http://advances.realtimerendering.com/s2021/Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf p. 84)
// 32 bits for voxel ids
typedef struct tg_obj
{
    u16                    type;                 // tg_structure_type
    u32                    first_voxel_id;
    u16                    packed_log2_whd;      // 5 bits each for log2_w, log2_h, log2_d. One bit unused. TODO: we only need to ensure division by 16 for 3 lods, so just not use lower 3 bits? this member would need to become bigger, though.. but for lod down to w,h,d = 2, we need 2^n?
    tgvk_buffer            ubo;                  // 32 bits first_voxel_id
    tgvk_descriptor_set    descriptor_set;
    tgvk_buffer            voxels;               // (w/(L+1) * h/(L+1)) bits for lod L >= 0, L < TG_MAX_LODS. Data is packed contiguous
    // TODO: implement below
    //tgvk_buffer            color_ids;            // 8 bits per voxel
    //u8                     p_color_lut[3 * 256]; // Three 8 bit components per color, 256 colors // TODO: optionally less colors, less memory. put LUT into one huge array and only reference pointer to location in here?
} tg_obj;

typedef struct tg_render_command
{
    tg_structure_type                    type;
    u32                                  renderer_info_count;
    tg_mesh_h                            h_mesh;
    tg_material_h                        h_material;
    tgvk_buffer                          model_ubo;
    tgvk_render_command_renderer_info    p_renderer_infos[TG_MAX_RENDERERS];
} tg_render_command;

typedef struct tg_render_target
{
    tg_structure_type    type;
    tgvk_image           color_attachment;
    tgvk_image           depth_attachment;
    tgvk_image           color_attachment_copy;
    tgvk_image           depth_attachment_copy;
    VkFence              fence; // TODO: use 'timeline' semaphore?
} tg_render_target;

typedef struct tg_mesh
{
    tg_structure_type    type;

    tg_bounds            bounds; // TODO: these are not used (actually, in kd-tree...)! let it be a sphere (radius + position (position because mesh might not be perfectly centered)) for view frustum culling?
    
    u32                  index_count;
    u32                  vertex_count;
    
    tgvk_buffer          index_buffer;
    tgvk_buffer          position_buffer;
    tgvk_buffer          normal_buffer;
    tgvk_buffer          uv_buffer;
    tgvk_buffer          tangent_buffer;
    tgvk_buffer          bitangent_buffer;
} tg_mesh;

typedef struct tg_rtvx_terrain
{
    tgvk_image_3d    voxels;
} tg_rtvx_terrain;

typedef struct tg_storage_buffer
{
    tg_structure_type    type;
    tgvk_buffer          buffer;
} tg_storage_buffer;

typedef struct tg_storage_image_3d
{
    tg_structure_type    type;
    tgvk_image_3d        image_3d;
} tg_storage_image_3d;

typedef struct tg_uniform_buffer
{
    tg_structure_type    type;
    tgvk_buffer          buffer;
} tg_uniform_buffer;

typedef struct tg_vertex_shader
{
    tg_structure_type    type;
    tgvk_shader          shader;
} tg_vertex_shader;

typedef struct tgvk_atmosphere_model
{
    struct
    {
	    b32                          use_constant_solar_spectrum;
	    b32                          use_ozone;
	    b32                          use_combined_textures;
	    b32                          use_half_precision;
	    tgvk_atmosphere_luminance    use_luminance;
        u32                          precomputed_wavelength_count;
	    b32                          combine_scattering_textures;
    } settings;

    struct
    {
        u32                          capacity;
        u32                          header_count;
        u32                          total_count;
        char*                        p_source;
    } api_shader;

    struct
    {
	    tgvk_image                   transmittance_texture;
	    tgvk_layered_image           scattering_texture;
	    tgvk_layered_image           optional_single_mie_scattering_texture;
	    tgvk_layered_image           no_single_mie_scattering_black_texture; // TODO: remove this and change 'p_atmosphere_shader' to not use 'single_mie_scattering_texture'
	    tgvk_image                   irradiance_texture;
        tgvk_buffer                  vertex_shader_ubo;
        tgvk_buffer                  ubo;
    } rendering;
} tgvk_atmosphere_model;

typedef struct tg_ray_trace_command
{
    tgvk_image_3d          voxel_image;
    tgvk_buffer            model_ubo;
    tgvk_buffer            material_ubo;
    tgvk_descriptor_set    descriptor_set;
} tg_ray_trace_command;

typedef struct tg_ray_tracer
{
    tg_structure_type              type;

    const tg_camera*               p_camera;
    tgvk_buffer                    visibility_buffer;
    tgvk_image                     hdr_color_attachment;
    tg_render_target               render_target;
    VkSemaphore                    semaphore;



    struct
    {
        u32                        capacity;
        u32                        count;
        tg_ray_trace_command_h*    p_commands;
    } commands;

    struct
    {
        tgvk_command_buffer        command_buffer;
        tgvk_image                 p_color_attachments[TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT];
        tgvk_framebuffer           framebuffer;
    } geometry_pass;

    struct
    {
        tgvk_command_buffer        command_buffer;
        tgvk_shader                fragment_shader;
        tgvk_pipeline              graphics_pipeline;
        tgvk_buffer                ubo;
        tgvk_descriptor_set        descriptor_set;
        tgvk_framebuffer           framebuffer;
    } shading_pass;

    struct
    {
        tgvk_command_buffer        command_buffer;
    } blit_pass;

    struct
    {
        tgvk_command_buffer        p_command_buffers[TG_MAX_SWAPCHAIN_IMAGES];
        VkSemaphore                image_acquired_semaphore;
        tgvk_framebuffer           p_framebuffers[TG_MAX_SWAPCHAIN_IMAGES];
        tgvk_pipeline              graphics_pipeline;
        tgvk_descriptor_set        descriptor_set;
    } present_pass;

    struct
    {
        tgvk_command_buffer        command_buffer;
    } clear_pass;
} tg_ray_tracer;

typedef struct tg_renderer
{
    tg_structure_type            type;

    const tg_camera*             p_camera;
    tgvk_buffer                  view_projection_ubo; // TODO: move to g buffer pass
    tgvk_image                   hdr_color_attachment;
    tg_render_target             render_target;
    VkSemaphore                  semaphore;

    u32                          deferred_command_buffer_count;
    u32                          forward_render_command_count;
    VkCommandBuffer              p_deferred_command_buffers[TG_MAX_RENDER_COMMANDS];
    tg_render_command_h          ph_forward_render_commands[TG_MAX_RENDER_COMMANDS];
    tg_rtvx_terrain_h            h_terrain;

    struct
    {
        tg_font_h                h_font; // TODO: tg_font and have an internal creator function for the font
        u32                      capacity;
        u32                      count;
        u32                      total_letter_count;
        u32*                     p_string_capacities;
        char**                   pp_strings;
    } text;

    tgvk_atmosphere_model        model;

    struct
    {
        tgvk_command_buffer      command_buffer;
        tgvk_image               p_color_attachments[TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT];
        tgvk_framebuffer         framebuffer;
    } geometry_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
        tgvk_framebuffer         framebuffer;
        tgvk_shader              fragment_shader;
        tgvk_pipeline            graphics_pipeline;
        tgvk_descriptor_set      descriptor_set;
        tgvk_buffer              ubo;
    } shading_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
        tgvk_framebuffer         framebuffer;
    } forward_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
        tgvk_shader              shader;
        tgvk_pipeline            pipeline;
        tgvk_descriptor_set      descriptor_set;
        tgvk_buffer              ubo;
    } ray_trace_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;

        tgvk_buffer              acquire_exposure_clear_storage_buffer;
        tgvk_shader              acquire_exposure_compute_shader;
        tgvk_pipeline            acquire_exposure_compute_pipeline;
        tgvk_descriptor_set      acquire_exposure_descriptor_set;
        tgvk_buffer              acquire_exposure_storage_buffer;

        tgvk_shader              finalize_exposure_compute_shader;
        tgvk_pipeline            finalize_exposure_compute_pipeline;
        tgvk_descriptor_set      finalize_exposure_descriptor_set;
        tgvk_buffer              finalize_exposure_storage_buffer;
        tgvk_buffer              finalize_exposure_dt_ubo;
        
        tgvk_framebuffer         adapt_exposure_framebuffer;
        tgvk_pipeline            adapt_exposure_graphics_pipeline;
        tgvk_descriptor_set      adapt_exposure_descriptor_set;
    } tone_mapping_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
        tgvk_framebuffer         framebuffer;
        tgvk_pipeline            pipeline;
        tgvk_descriptor_set      descriptor_set;
    } ui_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
    } blit_pass;

    struct
    {
        tgvk_command_buffer      p_command_buffers[TG_MAX_SWAPCHAIN_IMAGES];
        VkSemaphore              image_acquired_semaphore;
        tgvk_framebuffer         p_framebuffers[TG_MAX_SWAPCHAIN_IMAGES];
        tgvk_pipeline            graphics_pipeline;
        tgvk_descriptor_set      descriptor_set;
    } present_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
    } clear_pass;

#if TG_ENABLE_DEBUG_TOOLS == 1
#define TG_DEBUG_MAX_CUBES 2097152
    struct
    {
        u32                        cube_count;
        tgvk_buffer                cube_index_buffer;
        tgvk_buffer                cube_position_buffer;
        struct
        {
            tgvk_pipeline          graphics_pipeline;
            tgvk_descriptor_set    descriptor_set;
            tgvk_buffer            ubo;
            tgvk_command_buffer    command_buffer;
        } *p_cubes;
    } DEBUG;
#endif
} tg_renderer;



VkInstance                      instance;
tgvk_surface                    surface;
VkPhysicalDevice                physical_device;
VkDevice                        device;

VkSwapchainKHR                  swapchain;
VkExtent2D                      swapchain_extent;
VkImage                         p_swapchain_images[TG_MAX_SWAPCHAIN_IMAGES];
VkImageView                     p_swapchain_image_views[TG_MAX_SWAPCHAIN_IMAGES];
tgvk_shared_render_resources    shared_render_resources;



void                    tgvk_atmosphere_model_create(TG_OUT tgvk_atmosphere_model* p_model);
void                    tgvk_atmosphere_model_destroy(tgvk_atmosphere_model* p_model);
void                    tgvk_atmosphere_model_update_descriptor_set(tgvk_atmosphere_model* p_model, tgvk_descriptor_set* p_descriptor_set);
void                    tgvk_atmosphere_model_update(tgvk_atmosphere_model* p_model, m4 inv_view, m4 inv_proj);

void                    tgvk_buffer_copy(VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
tgvk_buffer             tgvk_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, tgvk_memory_type type TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_buffer_destroy(tgvk_buffer* p_buffer);

void                    tgvk_cmd_begin_render_pass(tgvk_command_buffer* p_command_buffer, VkRenderPass render_pass, tgvk_framebuffer* p_framebuffer, VkSubpassContents subpass_contents);
void                    tgvk_cmd_bind_and_draw_screen_quad(tgvk_command_buffer* p_command_buffer);
void                    tgvk_cmd_bind_descriptor_set(tgvk_command_buffer* p_command_buffer, tgvk_pipeline* p_pipeline, tgvk_descriptor_set* p_descriptor_set);
void                    tgvk_cmd_bind_index_buffer(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_buffer);
void                    tgvk_cmd_bind_pipeline(tgvk_command_buffer* p_command_buffer, tgvk_pipeline* p_pipeline);
void                    tgvk_cmd_bind_vertex_buffer(tgvk_command_buffer* p_command_buffer, u32 binding, const tgvk_buffer* p_buffer);
void                    tgvk_cmd_blit_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination, const VkImageBlit* p_region);
void                    tgvk_cmd_blit_image_3d_slice_to_image(tgvk_command_buffer* p_command_buffer, u32 slice_depth, tgvk_image_3d* p_source, tgvk_image* p_destination, const VkImageBlit* p_region);
void                    tgvk_cmd_blit_layered_image_layer_to_image(tgvk_command_buffer* p_command_buffer, u32 layer, tgvk_layered_image* p_source, tgvk_image* p_destination, const VkImageBlit* p_region);
void                    tgvk_cmd_clear_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image);
void                    tgvk_cmd_clear_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d);
void                    tgvk_cmd_clear_layered_image(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image);
void                    tgvk_cmd_copy_buffer(tgvk_command_buffer* p_command_buffer, VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
void                    tgvk_cmd_copy_buffer_to_cube_map(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_cube_map* p_destination);
void                    tgvk_cmd_copy_buffer_to_image(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_buffer_to_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image_3d* p_destination);
void                    tgvk_cmd_copy_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_color_image_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination);
void                    tgvk_cmd_copy_depth_image_pixel_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination, u32 x, u32 y);
void                    tgvk_cmd_copy_image_3d_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_source, tgvk_buffer* p_destination);
void                    tgvk_cmd_draw_indexed(tgvk_command_buffer* p_command_buffer, u32 index_count);
void                    tgvk_cmd_transition_cube_map_layout(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, tgvk_layout_type src_type, tgvk_layout_type dst_type);
void                    tgvk_cmd_transition_cube_map_layout2(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, tgvk_layout_type src_type, tgvk_layout_type dst_type);
void                    tgvk_cmd_transition_image_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_3d_layout(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, tgvk_layout_type src_type, tgvk_layout_type dst_type);
void                    tgvk_cmd_transition_image_3d_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_layered_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, tgvk_layout_type src_type, tgvk_layout_type dst_type);
void                    tgvk_cmd_transition_layered_image_layout2(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);

tgvk_command_buffer*    tgvk_command_buffer_get_and_begin_global(tgvk_command_pool_type type);
tgvk_command_buffer*    tgvk_command_buffer_get_global(tgvk_command_pool_type type);
tgvk_command_buffer     tgvk_command_buffer_create(tgvk_command_pool_type type, VkCommandBufferLevel level);
void                    tgvk_command_buffers_create(tgvk_command_pool_type type, VkCommandBufferLevel level, u32 count, tgvk_command_buffer* p_command_buffers);
void                    tgvk_command_buffer_destroy(tgvk_command_buffer* p_command_buffer);
void                    tgvk_command_buffers_destroy(u32 count, tgvk_command_buffer* p_command_buffers);
void                    tgvk_command_buffer_begin(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags);
void                    tgvk_command_buffer_begin_secondary(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags, VkRenderPass render_pass, tgvk_framebuffer* p_framebuffer);
void                    tgvk_command_buffer_end_and_submit(tgvk_command_buffer* p_command_buffer);

tgvk_cube_map           tgvk_cube_map_create(u32 dimension, VkFormat format, const tg_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_cube_map_destroy(tgvk_cube_map* p_cube_map);

tgvk_descriptor_set     tgvk_descriptor_set_create(const tgvk_pipeline* p_pipeline);
void                    tgvk_descriptor_set_destroy(tgvk_descriptor_set* p_descriptor_set);

void                    tgvk_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle shader_input_element_handle, u32 dst_binding);
void                    tgvk_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tgvk_cube_map* p_cube_map, u32 dst_binding);
void                    tgvk_descriptor_set_update_image(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding);
void                    tgvk_descriptor_set_update_image2(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, VkDescriptorType descriptor_type);
void                    tgvk_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_image_3d(VkDescriptorSet descriptor_set, tgvk_image_3d* p_image_3d, u32 dst_binding);
void                    tgvk_descriptor_set_update_layered_image(VkDescriptorSet descriptor_set, tgvk_layered_image* p_image, u32 dst_binding);
void                    tgvk_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding);
void                    tgvk_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding);
void                    tgvk_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding);
void                    tgvk_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_uniform_buffer_range(VkDescriptorSet descriptor_set, VkDeviceSize offset, VkDeviceSize range, tgvk_buffer* p_buffer, u32 dst_binding);
void                    tgvk_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                 tgvk_fence_create(VkFenceCreateFlags fence_create_flags);
void                    tgvk_fence_destroy(VkFence fence);
void                    tgvk_fence_reset(VkFence fence);
void                    tgvk_fence_wait(VkFence fence);

tgvk_framebuffer        tgvk_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
tgvk_framebuffer        tgvk_framebuffer_create_layered(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height, u32 layers);
void                    tgvk_framebuffer_destroy(tgvk_framebuffer* p_framebuffer);
void                    tgvk_framebuffers_destroy(u32 count, tgvk_framebuffer* p_framebuffers);

b32                     tgvk_get_physical_device_image_format_properties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, TG_OUT VkImageFormatProperties* p_image_format_properties);

tgvk_buffer*            tgvk_global_staging_buffer_take(VkDeviceSize size);
void                    tgvk_global_staging_buffer_release(void);

void*                   tgvk_handle_array(tg_structure_type type);
void*                   tgvk_handle_take(tg_structure_type type);
void                    tgvk_handle_release(void* p_handle);

tgvk_image              tgvk_image_create(tgvk_image_type_flags type_flags, u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
tgvk_image              tgvk_image_create2(tgvk_image_type type, const char* p_filename, const tg_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2));
void                    tgvk_image_destroy(tgvk_image* p_image);
b32                     tgvk_image_serialize(tgvk_image* p_image, const char* p_filename);
b32                     tgvk_image_deserialize(const char* p_filename, TG_OUT tgvk_image* p_image TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2));
b32                     tgvk_image_store_to_disc(tgvk_image* p_image, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_image_3d           tgvk_image_3d_create(tgvk_image_type type, u32 width, u32 height, u32 depth, VkFormat format, const tg_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_image_3d_destroy(tgvk_image_3d* p_image_3d);
b32                     tgvk_image_3d_store_slice_to_disc(tgvk_image_3d* p_image_3d, u32 slice_depth, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_layered_image      tgvk_layered_image_create(tgvk_image_type type, u32 width, u32 height, u32 layers, VkFormat format, const tg_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_layered_image_destroy(tgvk_layered_image* p_image);
b32                     tgvk_layered_image_store_layer_to_disc(tgvk_layered_image* p_image, u32 layer, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_pipeline           tgvk_pipeline_create_compute(const tgvk_shader* p_compute_shader);
tgvk_pipeline           tgvk_pipeline_create_graphics(const tgvk_graphics_pipeline_create_info* p_create_info);
void                    tgvk_pipeline_destroy(tgvk_pipeline* p_pipeline);

void                    tgvk_queue_present(VkPresentInfoKHR* p_present_info);
void                    tgvk_queue_submit(tgvk_queue_type type, u32 submit_count, VkSubmitInfo* p_submit_infos, VkFence fence);
void                    tgvk_queue_wait_idle(tgvk_queue_type type);

VkRenderPass            tgvk_render_pass_create(const VkAttachmentDescription* p_attachment_description, const VkSubpassDescription* p_subpass_description);
void                    tgvk_render_pass_destroy(VkRenderPass render_pass);
tg_render_target        tgvk_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tg_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tg_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_render_target_destroy(tg_render_target* p_render_target);

void                    tgvk_renderer_on_window_resize(tg_renderer_h h_renderer, u32 width, u32 height);

tgvk_sampler            tgvk_sampler_create(void);
tgvk_sampler            tgvk_sampler_create2(const tg_sampler_create_info* p_sampler_create_info);
void                    tgvk_sampler_destroy(tgvk_sampler* p_sampler);

VkSemaphore             tgvk_semaphore_create(void);
void                    tgvk_semaphore_destroy(VkSemaphore semaphore);

tgvk_shader             tgvk_shader_create(const char* p_filename);
tgvk_shader             tgvk_shader_create_from_glsl(tg_shader_type type, const char* p_source);
tgvk_shader             tgvk_shader_create_from_spirv(u32 size, const char* p_source);
void                    tgvk_shader_destroy(tgvk_shader* p_shader);

VkDescriptorType        tgvk_structure_type_convert_to_descriptor_type(tg_structure_type type);

tgvk_buffer             tgvk_uniform_buffer_create(VkDeviceSize size TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));

#endif

#endif
