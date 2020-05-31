#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_vulkan_memory_allocator.h"
#include <vulkan/vulkan.h>
#undef near
#undef far
#undef min
#undef max

#ifdef TG_DEBUG
#define VK_CALL(x)                                                   TG_ASSERT(x == VK_SUCCESS)
#else
#define VK_CALL(x)                                                   x
#endif

#define TG_VULKAN_SURFACE_IMAGE_COUNT                                3
#define TG_VULKAN_COLOR_IMAGE_FORMAT                                 VK_FORMAT_R8G8B8A8_SRGB
#define TG_VULKAN_COLOR_IMAGE_LAYOUT                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
#define TG_VULKAN_DEPTH_IMAGE_LAYOUT                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL





#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT    3
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT_COUNT    1
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_ATTACHMENT_COUNT          (TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT + TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT_COUNT)

#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_FORMAT           VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT       0
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_FORMAT             VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT         1
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_FORMAT             VK_FORMAT_R16G16B16A16_SFLOAT
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT         2
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_FORMAT              VK_FORMAT_D32_SFLOAT
#define TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT          3


#define TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_COUNT     1
#define TG_DEFERRED_RENDERER_SHADING_PASS_DEPTH_ATTACHMENT_COUNT     0
#define TG_DEFERRED_RENDERER_SHADING_PASS_ATTACHMENT_COUNT           (TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_COUNT + TG_DEFERRED_RENDERER_SHADING_PASS_DEPTH_ATTACHMENT_COUNT)

#define TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_FORMAT    VK_FORMAT_B8G8R8A8_UNORM
#define TG_DEFERRED_RENDERER_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS     512
#define TG_DEFERRED_RENDERER_SHADING_PASS_MAX_POINT_LIGHTS           512



#define TG_VULKAN_MAX_CAMERAS_PER_ENTITY                             4
#define TG_VULKAN_MAX_ENTITIES_PER_CAMERA                            65536
#define TG_VULKAN_MAX_LOD_COUNT                                      8



#define TG_CAMERA_VIEW(camera_h)                                     (((m4*)camera_h->view_projection_ubo.memory.p_mapped_device_memory)[0])
#define TG_CAMERA_PROJ(camera_h)                                     (((m4*)camera_h->view_projection_ubo.memory.p_mapped_device_memory)[1])
#define TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(e_h)                       (*(m4*)e_h->model_ubo.memory.p_mapped_device_memory)



typedef enum tg_vulkan_material_type
{
    TG_VULKAN_MATERIAL_TYPE_DEFERRED,
    TG_VULKAN_MATERIAL_TYPE_FORWARD
} tg_vulkan_material_type;



typedef struct tg_vulkan_descriptor
{
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;
} tg_vulkan_descriptor;

typedef struct tg_vulkan_buffer
{
    u64                       size;
    VkBuffer                  buffer;
    tg_vulkan_memory_block    memory;
} tg_vulkan_buffer;

typedef struct tg_vulkan_compute_shader
{
    VkShaderModule           shader_module;
    tg_vulkan_descriptor     descriptor;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               compute_pipeline;
} tg_vulkan_compute_shader;

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

typedef struct tg_vulkan_image_extent
{
    f32    left;
    f32    bottom;
    f32    right;
    f32    top;
} tg_vulkan_image_extent;

typedef struct tg_vulkan_queue
{
    u8         index;
    VkQueue    queue;
} tg_vulkan_queue;

typedef struct tg_vulkan_sampler_create_info
{
    VkFilter                min_filter;
    VkFilter                mag_filter;
    VkSamplerAddressMode    address_mode_u;
    VkSamplerAddressMode    address_mode_v;
    VkSamplerAddressMode    address_mode_w;
} tg_vulkan_sampler_create_info;

typedef struct tg_vulkan_color_image_create_info
{
    u32                               width;
    u32                               height;
    u32                               mip_levels;
    VkFormat                          format;
    tg_vulkan_sampler_create_info*    p_vulkan_sampler_create_info;
} tg_vulkan_color_image_create_info;

typedef struct tg_vulkan_depth_image_create_info
{
    u32                               width;
    u32                               height;
    VkFormat                          format;
    tg_vulkan_sampler_create_info*    p_vulkan_sampler_create_info;
} tg_vulkan_depth_image_create_info;

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



typedef struct tg_deferred_renderer_light_setup_uniform_buffer
{
    u32    directional_light_count;
    u32    point_light_count;
    u32    padding[2];

    v4     directional_light_positions_radii[TG_DEFERRED_RENDERER_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS];
    v4     directional_light_colors[TG_DEFERRED_RENDERER_SHADING_PASS_MAX_DIRECTIONAL_LIGHTS];

    v4     point_light_positions_radii[TG_DEFERRED_RENDERER_SHADING_PASS_MAX_POINT_LIGHTS];
    v4     point_light_colors[TG_DEFERRED_RENDERER_SHADING_PASS_MAX_POINT_LIGHTS];
} tg_deferred_renderer_light_setup_uniform_buffer;



typedef struct tg_color_image
{
    tg_handle_type            type;
    u32                       width;
    u32                       height;
    u32                       mip_levels;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
    VkSampler                 sampler;
} tg_color_image;

typedef struct tg_compute_shader
{
    tg_handle_type              type;
    tg_vulkan_compute_shader    compute_shader;
    VkCommandBuffer             command_buffer;
} tg_compute_shader;

typedef struct tg_depth_image
{
    tg_handle_type            type;
    u32                       width;
    u32                       height;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
    VkSampler                 sampler;
} tg_depth_image;

typedef struct tg_render_target
{
    tg_handle_type    type;
    tg_color_image    color_attachment;
    tg_depth_image    depth_attachment;
    tg_color_image    color_attachment_copy;
    tg_depth_image    depth_attachment_copy;
    VkFence           fence;
} tg_render_target;

typedef struct tg_camera
{
    tg_handle_type                   type;
    v3                               position;
    tg_vulkan_buffer                 view_projection_ubo;
    tg_render_target                 render_target;
    u32                              captured_entity_count;
    tg_entity_graphics_data_ptr_h    captured_entities[TG_VULKAN_MAX_ENTITIES_PER_CAMERA];
    struct
    {
        tg_deferred_renderer_h       deferred_renderer_h;
        tg_forward_renderer_h        forward_renderer_h;
    } capture_pass;
    struct
    {
        tg_vulkan_buffer             vbo;
        tg_vulkan_buffer             ibo;
        VkSemaphore                  image_acquired_semaphore;
        VkSemaphore                  semaphore;
        VkRenderPass                 render_pass;
        VkFramebuffer                p_framebuffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
        tg_vulkan_descriptor         descriptor;
        VkShaderModule               vertex_shader;
        VkShaderModule               fragment_shader;
        VkPipelineLayout             pipeline_layout;
        VkPipeline                   graphics_pipeline;
        VkCommandBuffer              p_command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
    } present_pass;
    struct
    {
        VkCommandBuffer              command_buffer;
    } clear_pass;
} tg_camera;

typedef struct tg_vulkan_camera_info
{
    tg_camera_h               camera_h;
    tg_vulkan_descriptor      descriptor;
    VkPipelineLayout          pipeline_layout;
    VkPipeline                graphics_pipeline;
    VkCommandBuffer           p_command_buffers[TG_VULKAN_MAX_LOD_COUNT];
} tg_vulkan_camera_info;

typedef struct tg_entity_graphics_data_ptr
{
    tg_handle_type           type;
    u32                      lod_count;
    tg_mesh_h                p_lod_meshes_h[TG_VULKAN_MAX_LOD_COUNT]; // TODO: does this need to be a pointer?
    tg_material_h            material_h;
    tg_vulkan_buffer         model_ubo;
    u32                      camera_info_count;
    tg_vulkan_camera_info    p_camera_infos[TG_VULKAN_MAX_CAMERAS_PER_ENTITY];
} tg_entity_graphics_data_ptr;

typedef struct tg_fragment_shader
{
    tg_handle_type    type;
    VkShaderModule    shader_module;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_handle_type             type;
    tg_vulkan_material_type    material_type;
    tg_vertex_shader_h         vertex_shader_h;
    tg_fragment_shader_h       fragment_shader_h;
    tg_vulkan_descriptor       descriptor;
} tg_material;

typedef struct tg_mesh
{
    tg_handle_type      type;
    tg_bounds           bounds;
    tg_vulkan_buffer    vbo;
    tg_vulkan_buffer    ibo;
} tg_mesh;

typedef struct tg_storage_buffer
{
    tg_handle_type      type;
    tg_vulkan_buffer    buffer;
} tg_storage_buffer;

typedef struct tg_storage_image_3d
{
    tg_handle_type            type;
    u32                       width;
    u32                       height;
    u32                       depth;
    VkFormat                  format;
    VkImage                   image;
    tg_vulkan_memory_block    memory;
    VkImageView               image_view;
} tg_storage_image_3d;

typedef struct tg_texture_atlas
{
    tg_handle_type             type;
    tg_color_image             color_image;
    tg_vulkan_image_extent*    p_extents;
} tg_texture_atlas;

typedef struct tg_uniform_buffer
{
    tg_handle_type      type;
    tg_vulkan_buffer    buffer;
} tg_uniform_buffer;

typedef struct tg_vertex_shader
{
    tg_handle_type    type;
    VkShaderModule    shader_module;
} tg_vertex_shader;



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



void                          tg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination);
tg_vulkan_buffer              tg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                          tg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);
void                          tg_vulkan_buffer_flush_mapped_memory(tg_vulkan_buffer* p_vulkan_buffer);

void                          tg_vulkan_camera_info_clear(tg_vulkan_camera_info* p_vulkan_camera_info);

tg_color_image                tg_vulkan_color_image_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info);
VkFormat                      tg_vulkan_color_image_convert_format(tg_color_image_format format);
void                          tg_vulkan_color_image_destroy(tg_color_image* p_color_image);

VkCommandBuffer               tg_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level);
void                          tg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                          tg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkSubpassContents subpass_contents);
void                          tg_vulkan_command_buffer_cmd_blit_color_image(VkCommandBuffer command_buffer, tg_color_image* p_source, tg_color_image* p_destination, const VkImageBlit* p_region);
void                          tg_vulkan_command_buffer_cmd_blit_depth_image(VkCommandBuffer command_buffer, tg_depth_image* p_source, tg_depth_image* p_destination, const VkImageBlit* p_region);
void                          tg_vulkan_command_buffer_cmd_clear_color_image(VkCommandBuffer command_buffer, tg_color_image* p_color_image);
void                          tg_vulkan_command_buffer_cmd_clear_depth_image(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image);
void                          tg_vulkan_command_buffer_cmd_clear_storage_image_3d(VkCommandBuffer command_buffer, tg_storage_image_3d* p_storage_image_3d);
void                          tg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(VkCommandBuffer command_buffer, VkBuffer source, tg_color_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_buffer_to_depth_image(VkCommandBuffer command_buffer, VkBuffer source, tg_depth_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_color_image(VkCommandBuffer command_buffer, tg_color_image* p_source, tg_color_image* p_destination);
void                          tg_vulkan_command_buffer_cmd_copy_color_image_to_buffer(VkCommandBuffer command_buffer, tg_color_image* p_source, VkBuffer destination);
void                          tg_vulkan_command_buffer_cmd_copy_storage_image_3d_to_buffer(VkCommandBuffer command_buffer, tg_storage_image_3d* p_source, VkBuffer destination);
void                          tg_vulkan_command_buffer_cmd_transition_color_image_layout(VkCommandBuffer command_buffer, tg_color_image* p_color_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_cmd_transition_depth_image_layout(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_cmd_transition_storage_image_3d_layout(VkCommandBuffer command_buffer, tg_storage_image_3d* p_storage_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                          tg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue* p_vulkan_queue);
void                          tg_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void                          tg_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                          tg_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

VkPipeline                    tg_vulkan_compute_pipeline_create(VkShaderModule shader_module, VkPipelineLayout pipeline_layout);
void                          tg_vulkan_compute_pipeline_destroy(VkPipeline compute_pipeline);

tg_vulkan_compute_shader      tg_vulkan_compute_shader_create(const char* filename, u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                          tg_vulkan_compute_shader_destroy(tg_vulkan_compute_shader* p_vulkan_compute_shader);

tg_depth_image                tg_vulkan_depth_image_create(const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info);
VkFormat                      tg_vulkan_depth_image_convert_format(tg_depth_image_format format);
void                          tg_vulkan_depth_image_destroy(tg_depth_image* p_depth_image);

void                          tg_deferred_renderer_execute(tg_deferred_renderer_h deferred_renderer_h, VkCommandBuffer command_buffer);// TODO: move everything over and rename functions

tg_vulkan_descriptor          tg_vulkan_descriptor_create(u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                          tg_vulkan_descriptor_destroy(tg_vulkan_descriptor* p_vulkan_descriptor);

void                          tg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle shader_input_element_handle, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_color_image(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_color_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_depth_image(VkDescriptorSet descriptor_set, tg_depth_image* p_depth_image, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_depth_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_depth_image, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_storage_image_3d(VkDescriptorSet descriptor_set, tg_storage_image_3d* p_storage_image_3d, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_texture_atlas(VkDescriptorSet descriptor_set, tg_texture_atlas* p_texture_atlas, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                       tg_vulkan_fence_create(VkFenceCreateFlags fence_create_flags);
void                          tg_vulkan_fence_destroy(VkFence fence);
void                          tg_vulkan_fence_reset(VkFence fence);
void                          tg_vulkan_fence_wait(VkFence fence);

VkFramebuffer                 tg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                          tg_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);
void                          tg_vulkan_framebuffers_destroy(u32 count, VkFramebuffer* p_framebuffers);

VkPipeline                    tg_vulkan_graphics_pipeline_create(const tg_vulkan_graphics_pipeline_create_info* p_vulkan_graphics_pipeline_create_info);
void                          tg_vulkan_graphics_pipeline_destroy(VkPipeline graphics_pipeline);

VkDescriptorType              tg_vulkan_handle_type_convert_to_descriptor_type(tg_handle_type type);

VkFilter                      tg_vulkan_image_convert_filter(tg_image_filter filter);
VkSamplerAddressMode          tg_vulkan_image_convert_address_mode(tg_image_address_mode address_mode);

VkPhysicalDeviceProperties    tg_vulkan_physical_device_get_properties();

VkPipelineLayout              tg_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* p_descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* p_push_constant_ranges);
void                          tg_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);

VkRenderPass                  tg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                          tg_vulkan_render_pass_destroy(VkRenderPass render_pass);

tg_render_target              tg_vulkan_render_target_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info, const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info, VkFenceCreateFlags fence_create_flags);
void                          tg_vulkan_render_target_destroy(tg_render_target* p_render_target);

VkSemaphore                   tg_vulkan_semaphore_create();
void                          tg_vulkan_semaphore_destroy(VkSemaphore semaphore);

VkShaderModule                tg_vulkan_shader_module_create(const char* p_filename);

tg_storage_image_3d           tg_vulkan_storage_image_3d_create(u32 width, u32 height, u32 depth, VkFormat format);
void                          tg_vulkan_storage_image_3d_destroy(tg_storage_image_3d* p_storage_image_3d);

VkFormat                      tg_vulkan_storage_image_convert_format(tg_storage_image_format format);

#endif

#endif
