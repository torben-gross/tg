#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include "graphics/tg_spirv.h"
#include "graphics/vulkan/tg_vulkan_memory_allocator.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"

#pragma warning(push, 3)
#include <vulkan/vulkan.h>
#pragma warning(pop)

#ifdef TG_WIN32
#undef near
#undef far
#undef min
#undef max
#endif



#ifdef TG_DEBUG
#define TGVK_CALL_NAME2(x, y)    x ## y
#define TGVK_CALL_NAME(x, y)     TGVK_CALL_NAME2(x, y)
#define TGVK_CALL(x)                                                                                                   \
    const VkResult TGVK_CALL_NAME(vk_call_result, __LINE__) = (x);                                                     \
    if (TGVK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS)                                                        \
    {                                                                                                                  \
        char TGVK_CALL_NAME(p_buffer, __LINE__)[1024] = { 0 };                                                         \
        tgvk_vkresult_convert_to_string(TGVK_CALL_NAME(p_buffer, __LINE__), TGVK_CALL_NAME(vk_call_result, __LINE__)); \
        TG_DEBUG_LOG("VkResult: %s\n", TGVK_CALL_NAME(p_buffer, __LINE__));                                            \
    }                                                                                                                  \
    if (TGVK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS) TG_INVALID_CODEPATH()

#else
#define TGVK_CALL(x) x
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
    TGVK_IMAGE_TYPE_COLOR,
    TGVK_IMAGE_TYPE_DEPTH,
    TGVK_IMAGE_TYPE_STORAGE
} tgvk_image_type;

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
    TGVK_QUEUE_TYPE_COUNT
} tgvk_queue_type;

typedef enum tgvk_geometry_attachment
{
    TGVK_GEOMETRY_ATTACHMENT_POSITION                 = 0,
    TGVK_GEOMETRY_ATTACHMENT_NORMAL                   = 1,
    TGVK_GEOMETRY_ATTACHMENT_ALBEDO                   = 2,
    TGVK_GEOMETRY_ATTACHMENT_METALLIC_ROUGHNESS_AO    = 3,
    TGVK_GEOMETRY_ATTACHMENT_DEPTH                    = 4,
    TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT              = TGVK_GEOMETRY_ATTACHMENT_METALLIC_ROUGHNESS_AO + 1,
    TGVK_GEOMETRY_ATTACHMENT_DEPTH_COUNT              = 1,
    TGVK_GEOMETRY_ATTACHMENT_COUNT                    = TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT + TGVK_GEOMETRY_ATTACHMENT_DEPTH_COUNT
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

typedef struct tgvk_cube_map
{
    u32                  dimension;
    VkFormat             format;
    VkImage              image;
    tgvk_memory_block    memory;
    VkImageView          image_view;
    VkSampler            sampler;
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
    VkSampler            sampler;
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
    VkSampler            sampler;
} tgvk_image_3d;

typedef struct tgvk_framebuffer
{
    u32              width;
    u32              height;
    u32              layers;
    VkFramebuffer    framebuffer;
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
    VkSampler            sampler;
} tgvk_layered_image;

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
    tgvk_descriptor_set    p_shadow_descriptor_sets[TG_CASCADED_SHADOW_MAPS];
    tgvk_command_buffer    p_shadow_command_buffers[TG_CASCADED_SHADOW_MAPS];
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

    VkRenderPass     shadow_render_pass;
    tgvk_pipeline    shadow_pipeline;
    VkRenderPass     geometry_render_pass;
    VkRenderPass     ssao_render_pass;
    VkRenderPass     ssao_blur_render_pass;
    VkRenderPass     shading_render_pass;
    VkRenderPass     tone_mapping_render_pass;
    VkRenderPass     forward_render_pass;
    VkRenderPass     atmosphere_render_pass;
    VkRenderPass     present_render_pass;
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
	    b32                          do_white_balance;
	    f64                          exposure;
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
        tgvk_shader                  vertex_shader;
        tgvk_shader                  fragment_shader;
        tgvk_framebuffer             framebuffer;
        tgvk_pipeline                graphics_pipeline;
        tgvk_descriptor_set          descriptor_set;
        tgvk_buffer                  vertex_shader_ubo;
        tgvk_buffer                  fragment_shader_ubo;
        tgvk_command_buffer          command_buffer;
    } rendering;
} tgvk_atmosphere_model;

typedef struct tg_renderer
{
    tg_structure_type            type;
    const tg_camera*             p_camera;
    tgvk_buffer                  view_projection_ubo;
    tgvk_image                   hdr_color_attachment;
    tg_render_target             render_target;
    VkSemaphore                  semaphore;

    u32                          deferred_command_buffer_count;
    u32                          shadow_command_buffer_count;
    u32                          forward_render_command_count;
    VkCommandBuffer              p_deferred_command_buffers[TG_MAX_RENDER_COMMANDS];
    VkCommandBuffer              p_shadow_command_buffers[TG_CASCADED_SHADOW_MAPS][TG_MAX_RENDER_COMMANDS];
    tg_render_command_h          ph_forward_render_commands[TG_MAX_RENDER_COMMANDS];

    struct
    {
        b32                      enabled;
        tgvk_image               p_shadow_maps[TG_CASCADED_SHADOW_MAPS];
        tgvk_framebuffer         p_framebuffers[TG_CASCADED_SHADOW_MAPS];
        tgvk_command_buffer      command_buffer;
        tgvk_buffer              p_lightspace_ubos[TG_CASCADED_SHADOW_MAPS];
    } shadow_pass;

    struct
    {
        tgvk_image               p_color_attachments[TGVK_GEOMETRY_ATTACHMENT_COLOR_COUNT];
        tgvk_framebuffer         framebuffer;
        tgvk_command_buffer      command_buffer;
    } geometry_pass;

    struct
    {
        tgvk_image               ssao_attachment;
        tgvk_framebuffer         ssao_framebuffer;
        tgvk_pipeline            ssao_graphics_pipeline;
        tgvk_descriptor_set      ssao_descriptor_set;
        tgvk_image               ssao_noise_image;
        tgvk_buffer              ssao_ubo;

        tgvk_image               blur_attachment;
        tgvk_framebuffer         blur_framebuffer;
        tgvk_pipeline            blur_graphics_pipeline;
        tgvk_descriptor_set      blur_descriptor_set;

        tgvk_command_buffer      command_buffer;
    } ssao_pass;

    struct
    {
        tgvk_framebuffer         framebuffer;
        tgvk_shader              fragment_shader;
        tgvk_pipeline            graphics_pipeline;
        tgvk_descriptor_set      descriptor_set;
        tgvk_command_buffer      command_buffer;
        tgvk_buffer              shading_info_ubo;
    } shading_pass;

    struct
    {
        tgvk_framebuffer         framebuffer;
        tgvk_command_buffer      command_buffer;
    } forward_pass;

    struct
    {
        tgvk_atmosphere_model    model;
    } atmosphere_pass;

    struct
    {
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
        tgvk_command_buffer      adapt_exposure_command_buffer;
    } tone_mapping_pass;

    struct
    {
        tgvk_command_buffer      command_buffer;
    } blit_pass;

    struct
    {
        VkSemaphore              image_acquired_semaphore;
        tgvk_framebuffer         p_framebuffers[TG_MAX_SWAPCHAIN_IMAGES];
        tgvk_pipeline            graphics_pipeline;
        tgvk_descriptor_set      descriptor_set;
        tgvk_command_buffer      p_command_buffers[TG_MAX_SWAPCHAIN_IMAGES];
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



void                    tgvk_atmosphere_model_create(tgvk_image* p_color_attachment, tgvk_image* p_depth_attachment, TG_OUT tgvk_atmosphere_model* p_model);
void                    tgvk_atmosphere_model_destroy(tgvk_atmosphere_model* p_model);
void                    tgvk_atmosphere_model_update(tgvk_atmosphere_model* p_model, v3 sun_direction, m4 inv_view, m4 inv_proj);

void                    tgvk_buffer_copy(VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
tgvk_buffer             tgvk_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                    tgvk_buffer_destroy(tgvk_buffer* p_buffer);
void                    tgvk_buffer_flush_device_to_host(tgvk_buffer* p_buffer);
void                    tgvk_buffer_flush_device_to_host_range(tgvk_buffer* p_buffer, VkDeviceSize offset, VkDeviceSize size);
void                    tgvk_buffer_flush_host_to_device(tgvk_buffer* p_buffer);
void                    tgvk_buffer_flush_host_to_device_range(tgvk_buffer* p_buffer, VkDeviceSize offset, VkDeviceSize size);

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
void                    tgvk_cmd_copy_buffer_to_color_image(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_buffer_to_cube_map(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_cube_map* p_destination);
void                    tgvk_cmd_copy_buffer_to_depth_image(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_buffer_to_image_3d(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image_3d* p_destination);
void                    tgvk_cmd_copy_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_color_image_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, VkBuffer destination);
void                    tgvk_cmd_copy_depth_image_pixel_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, VkBuffer destination, u32 x, u32 y);
void                    tgvk_cmd_copy_image_3d_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_source, VkBuffer destination);
void                    tgvk_cmd_draw_indexed(tgvk_command_buffer* p_command_buffer, u32 index_count);
void                    tgvk_cmd_transition_cube_map_layout(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_3d_layout(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_layered_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);

tgvk_command_buffer*    tgvk_command_buffer_get_global(tgvk_command_pool_type type);
tgvk_command_buffer     tgvk_command_buffer_create(tgvk_command_pool_type type, VkCommandBufferLevel level);
void                    tgvk_command_buffers_create(tgvk_command_pool_type type, VkCommandBufferLevel level, u32 count, tgvk_command_buffer* p_command_buffers);
void                    tgvk_command_buffer_destroy(tgvk_command_buffer* p_command_buffer);
void                    tgvk_command_buffers_destroy(u32 count, tgvk_command_buffer* p_command_buffers);
void                    tgvk_command_buffer_begin(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags);
void                    tgvk_command_buffer_begin_secondary(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags, VkRenderPass render_pass, VkFramebuffer framebuffer);
void                    tgvk_command_buffer_end_and_submit(tgvk_command_buffer* p_command_buffer);

tgvk_cube_map           tgvk_cube_map_create(u32 dimension, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
void                    tgvk_cube_map_destroy(tgvk_cube_map* p_cube_map);

tgvk_descriptor_set     tgvk_descriptor_set_create(const tgvk_pipeline* p_pipeline);
void                    tgvk_descriptor_set_destroy(tgvk_descriptor_set* p_descriptor_set);

void                    tgvk_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle shader_input_element_handle, u32 dst_binding);
void                    tgvk_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tgvk_cube_map* p_cube_map, u32 dst_binding);
void                    tgvk_descriptor_set_update_image(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding);
void                    tgvk_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_image_3d(VkDescriptorSet descriptor_set, tgvk_image_3d* p_image_3d, u32 dst_binding);
void                    tgvk_descriptor_set_update_layered_image(VkDescriptorSet descriptor_set, tgvk_layered_image* p_image, u32 dst_binding);
void                    tgvk_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding);
void                    tgvk_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                    tgvk_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                    tgvk_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_uniform_buffer_range(VkDescriptorSet descriptor_set, VkDeviceSize offset, VkDeviceSize range, VkBuffer buffer, u32 dst_binding);
void                    tgvk_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                 tgvk_fence_create(VkFenceCreateFlags fence_create_flags);
void                    tgvk_fence_destroy(VkFence fence);
void                    tgvk_fence_reset(VkFence fence);
void                    tgvk_fence_wait(VkFence fence);

tgvk_framebuffer        tgvk_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
tgvk_framebuffer        tgvk_framebuffer_create_layered(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height, u32 layers);
void                    tgvk_framebuffer_destroy(tgvk_framebuffer* p_framebuffer);
void                    tgvk_framebuffers_destroy(u32 count, tgvk_framebuffer* p_framebuffers);

void                    tgvk_get_physical_device_format_properties(VkFormat format, TG_OUT VkFormatProperties* p_format_properties);

tgvk_buffer*            tgvk_global_staging_buffer_take(VkDeviceSize size);
void                    tgvk_global_staging_buffer_release(void);

void*                   tgvk_handle_array(tg_structure_type type);
void*                   tgvk_handle_take(tg_structure_type type);
void                    tgvk_handle_release(void* p_handle);

tgvk_image              tgvk_image_create(tgvk_image_type type, u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
tgvk_image              tgvk_image_create2(tgvk_image_type type, const char* p_filename, const tg_sampler_create_info* p_sampler_create_info);
void                    tgvk_image_destroy(tgvk_image* p_image);
b32                     tgvk_image_store_to_disc(tgvk_image* p_image, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_image_3d           tgvk_image_3d_create(tgvk_image_type type, u32 width, u32 height, u32 depth, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
void                    tgvk_image_3d_destroy(tgvk_image_3d* p_image_3d);
b32                     tgvk_image_3d_store_slice_to_disc(tgvk_image_3d* p_image_3d, u32 slice_depth, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_layered_image      tgvk_layered_image_create(tgvk_image_type type, u32 width, u32 height, u32 layers, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
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
tg_render_target        tgvk_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tg_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tg_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags);
void                    tgvk_render_target_destroy(tg_render_target* p_render_target);

void                    tgvk_renderer_on_window_resize(tg_renderer_h h_renderer, u32 width, u32 height);

VkSemaphore             tgvk_semaphore_create(void);
void                    tgvk_semaphore_destroy(VkSemaphore semaphore);

tgvk_shader             tgvk_shader_create(const char* p_filename);
tgvk_shader             tgvk_shader_create_from_glsl(tg_shader_type type, const char* p_source);
tgvk_shader             tgvk_shader_create_from_spirv(u32 size, const char* p_source);
void                    tgvk_shader_destroy(tgvk_shader* p_shader);

VkDescriptorType        tgvk_structure_type_convert_to_descriptor_type(tg_structure_type type);

void                    tgvk_vkresult_convert_to_string(char* p_buffer, VkResult result);

#endif

#endif
