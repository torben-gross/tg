#ifndef tgg_VULKAN_H
#define tgg_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include <vulkan/vulkan.h>
#undef near
#undef far

#ifdef TG_DEBUG
#define VK_CALL(x) TG_ASSERT(x == VK_SUCCESS)
#else
#define VK_CALL(x) x
#endif

#define TG_VULKAN_SURFACE_IMAGE_COUNT    3
#define TG_VULKAN_COLOR_IMAGE_FORMAT     VK_FORMAT_R8G8B8A8_SRGB // TODO: i dont really need this
#define TG_VULKAN_COLOR_IMAGE_LAYOUT     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
#define TG_VULKAN_DEPTH_IMAGE_LAYOUT     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL



#define TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT    3
#define TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT_COUNT    1
#define TG_RENDERER_3D_GEOMETRY_PASS_ATTACHMENT_COUNT          TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT + TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT_COUNT

#define TG_RENDERER_3D_GEOMETRY_PASS_POSITION_FORMAT           VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT       0
#define TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_FORMAT             VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT         1
#define TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_FORMAT             VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT         2
#define TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_FORMAT              VK_FORMAT_D32_SFLOAT
#define TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT          3


#define TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_COUNT     1
#define TG_RENDERER_3D_SHADING_PASS_DEPTH_ATTACHMENT_COUNT     0
#define TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT           TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_COUNT + TG_RENDERER_3D_SHADING_PASS_DEPTH_ATTACHMENT_COUNT

#define TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_FORMAT    VK_FORMAT_B8G8R8A8_UNORM
#define TG_RENDERER_3D_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS     512
#define TG_RENDERER_3D_SHADING_PASS_MAX_POINT_LIGHTS           512





typedef struct tg_vulkan_descriptor
{
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;
} tg_vulkan_descriptor;

typedef struct tg_vulkan_color_image_create_info
{
    u32                     width;
    u32                     height;
    u32                     mip_levels;
    VkFormat                format;
    VkFilter                min_filter;
    VkFilter                mag_filter;
    VkSamplerAddressMode    address_mode_u;
    VkSamplerAddressMode    address_mode_v;
    VkSamplerAddressMode    address_mode_w;
} tg_vulkan_color_image_create_info;

typedef struct tg_vulkan_buffer
{
    u64               size;
    VkBuffer          buffer;
    VkDeviceMemory    device_memory;
    void*             p_mapped_device_memory;
} tg_vulkan_buffer;

typedef struct tg_vulkan_compute_shader
{
    VkShaderModule           shader_module;
    tg_vulkan_descriptor     descriptor;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               compute_pipeline;
} tg_vulkan_compute_shader;

typedef struct tg_vulkan_depth_image_create_info
{
    u32                     width;
    u32                     height;
    VkFormat                format;
    VkFilter                min_filter;
    VkFilter                mag_filter;
    VkSamplerAddressMode    address_mode_u;
    VkSamplerAddressMode    address_mode_v;
    VkSamplerAddressMode    address_mode_w;
} tg_vulkan_depth_image_create_info;

typedef struct tg_vulkan_image_extent
{
    f32    left;
    f32    bottom;
    f32    right;
    f32    top;
} tg_vulkan_image_extent;

typedef struct tg_vulkan_graphics_pipeline_create_info
{
    VkShaderModule                           vertex_shader;
    VkShaderModule                           fragment_shader;
    VkCullModeFlagBits                       cull_mode;
    VkSampleCountFlagBits                    sample_count;
    VkBool32                                 depth_test_enable;
    VkBool32                                 depth_write_enable;
    u32                                      attachment_count;
    VkBool32                                 blend_enable;
    VkPipelineVertexInputStateCreateInfo*    p_pipeline_vertex_input_state_create_info;
    VkPipelineLayout                         pipeline_layout;
    VkRenderPass                             render_pass;
} tg_vulkan_graphics_pipeline_create_info;

typedef struct tg_vulkan_queue
{
    u8         index;
    VkQueue    queue;
} tg_vulkan_queue;

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



typedef struct tg_renderer_3d_light_setup_uniform_buffer
{
    u32    directional_light_count;
    u32    point_light_count;
    u32    padding[2];

    v4     directional_light_positions_radii[TG_RENDERER_3D_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS];
    v4     directional_light_colors[TG_RENDERER_3D_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS];

    v4     point_light_positions_radii[TG_RENDERER_3D_SHADING_PASS_MAX_POINT_LIGHTS];
    v4     point_light_colors[TG_RENDERER_3D_SHADING_PASS_MAX_POINT_LIGHTS];
} tg_renderer_3d_light_setup_uniform_buffer;

typedef struct tg_renderer_3d_copy_image_compute_buffer
{
    u32    width;
    u32    height;
    u32    padding[2];
    v4     data[0];
} tg_renderer_3d_copy_image_compute_buffer;



typedef struct tg_camera
{
    m4    view;
    m4    projection;
} tg_camera;

typedef struct tg_color_image
{
    u32                   width;
    u32                   height;
    u32                   mip_levels;
    VkFormat              format;
    VkImage               color_image;
    VkDeviceMemory        device_memory;
    VkImageView           image_view;
    VkSampler             sampler;
} tg_color_image;

typedef struct tg_compute_buffer
{
    tg_vulkan_buffer    buffer;
} tg_compute_buffer;

typedef struct tg_compute_shader
{
    u32                         input_element_count;
    tg_shader_input_element*    p_input_elements;
    tg_vulkan_compute_shader    compute_shader;
    VkCommandBuffer             command_buffer;
} tg_compute_shader;

typedef struct tg_depth_image
{
    u32                   width;
    u32                   height;
    VkFormat              format;
    VkImage               image;
    VkDeviceMemory        device_memory;
    void*                 p_mapped_device_memory;
    VkImageView           image_view;
    VkSampler             sampler;
} tg_depth_image;

typedef struct tg_entity_graphics_data_ptr
{
    tg_renderer_3d_h         renderer_3d_h;
    tg_vulkan_buffer         uniform_buffer;
    tg_vulkan_descriptor     descriptor;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               graphics_pipeline;
    VkCommandBuffer          command_buffer;
} tg_entity_graphics_data_ptr;

typedef struct tg_fragment_shader
{
    VkShaderModule    shader_module;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_vertex_shader_h          vertex_shader_h;
    tg_fragment_shader_h        fragment_shader_h;
    tg_vulkan_descriptor        descriptor;
} tg_material;

typedef struct tg_mesh
{
    tg_vulkan_buffer    vbo;
    tg_vulkan_buffer    ibo;
} tg_mesh;

typedef struct tg_model
{
    tg_mesh_h          mesh_h;
    tg_material_h      material_h;
} tg_model;

typedef struct tg_texture_atlas
{
    tg_color_image             color_image;
    tg_vulkan_image_extent*    p_extents;
} tg_texture_atlas;

typedef struct tg_uniform_buffer
{
    tg_vulkan_buffer    buffer;
} tg_uniform_buffer;

typedef struct tg_vertex_shader
{
    VkShaderModule    shader_module;
} tg_vertex_shader;



typedef struct tg_renderer_3d_geometry_pass
{
    tg_color_image               position_attachment;
    tg_color_image               normal_attachment;
    tg_color_image               albedo_attachment;
    tg_depth_image               depth_attachment;

    VkFence                      rendering_finished_fence;
    VkSemaphore                  rendering_finished_semaphore;

    tg_vulkan_buffer             view_projection_ubo;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    VkCommandBuffer              command_buffer;
} tg_renderer_3d_geometry_pass;

typedef struct tg_renderer_3d_shading_pass
{
    tg_color_image               color_attachment;

    tg_vulkan_buffer             vbo;
    tg_vulkan_buffer             ibo;

    VkFence                      rendering_finished_fence;
    VkSemaphore                  rendering_finished_semaphore;
    VkFence                      geometry_pass_attachments_cleared_fence;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    tg_vulkan_descriptor         descriptor;

    VkShaderModule               vertex_shader_h;
    VkShaderModule               fragment_shader_h;
    VkPipelineLayout             pipeline_layout;
    VkPipeline                   graphics_pipeline;

    VkCommandBuffer              command_buffer;

    tg_vulkan_buffer             point_lights_ubo;

    struct
    {
        tg_vulkan_compute_shader     find_exposure_compute_shader;
        tg_vulkan_buffer             exposure_compute_buffer;

        tg_color_image               color_attachment;
        VkRenderPass                 render_pass;
        VkFramebuffer                framebuffer;

        tg_vulkan_descriptor         descriptor;

        VkShaderModule               vertex_shader_h;
        VkShaderModule               fragment_shader_h;
        VkPipelineLayout             pipeline_layout;
        VkPipeline                   graphics_pipeline;
    } exposure;
} tg_renderer_3d_shading_pass;

typedef struct tg_renderer_3d_present_pass
{
    tg_vulkan_buffer         vbo;
    tg_vulkan_buffer         ibo;

    VkSemaphore              image_acquired_semaphore;
    VkFence                  rendering_finished_fence;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];

    tg_vulkan_descriptor     descriptor;

    VkShaderModule           vertex_shader_h;
    VkShaderModule           fragment_shader_h;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               graphics_pipeline;

    VkCommandBuffer          command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
} tg_renderer_3d_present_pass;

typedef struct tg_renderer_3d
{
    tg_camera_h                     camera_h;
    tg_renderer_3d_geometry_pass    geometry_pass;
    tg_renderer_3d_shading_pass     shading_pass;
    tg_renderer_3d_present_pass     present_pass;
} tg_renderer_3d;



VkInstance           instance;
tg_vulkan_surface    surface;
VkPhysicalDevice     physical_device;
VkDevice             device;
tg_vulkan_queue      graphics_queue;
tg_vulkan_queue      present_queue;
tg_vulkan_queue      compute_queue;
VkCommandPool        graphics_command_pool;
VkCommandPool        compute_command_pool;



VkSwapchainKHR       swapchain;
VkExtent2D           swapchain_extent;
VkImage              swapchain_images[TG_VULKAN_SURFACE_IMAGE_COUNT];
VkImageView          swapchain_image_views[TG_VULKAN_SURFACE_IMAGE_COUNT];



void                          tgg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination);
tg_vulkan_buffer              tgg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                          tgg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);

tg_color_image                tgg_vulkan_color_image_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info);
VkFormat                      tgg_vulkan_color_image_convert_format(tg_color_image_format format);
void                          tgg_vulkan_color_image_destroy(tg_color_image* p_color_image);

VkCommandBuffer               tgg_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level);
void                          tgg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                          tgg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkSubpassContents subpass_contents);
void                          tgg_vulkan_command_buffer_cmd_clear_color_image(VkCommandBuffer command_buffer, tg_color_image* p_color_image);
void                          tgg_vulkan_command_buffer_cmd_clear_depth_image(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image);
void                          tgg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(VkCommandBuffer command_buffer, VkBuffer source, tg_color_image* p_destination);
void                          tgg_vulkan_command_buffer_cmd_copy_buffer_to_depth_image(VkCommandBuffer command_buffer, VkBuffer source, tg_depth_image* p_destination);
void                          tgg_vulkan_command_buffer_cmd_copy_color_image_to_buffer(VkCommandBuffer command_buffer, tg_color_image* p_source, VkBuffer destination);
void                          tgg_vulkan_command_buffer_cmd_transition_color_image_layout(VkCommandBuffer command_buffer, tg_color_image* p_color_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tgg_vulkan_command_buffer_cmd_transition_depth_image_layout(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tgg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue* p_vulkan_queue);
void                          tgg_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void                          tgg_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                          tgg_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

VkPipeline                    tgg_vulkan_compute_pipeline_create(VkShaderModule shader_module, VkPipelineLayout pipeline_layout);
void                          tgg_vulkan_compute_pipeline_destroy(VkPipeline compute_pipeline);

tg_vulkan_compute_shader      tgg_vulkan_compute_shader_create(const char* filename, u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                          tgg_vulkan_compute_shader_destroy(tg_vulkan_compute_shader* p_vulkan_compute_shader);

tg_depth_image                tgg_vulkan_depth_image_create(const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info);
VkFormat                      tgg_vulkan_depth_image_convert_format(tg_depth_image_format format);
void                          tgg_vulkan_depth_image_destroy(tg_depth_image* p_depth_image);

tg_vulkan_descriptor          tgg_vulkan_descriptor_create(u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                          tgg_vulkan_descriptor_destroy(tg_vulkan_descriptor* p_vulkan_descriptor);

void                          tgg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_shader_input_element* p_shader_input_element, tg_handle shader_input_element_handle, u32 dst_binding);
void                          tgg_vulkan_descriptor_set_update_color_image(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding);
void                          tgg_vulkan_descriptor_set_update_color_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding, u32 array_index);
void                          tgg_vulkan_descriptor_set_update_depth_image(VkDescriptorSet descriptor_set, tg_depth_image* p_depth_image, u32 dst_binding);
void                          tgg_vulkan_descriptor_set_update_depth_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_depth_image, u32 dst_binding, u32 array_index);
void                          tgg_vulkan_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tgg_vulkan_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tgg_vulkan_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tgg_vulkan_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tgg_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                       tgg_vulkan_fence_create(VkFenceCreateFlags fence_create_flags);
void                          tgg_vulkan_fence_destroy(VkFence fence);
void                          tgg_vulkan_fence_reset(VkFence fence);
void                          tgg_vulkan_fence_wait(VkFence fence);

VkFramebuffer                 tgg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                          tgg_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);

VkPipeline                    tgg_vulkan_graphics_pipeline_create(const tg_vulkan_graphics_pipeline_create_info* p_vulkan_graphics_pipeline_create_info);
void                          tgg_vulkan_graphics_pipeline_destroy(VkPipeline graphics_pipeline);

VkFilter                      tgg_vulkan_image_convert_filter(tg_image_filter filter);
VkSamplerAddressMode          tgg_vulkan_image_convert_address_mode(tg_image_address_mode address_mode);

VkPhysicalDeviceProperties    tgg_vulkan_physical_device_get_properties();

VkPipelineLayout              tgg_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges);
void                          tgg_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);

VkRenderPass                  tgg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                          tgg_vulkan_render_pass_destroy(VkRenderPass render_pass);

VkSampler                     tgg_vulkan_sampler_create(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w);
void                          tgg_vulkan_sampler_destroy(VkSampler sampler);

VkSemaphore                   tgg_vulkan_semaphore_create();
void                          tgg_vulkan_semaphore_destroy(VkSemaphore semaphore);

VkDescriptorType              tgg_vulkan_shader_input_element_type_convert(tg_shader_input_element_type type);

VkShaderModule                tgg_vulkan_shader_module_create(const char* p_filename);
void                          tgg_vulkan_shader_module_destroy(VkShaderModule shader_module);

#endif

#endif
