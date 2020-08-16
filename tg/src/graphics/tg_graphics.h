#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "math/tg_math.h"
#include "tg_common.h"


#define TG_MAX_COMPUTE_SHADERS            16
#define TG_MAX_FRAGMENT_SHADERS           32
#define TG_MAX_MATERIALS                  512
#define TG_MAX_MESHES                     65536
#define TG_MAX_RENDER_COMMANDS            65536
#define TG_MAX_RAYTRACERS                 2
#define TG_MAX_RENDERERS                  4
#define TG_MAX_VERTEX_SHADERS             32

#define TG_MAX_SHADER_ATTACHMENTS         8
#define TG_MAX_SHADER_GLOBAL_RESOURCES    32
#define TG_MAX_SHADER_INPUTS              32
#define TG_SHADER_RESERVED_BINDINGS       2

#define TG_CASCADED_SHADOW_MAPS           3
#define TG_CASCADED_SHADOW_MAP_SIZE       1024

#define TG_SSAO_MAP_SIZE                  512

#define TG_MAX_DIRECTIONAL_LIGHTS         512
#define TG_MAX_POINT_LIGHTS               512

#define TG_IMAGE_MAX_MIP_LEVELS(w, h)     ((u32)tgm_f32_log2((f32)tgm_u32_max((u32)w, (u32)h)) + 1)

#define TG_CAMERA_LEFT(camera)        (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){ -1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_RIGHT(camera)       (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_DOWN(camera)        (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f, -1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_UP(camera)          (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_FORWARD(camera)     (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f, -1.0f,  0.0f }).xyz)
#define TG_CAMERA_BACKWARD(camera)    (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f,  1.0f,  0.0f }).xyz)



TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_raytracer);
TG_DECLARE_HANDLE(tg_render_command);
TG_DECLARE_HANDLE(tg_render_target);
TG_DECLARE_HANDLE(tg_renderer);
TG_DECLARE_HANDLE(tg_storage_buffer);
TG_DECLARE_HANDLE(tg_vertex_shader);

typedef void* tg_handle;



typedef enum tg_camera_type
{
	TG_CAMERA_TYPE_ORTHOGRAPHIC,
	TG_CAMERA_TYPE_PERSPECTIVE
} tg_camera_type;

typedef enum tg_color_image_format
{
	TG_COLOR_IMAGE_FORMAT_A8B8G8R8               = 57,
	TG_COLOR_IMAGE_FORMAT_B8G8R8A8               = 50,
	TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT    = 97,
	TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT    = 109,
	TG_COLOR_IMAGE_FORMAT_R8                     = 9,
	TG_COLOR_IMAGE_FORMAT_R8G8                   = 22,
	TG_COLOR_IMAGE_FORMAT_R8G8B8                 = 29,
	TG_COLOR_IMAGE_FORMAT_R8G8B8A8               = 43
} tg_color_image_format;

typedef enum tg_depth_image_format
{
	TG_DEPTH_IMAGE_FORMAT_D16_UNORM     = 124,
	TG_DEPTH_IMAGE_FORMAT_D32_SFLOAT    = 126
} tg_depth_image_format;

typedef enum tg_structure_type
{
	TG_STRUCTURE_TYPE_INVALID = 0,
	TG_STRUCTURE_TYPE_STORAGE_BUFFER,
	TG_STRUCTURE_TYPE_COLOR_IMAGE,
	TG_STRUCTURE_TYPE_COLOR_IMAGE_3D,
	TG_STRUCTURE_TYPE_COMPUTE_SHADER,
    TG_STRUCTURE_TYPE_CUBE_MAP,
	TG_STRUCTURE_TYPE_DEPTH_IMAGE,
	TG_STRUCTURE_TYPE_FRAGMENT_SHADER,
	TG_STRUCTURE_TYPE_MATERIAL,
	TG_STRUCTURE_TYPE_MESH,
	TG_STRUCTURE_TYPE_INDEX_BUFFER,
	TG_STRUCTURE_TYPE_RAYTRACER,
	TG_STRUCTURE_TYPE_RENDER_COMMAND,
	TG_STRUCTURE_TYPE_RENDER_TARGET,
	TG_STRUCTURE_TYPE_RENDERER,
	TG_STRUCTURE_TYPE_UNIFORM_BUFFER,
	TG_STRUCTURE_TYPE_VERTEX_BUFFER,
	TG_STRUCTURE_TYPE_VERTEX_SHADER
} tg_structure_type;

typedef enum tg_image_address_mode
{
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER         = 3,
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE           = 2,
	TG_IMAGE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE    = 4,
	TG_IMAGE_ADDRESS_MODE_MIRRORED_REPEAT         = 1,
	TG_IMAGE_ADDRESS_MODE_REPEAT                  = 0
} tg_image_address_mode;

typedef enum tg_image_filter
{
	TG_IMAGE_FILTER_LINEAR     = 1,
	TG_IMAGE_FILTER_NEAREST    = 0
} tg_image_filter;

typedef enum tg_vertex_input_attribute_format
{
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_INVALID                = 0,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT             = 100,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT               = 99,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT               = 98,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT          = 103,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT            = 102,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT            = 101,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT       = 106,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT         = 105,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT         = 104,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT    = 109,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT      = 108,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT      = 107
} tg_vertex_input_attribute_format;



typedef struct tg_bounds // TODO: this should be part of physics
{
	v3    min;
	v3    max;
} tg_bounds;

typedef struct tg_camera
{
	tg_camera_type    type;
	v3                position;
	f32               pitch;
	f32               yaw;
	f32               roll;
	union
	{
		struct
		{
			f32       left;
			f32       right;
			f32       bottom;
			f32       top;
			f32       far;
			f32       near;
		} orthographic;
		struct
		{
			f32       fov_y_in_radians;
			f32       aspect;
			f32       near;
			f32       far;
		} perspective;
	};
} tg_camera;

typedef struct tg_sampler_create_info
{
	tg_image_filter          min_filter;
	tg_image_filter          mag_filter;
	tg_image_address_mode    address_mode_u;
	tg_image_address_mode    address_mode_v;
	tg_image_address_mode    address_mode_w;
} tg_sampler_create_info;

typedef struct tg_kd_node tg_kd_node;
typedef struct tg_kd_node
{
	u32            flags; // 000 := leaf, 001 := x-axis, 010 := y-axis, 100 := z-axis
	union
	{
		struct
		{
			f32    split_position;
			u32    p_child_indices[2];
		} node;
		struct
		{
			u32    first_index_offset;
			u32    index_count;
		} leaf;
	};
} tg_kd_node;

typedef struct tg_mesh tg_mesh;
typedef struct tg_kd_tree
{
	const tg_mesh*    p_mesh;
	u32               index_capacity;
	u32               index_count;
	u32               node_count;
	u32*              p_indices;
	tg_kd_node        p_nodes[0];
} tg_kd_tree;

typedef struct tg_vertex
{
	v3    position;
	v3    normal;
	v2    uv;
	v3    tangent;
	v3    bitangent;
} tg_vertex;





#ifdef TG_VULKAN



// TODO: these probably correspond to the ones in graphics.h
#define TG_SPIRV_MAX_NAME    256



typedef enum tg_spirv_global_resource_type
{
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_INVALID                   = 0,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER    = 1,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_UNIFORM_BUFFER            = 6,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_BUFFER            = 7,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_IMAGE             = 3
} tg_spirv_global_resource_type;

typedef enum tg_spirv_shader_type
{
    TG_SPIRV_SHADER_TYPE_COMPUTE     = 32,
    TG_SPIRV_SHADER_TYPE_FRAGMENT    = 16,
    TG_SPIRV_SHADER_TYPE_VERTEX      = 1
} tg_spirv_shader_type;



typedef struct tg_spirv_global_resource
{
    tg_spirv_global_resource_type    type;
    u32                              array_element_count; // 0 if this is not an array
    u32                              descriptor_set;
    u32                              binding;
} tg_spirv_global_resource;

typedef struct tg_spirv_inout_resource
{
    u32                                 location;
    tg_vertex_input_attribute_format    format;
    u32                                 offset;
} tg_spirv_inout_resource;

typedef struct tg_spirv_layout
{
    tg_spirv_shader_type        shader_type;
    char                        p_entry_point_name[TG_SPIRV_MAX_NAME];
    u8                          input_resource_count;
    u8                          global_resource_count;
    u8                          output_resource_count;
    u32                         vertex_stride;
    tg_spirv_inout_resource     p_input_resources[TG_MAX_SHADER_INPUTS];
    tg_spirv_global_resource    p_global_resources[TG_MAX_SHADER_GLOBAL_RESOURCES];
    tg_spirv_inout_resource     p_output_resources[TG_MAX_SHADER_INPUTS];
} tg_spirv_layout;



void tg_spirv_fill_layout(u32 word_count, const u32* p_words, tg_spirv_layout* p_spirv_layout);





#include "graphics/vulkan/tg_vulkan_memory_allocator.h"
#include "platform/tg_platform.h"
#include <vulkan/vulkan.h>

#ifdef TG_WIN32
#undef near
#undef far
#undef min
#undef max
#endif



#ifdef TG_DEBUG
#define VK_CALL_NAME2(x, y) x ## y
#define VK_CALL_NAME(x, y) VK_CALL_NAME2(x, y)

#define VK_CALL(x) \
    const VkResult VK_CALL_NAME(vk_call_result, __LINE__) = (x); \
    if (VK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS) \
    { \
        char p_buffer[1024] = { 0 }; \
        tg_vulkan_vkresult_convert_to_string(p_buffer, VK_CALL_NAME(vk_call_result, __LINE__)); \
        TG_DEBUG_LOG("VkResult: %s\n", p_buffer); \
    } \
    if (VK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS) TG_INVALID_CODEPATH();

#else
#define VK_CALL(x) x
#endif

#define TG_VULKAN_SURFACE_IMAGE_COUNT    2
#define TG_VULKAN_MAX_LOD_COUNT     8



#define TG_VULKAN_TAKE_HANDLE(p_array, p_result)                                   \
    const u32 array_element_count = sizeof(p_array) / sizeof(*(p_array));            \
    for (u32 handle_index = 0; handle_index < array_element_count; handle_index++) \
    {                                                                              \
        if ((p_array)[handle_index].type == TG_STRUCTURE_TYPE_INVALID)                  \
        {                                                                          \
            (p_result) = &(p_array)[handle_index];                                     \
            break;                                                                 \
        }                                                                          \
    }                                                                              \
    TG_ASSERT(p_result)

#define TG_VULKAN_RELEASE_HANDLE(handle)                             TG_ASSERT((handle) && (handle)->type != TG_STRUCTURE_TYPE_INVALID); (handle)->type = TG_STRUCTURE_TYPE_INVALID

#define TG_MESH_POSITIONS(mesh) ((mesh).p_positions)



typedef enum tg_vulkan_command_pool_type
{
    TG_VULKAN_COMMAND_POOL_TYPE_COMPUTE,
    TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS
} tg_vulkan_command_pool_type;

typedef enum tg_vulkan_material_type
{
    TG_VULKAN_MATERIAL_TYPE_DEFERRED,
    TG_VULKAN_MATERIAL_TYPE_FORWARD
} tg_vulkan_material_type;

typedef enum tg_vulkan_queue_type
{
    TG_VULKAN_QUEUE_TYPE_COMPUTE,
    TG_VULKAN_QUEUE_TYPE_GRAPHICS,
    TG_VULKAN_QUEUE_TYPE_PRESENT,

    TG_VULKAN_QUEUE_TYPE_COUNT
} tg_vulkan_queue_type;

typedef enum tg_deferred_geometry_color_attachment
{
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_POSITION,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_NORMAL,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_ALBEDO,
    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_METALLIC_ROUGHNESS_AO,

    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT
} tg_deferred_geometry_color_attachment;



typedef struct tg_vulkan_buffer
{
    VkBuffer                  buffer;
    tg_vulkan_memory_block    memory;
} tg_vulkan_buffer;

typedef struct tg_vulkan_cube_map
{
    u32                       dimension;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
    VkSampler                 sampler;
} tg_vulkan_cube_map;

typedef struct tg_vulkan_image
{
    u32                       width;
    u32                       height;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
    VkSampler                 sampler;
} tg_vulkan_image;

typedef struct tg_vulkan_image_3d
{
    u32                       width;
    u32                       height;
    u32                       depth;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
    VkSampler                 sampler;
} tg_vulkan_image_3d;

typedef struct tg_vulkan_framebuffer
{
    u32              width;
    u32              height;
    VkFramebuffer    framebuffer;
} tg_vulkan_framebuffer;

typedef struct tg_vulkan_shader
{
    tg_spirv_layout    spirv_layout;
    VkShaderModule     shader_module;
} tg_vulkan_shader;

typedef struct tg_vulkan_graphics_pipeline_create_info
{
    const tg_vulkan_shader*                  p_vertex_shader;
    const tg_vulkan_shader*                  p_fragment_shader;
    VkCullModeFlagBits                       cull_mode;
    VkSampleCountFlagBits                    sample_count;
    VkBool32                                 depth_test_enable;
    VkBool32                                 depth_write_enable;
    VkBool32                                 blend_enable;
    VkRenderPass                             render_pass;
    v2                                       viewport_size;
} tg_vulkan_graphics_pipeline_create_info;

typedef struct tg_vulkan_image_extent
{
    f32    left;
    f32    bottom;
    f32    right;
    f32    top;
} tg_vulkan_image_extent;

typedef struct tg_vulkan_pipeline
{
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;
} tg_vulkan_pipeline;

typedef struct tg_vulkan_queue
{
    tg_mutex_h    h_mutex;
    u32           index;
    VkQueue       queue;
} tg_vulkan_queue;

typedef struct tg_vulkan_render_command_renderer_info
{
    tg_renderer_h         h_renderer;
    tg_vulkan_pipeline    graphics_pipeline;
    VkCommandBuffer       command_buffer;
    tg_vulkan_pipeline    p_shadow_graphics_pipelines[TG_CASCADED_SHADOW_MAPS];
    VkCommandBuffer       p_shadow_command_buffers[TG_CASCADED_SHADOW_MAPS];
} tg_vulkan_render_command_renderer_info;

typedef struct tg_vulkan_screen_vertex
{
    v2    position;
    v2    uv;
} tg_vulkan_screen_vertex;

typedef struct tg_vulkan_surface
{
    VkSurfaceKHR          surface;
    VkSurfaceFormatKHR    format;
} tg_vulkan_surface;



typedef struct tg_color_image
{
    tg_structure_type    type;
    tg_vulkan_image      vulkan_image;
} tg_color_image;

typedef struct tg_color_image_3d
{
    tg_structure_type     type;
    tg_vulkan_image_3d    vulkan_image_3d;
} tg_color_image_3d;

typedef struct tg_compute_shader
{
    tg_structure_type     type;
    tg_vulkan_shader      vulkan_shader;
    tg_vulkan_pipeline    compute_pipeline;
    VkCommandBuffer       command_buffer;
} tg_compute_shader;

typedef struct tg_cube_map
{
    tg_structure_type     type;
    tg_vulkan_cube_map    vulkan_cube_map;
} tg_cube_map;

typedef struct tg_depth_image
{
    tg_structure_type    type;
    tg_vulkan_image      vulkan_image;
} tg_depth_image;

typedef struct tg_render_command
{
    tg_structure_type                         type;
    tg_mesh*                                  p_mesh;
    tg_material_h                             h_material;
    tg_vulkan_buffer                          model_ubo;
    u32                                       renderer_info_count;
    tg_vulkan_render_command_renderer_info    p_renderer_infos[TG_MAX_RENDERERS];
} tg_render_command;

typedef struct tg_render_target
{
    tg_structure_type    type;
    tg_vulkan_image      color_attachment;
    tg_vulkan_image      depth_attachment;
    tg_vulkan_image      color_attachment_copy;
    tg_vulkan_image      depth_attachment_copy;
    VkFence              fence; // TODO: use 'timeline' semaphore?
} tg_render_target;

typedef struct tg_fragment_shader
{
    tg_structure_type    type;
    tg_vulkan_shader     vulkan_shader;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_structure_type          type;
    tg_vulkan_material_type    material_type;
    tg_vertex_shader_h         h_vertex_shader;
    tg_fragment_shader_h       h_fragment_shader;
} tg_material;

typedef struct tg_mesh
{
    tg_structure_type    type;
    tg_bounds            bounds;

    u32                  index_count; // TODO: move all of these into vulkan_buffer, this one only has its total size but not its size in USAGE
    u32                  position_count;
    u32                  normal_count;
    u32                  uv_count;
    u32                  tangent_count;
    u32                  bitangent_count;

    tg_vulkan_buffer     index_buffer;
    tg_vulkan_buffer     positions_buffer;
    tg_vulkan_buffer     normals_buffer;
    tg_vulkan_buffer     uvs_buffer;
    tg_vulkan_buffer     tangents_buffer;
    tg_vulkan_buffer     bitangents_buffer;

    u32                  index_capacity;
    u32                  position_capacity;
    u16*                 p_indices;
    v3*                  p_positions;
} tg_mesh;

typedef struct tg_raytracer
{
    tg_structure_type    type;
    const tg_camera*     p_camera;
    VkSemaphore          semaphore;
    VkFence              fence;
    tg_vulkan_image      storage_image; // TODO: this is a storage image, can every image also be a storage image? also, this should be a render target
    tg_vulkan_buffer     screen_quad_vbo;
    tg_vulkan_buffer     screen_quad_ibo;

    struct
    {
        tg_vulkan_pipeline       compute_pipeline;
        VkCommandBuffer          command_buffer;
        tg_vulkan_buffer         ubo;
    } raytrace_pass;

    struct
    {
        VkSemaphore              image_acquired_semaphore;
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    p_framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          p_command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
    } present_pass;
} tg_raytracer;

typedef struct tg_renderer
{
    tg_structure_type            type;
    const tg_camera*             p_camera;
    tg_vulkan_buffer             view_projection_ubo;
    tg_render_target             render_target;
    tg_vulkan_buffer             screen_quad_indices;
    tg_vulkan_buffer             screen_quad_positions_buffer;
    tg_vulkan_buffer             screen_quad_uvs_buffer;

    u32                          render_command_count;
    tg_render_command_h          ph_render_commands[TG_MAX_RENDER_COMMANDS];

    struct
    {
        b32                      enabled;
        tg_vulkan_image          p_shadow_maps[TG_CASCADED_SHADOW_MAPS];
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    p_framebuffers[TG_CASCADED_SHADOW_MAPS];
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          command_buffer;
        tg_vulkan_buffer         p_lightspace_ubos[TG_CASCADED_SHADOW_MAPS];
    } shadow_pass;

    struct
    {
        tg_vulkan_image          p_color_attachments[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT];
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    framebuffer;
        VkCommandBuffer          command_buffer;
    } geometry_pass;

    struct
    {
        tg_vulkan_image          ssao_attachment;
        VkRenderPass             ssao_render_pass;
        tg_vulkan_framebuffer    ssao_framebuffer;
        tg_vulkan_pipeline       ssao_graphics_pipeline;
        tg_vulkan_image          ssao_noise_image;
        tg_vulkan_buffer         ssao_ubo;

        tg_vulkan_image          blur_attachment;
        VkRenderPass             blur_render_pass;
        tg_vulkan_framebuffer    blur_framebuffer;
        tg_vulkan_pipeline       blur_graphics_pipeline;

        VkCommandBuffer          command_buffer;
    } ssao_pass;

    struct
    {
        tg_vulkan_image          color_attachment;
        VkSemaphore              rendering_finished_semaphore; // TODO: only one needed
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    framebuffer;
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          command_buffer;
        tg_vulkan_buffer         shading_info_ubo;
    } shading_pass;

    struct
    {
        tg_vulkan_shader         acquire_exposure_compute_shader;
        tg_vulkan_pipeline       acquire_exposure_compute_pipeline;
        tg_vulkan_buffer         exposure_storage_buffer;
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    framebuffer;
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          command_buffer;
    } tone_mapping_pass;

    struct
    {
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    framebuffer;
        VkCommandBuffer          command_buffer;
    } forward_pass;

    struct
    {
        VkSemaphore              image_acquired_semaphore;
        VkSemaphore              semaphore;
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    p_framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          p_command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
    } present_pass;

    struct
    {
        VkCommandBuffer          command_buffer;
    } clear_pass;

    v3                           probe_position;
    tg_color_image_3d            probe;
    tg_render_command_h          h_probe_raytracing_target;
} tg_renderer;

typedef struct tg_storage_buffer
{
    tg_structure_type    type;
    tg_vulkan_buffer     vulkan_buffer;
} tg_storage_buffer;

typedef struct tg_uniform_buffer
{
    tg_structure_type    type;
    tg_vulkan_buffer     vulkan_buffer;
} tg_uniform_buffer;

typedef struct tg_vertex_shader
{
    tg_structure_type    type;
    tg_vulkan_shader     vulkan_shader;
} tg_vertex_shader;



VkInstance            instance;
tg_vulkan_surface     surface;
VkPhysicalDevice      physical_device;
VkDevice              device;



VkSwapchainKHR        swapchain;
VkExtent2D            swapchain_extent;
VkImage               p_swapchain_images[TG_VULKAN_SURFACE_IMAGE_COUNT];
VkImageView           p_swapchain_image_views[TG_VULKAN_SURFACE_IMAGE_COUNT];
VkCommandBuffer       global_graphics_command_buffer;
VkCommandBuffer       global_compute_command_buffer;



tg_compute_shader     p_compute_shaders[TG_MAX_COMPUTE_SHADERS];
tg_fragment_shader    p_fragment_shaders[TG_MAX_FRAGMENT_SHADERS];
tg_material           p_materials[TG_MAX_MATERIALS];
tg_mesh               p_meshes[TG_MAX_MESHES];
tg_render_command     p_render_commands[TG_MAX_RENDER_COMMANDS];
tg_raytracer          p_raytracers[TG_MAX_RAYTRACERS];
tg_renderer           p_renderers[TG_MAX_RENDERERS];
tg_vertex_shader      p_vertex_shaders[TG_MAX_VERTEX_SHADERS];



void                          tg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination);
tg_vulkan_buffer              tg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                          tg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);
void                          tg_vulkan_buffer_flush_mapped_memory(tg_vulkan_buffer* p_vulkan_buffer);

tg_vulkan_image               tg_vulkan_color_image_create(u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
tg_vulkan_image               tg_vulkan_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info);
void                          tg_vulkan_color_image_destroy(tg_vulkan_image* p_vulkan_image);

tg_vulkan_image_3d            tg_vulkan_color_image_3d_create(u32 width, u32 height, u32 depth, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
void                          tg_vulkan_color_image_3d_destroy(tg_vulkan_image_3d* p_vulkan_image_3d);

VkCommandBuffer               tg_vulkan_command_buffer_allocate(tg_vulkan_command_pool_type type, VkCommandBufferLevel level);
void                          tg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                          tg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, tg_vulkan_framebuffer* p_framebuffer, VkSubpassContents subpass_contents);
void                          tg_vulkan_command_buffer_cmd_blit_image(VkCommandBuffer command_buffer, tg_vulkan_image* p_source, tg_vulkan_image* p_destination, const VkImageBlit* p_region);
void                          tg_vulkan_command_buffer_cmd_clear_color_image(VkCommandBuffer command_buffer, tg_vulkan_image* p_vulkan_image);
void                          tg_vulkan_command_buffer_cmd_clear_depth_image(VkCommandBuffer command_buffer, tg_vulkan_image* p_vulkan_image);
void                          tg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(VkCommandBuffer command_buffer, VkBuffer source, tg_vulkan_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_buffer_to_cube_map(VkCommandBuffer command_buffer, VkBuffer source, tg_vulkan_cube_map* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_buffer_to_depth_image(VkCommandBuffer command_buffer, VkBuffer source, tg_vulkan_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_color_image(VkCommandBuffer command_buffer, tg_vulkan_image* p_source, tg_vulkan_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_color_image_to_buffer(VkCommandBuffer command_buffer, tg_vulkan_image* p_source, VkBuffer destination);
void                          tg_vulkan_command_buffer_cmd_transition_color_image_layout(VkCommandBuffer command_buffer, tg_vulkan_image* p_vulkan_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_cmd_transition_cube_map_layout(VkCommandBuffer command_buffer, tg_vulkan_cube_map* p_vulkan_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_cmd_transition_depth_image_layout(VkCommandBuffer command_buffer, tg_vulkan_image* p_vulkan_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue_type type);
void                          tg_vulkan_command_buffer_free(tg_vulkan_command_pool_type type, VkCommandBuffer command_buffer); // TODO: save command pool in command buffers?
void                          tg_vulkan_command_buffers_allocate(tg_vulkan_command_pool_type type, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                          tg_vulkan_command_buffers_free(tg_vulkan_command_pool_type type, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

tg_vulkan_cube_map            tg_vulkan_cube_map_create(u32 dimension, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
void                          tg_vulkan_cube_map_destroy(tg_vulkan_cube_map* p_vulkan_cube_map);

tg_vulkan_image               tg_vulkan_depth_image_create(u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info);
void                          tg_vulkan_depth_image_destroy(tg_vulkan_image* p_vulkan_image);

void                          tg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle shader_input_element_handle, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tg_vulkan_cube_map* p_vulkan_cube_map, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_image(VkDescriptorSet descriptor_set, tg_vulkan_image* p_vulkan_image, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tg_vulkan_image* p_vulkan_image, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                       tg_vulkan_fence_create(VkFenceCreateFlags fence_create_flags);
void                          tg_vulkan_fence_destroy(VkFence fence);
void                          tg_vulkan_fence_reset(VkFence fence);
void                          tg_vulkan_fence_wait(VkFence fence);

tg_vulkan_framebuffer         tg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                          tg_vulkan_framebuffer_destroy(tg_vulkan_framebuffer* p_framebuffer);
void                          tg_vulkan_framebuffers_destroy(u32 count, tg_vulkan_framebuffer* p_framebuffers);

VkPhysicalDeviceProperties    tg_vulkan_physical_device_get_properties();

tg_vulkan_pipeline            tg_vulkan_pipeline_create_compute(const tg_vulkan_shader* p_compute_shader);
tg_vulkan_pipeline            tg_vulkan_pipeline_create_graphics(const tg_vulkan_graphics_pipeline_create_info* p_create_info);
tg_vulkan_pipeline            tg_vulkan_pipeline_create_graphics2(const tg_vulkan_graphics_pipeline_create_info* p_create_info, u32 vertex_attrib_count, VkVertexInputBindingDescription* p_vertex_bindings, VkVertexInputAttributeDescription* p_vertex_attribs);
void                          tg_vulkan_pipeline_destroy(tg_vulkan_pipeline* p_pipeline);

void                          tg_vulkan_queue_present(VkPresentInfoKHR* p_present_info);
void                          tg_vulkan_queue_submit(tg_vulkan_queue_type type, u32 submit_count, VkSubmitInfo* p_submit_infos, VkFence fence);
void                          tg_vulkan_queue_wait_idle(tg_vulkan_queue_type type);

VkRenderPass                  tg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                          tg_vulkan_render_pass_destroy(VkRenderPass render_pass);

tg_render_target              tg_vulkan_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tg_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tg_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags);
void                          tg_vulkan_render_target_destroy(tg_render_target* p_render_target);

VkSemaphore                   tg_vulkan_semaphore_create();
void                          tg_vulkan_semaphore_destroy(VkSemaphore semaphore);

tg_vulkan_shader              tg_vulkan_shader_create(const char* p_filename);
void                          tg_vulkan_shader_destroy(tg_vulkan_shader* p_vulkan_shader);

VkDescriptorType              tg_vulkan_structure_type_convert_to_descriptor_type(tg_structure_type type);

void                          tg_vulkan_vkresult_convert_to_string(char* p_buffer, VkResult result);



#endif





void                    tg_graphics_init();
void                    tg_graphics_on_window_resize(u32 width, u32 height);
void                    tg_graphics_shutdown();
void                    tg_graphics_wait_idle();



u32                     tg_vertex_input_attribute_format_get_alignment(tg_vertex_input_attribute_format format);
u32                     tg_vertex_input_attribute_format_get_size(tg_vertex_input_attribute_format format);



tg_color_image          tg_color_image_create(u32 width, u32 height, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
tg_color_image          tg_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info);
void                    tg_color_image_destroy(tg_color_image* p_color_image);

u32                     tg_color_image_format_size(tg_color_image_format format);

tg_color_image_3d       tg_color_image_3d_create(u32 width, u32 height, u32 depth, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                    tg_color_image_3d_destroy(tg_color_image_3d* p_color_image_3d);

void                    tg_compute_shader_bind_input(tg_compute_shader_h h_compute_shader, u32 first_handle_index, u32 handle_count, tg_handle* p_handles);
tg_compute_shader_h     tg_compute_shader_create(const char* filename);
void                    tg_compute_shader_dispatch(tg_compute_shader_h h_compute_shader, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void                    tg_compute_shader_destroy(tg_compute_shader_h h_compute_shader);
tg_compute_shader_h     tg_compute_shader_get(const char* filename);

tg_cube_map             tg_cube_map_create(u32 dimension, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                    tg_cube_map_destroy(tg_cube_map* p_cube_map);
void                    tg_cube_map_set_data(tg_cube_map* p_cube_map, void* p_data);

tg_depth_image          tg_depth_image_create(u32 width, u32 height, tg_depth_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                    tg_depth_image_destroy(tg_depth_image* p_depth_image);

tg_fragment_shader_h    tg_fragment_shader_create(const char* p_filename);
void                    tg_fragment_shader_destroy(tg_fragment_shader_h h_fragment_shader);
tg_fragment_shader_h    tg_fragment_shader_get(const char* p_filename);

tg_kd_tree*             tg_kd_tree_create(const tg_mesh* p_mesh);
void                    tg_kd_tree_destroy(tg_kd_tree* p_kd_tree);

tg_material_h           tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
tg_material_h           tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
void                    tg_material_destroy(tg_material_h h_material);
b32                     tg_material_is_deferred(tg_material_h h_material);
b32                     tg_material_is_forward(tg_material_h h_material);

tg_mesh                 tg_mesh_create();
tg_mesh                 tg_mesh_create2(const char* p_filename, v3 scale); // TODO: scale is temporary
tg_mesh                 tg_mesh_create_sphere(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents);
tg_mesh                 tg_mesh_create_sphere_flat(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents);
void                    tg_mesh_set_indices(tg_mesh* p_mesh, u32 count, const u16* p_indices);
void                    tg_mesh_set_positions(tg_mesh* p_mesh, u32 count, const v3* p_positions);
void                    tg_mesh_set_normals(tg_mesh* p_mesh, u32 count, const v3* p_normals);
void                    tg_mesh_set_uvs(tg_mesh* p_mesh, u32 count, const v2* p_uvs);
void                    tg_mesh_set_tangents_bitangents(tg_mesh* p_mesh, u32 count, const v3* p_tangents, const v3* p_bitangents);
void                    tg_mesh_regenerate_normals(tg_mesh* p_mesh);
void                    tg_mesh_regenerate_tangents_bitangents(tg_mesh* p_mesh);
void                    tg_mesh_destroy(tg_mesh* p_mesh);

tg_raytracer_h          tg_raytracer_create(tg_camera* p_camera);
void                    tg_raytracer_begin(tg_raytracer_h h_raytracer);
void                    tg_raytracer_push_directional_light(tg_raytracer_h h_raytracer, v3 direction, v3 color);
void                    tg_raytracer_push_point_light(tg_raytracer_h h_raytracer, v3 position, v3 color);
void                    tg_raytracer_push_sphere(tg_raytracer_h h_raytracer, v3 center, f32 radius);
void                    tg_raytracer_end(tg_raytracer_h h_raytracer);
void                    tg_raytracer_present(tg_raytracer_h h_raytracer);
void                    tg_raytracer_get_render_target(tg_raytracer_h h_raytracer);
void                    tg_raytracer_destroy(tg_raytracer_h h_raytracer);

tg_render_command_h     tg_render_command_create(tg_mesh* p_mesh, tg_material_h h_material, v3 position, u32 global_resource_count, tg_handle* p_global_resources);
void                    tg_render_command_destroy(tg_render_command_h h_render_command);
void                    tg_render_command_set_position(tg_render_command_h h_render_command, v3 position);

tg_renderer_h           tg_renderer_create(tg_camera* p_camera);
void                    tg_renderer_destroy(tg_renderer_h h_renderer);
void                    tg_renderer_enable_shadows(tg_renderer_h h_renderer, b32 enable);
void                    tg_renderer_bake_begin(tg_renderer_h h_renderer);
void                    tg_renderer_bake_push_probe(tg_renderer_h h_renderer, f32 x, f32 y, f32 z);
void                    tg_renderer_bake_push_static(tg_renderer_h h_renderer, tg_render_command_h h_render_command);
void                    tg_renderer_bake_end(tg_renderer_h h_renderer);
void                    tg_renderer_begin(tg_renderer_h h_renderer);
void                    tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color);
void                    tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color);
void                    tg_renderer_execute(tg_renderer_h h_renderer, tg_render_command_h h_render_command);
void                    tg_renderer_end(tg_renderer_h h_renderer);
void                    tg_renderer_present(tg_renderer_h h_renderer);
void                    tg_renderer_clear(tg_renderer_h h_renderer);
tg_render_target_h      tg_renderer_get_render_target(tg_renderer_h h_renderer);

tg_storage_buffer       tg_storage_buffer_create(u64 size, b32 visible);
u64                     tg_storage_buffer_size(tg_storage_buffer* p_storage_buffer);
void*                   tg_storage_buffer_data(tg_storage_buffer* p_storage_buffer);
void                    tg_storage_buffer_destroy(tg_storage_buffer* p_storage_buffer);

tg_uniform_buffer       tg_uniform_buffer_create(u64 size);
u64                     tg_uniform_buffer_size(tg_uniform_buffer* p_uniform_buffer);
void*                   tg_uniform_buffer_data(tg_uniform_buffer* p_uniform_buffer);
void                    tg_uniform_buffer_destroy(tg_uniform_buffer* p_uniform_buffer);

tg_vertex_shader_h      tg_vertex_shader_create(const char* p_filename);
void                    tg_vertex_shader_destroy(tg_vertex_shader_h h_vertex_shader);
tg_vertex_shader_h      tg_vertex_shader_get(const char* p_filename);

#endif
