#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

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

#define TG_GRAPHICS_VULKAN_SURFACE_IMAGE_COUNT 3





typedef struct tg_vulkan_buffer
{
    u64               size;
    VkBuffer          buffer;
    VkDeviceMemory    device_memory;
    void* p_mapped_device_memory;
} tg_vulkan_buffer;

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

typedef struct tg_vulkan_surface
{
    VkSurfaceKHR          surface;
    VkSurfaceFormatKHR    format;
} tg_vulkan_surface;

typedef struct tg_vulkan_queue
{
    u8         index;
    VkQueue    queue;
} tg_vulkan_queue;





typedef struct tg_camera
{
    struct
    {
        f32             fov_y;
        f32             near;
        f32             far;
        m4              projection;
    } projection;

    struct
    {
        v3              position;
        f32             pitch;
        f32             yaw;
        f32             roll;
        m4              view;
    } view;
} tg_camera;

typedef struct tg_compute_buffer
{
    tg_vulkan_buffer    buffer;
} tg_compute_buffer;

typedef struct tg_compute_shader
{
    u32                                      input_element_count;
    tg_compute_shader_input_element_type*    p_input_element_types;

    VkShaderModule                           shader_module;
    VkDescriptorPool                         descriptor_pool;
    VkDescriptorSetLayout                    descriptor_set_layout;
    VkDescriptorSet                          descriptor_set;
    VkPipelineLayout                         pipeline_layout;
    VkPipeline                               pipeline;
    VkCommandPool                            command_pool;
    VkCommandBuffer                          command_buffer;
} tg_compute_shader;

typedef struct tg_fragment_shader
{
    VkShaderModule    shader_module;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_vertex_shader_h       vertex_shader_h;
    tg_fragment_shader_h     fragment_shader_h;
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

    struct
    {
        VkDescriptorPool         descriptor_pool;
        VkDescriptorSetLayout    descriptor_set_layout;
        VkDescriptorSet          descriptor_set;
        VkPipelineLayout         pipeline_layout;
        VkPipeline               pipeline;
        VkCommandBuffer          command_buffer;
    } render_data;

} tg_model;

typedef struct tg_uniform_buffer
{
    tg_vulkan_buffer    buffer;
} tg_uniform_buffer;

typedef struct tg_vertex_shader
{
    VkShaderModule    shader_module;
} tg_vertex_shader;



VkInstance           instance;
tg_vulkan_surface    surface;
VkPhysicalDevice     physical_device;
VkDevice             device;
tg_vulkan_queue      graphics_queue;
tg_vulkan_queue      present_queue;
tg_vulkan_queue      compute_queue;
VkCommandPool        command_pool;



VkSwapchainKHR      swapchain;
VkExtent2D          swapchain_extent;
VkImage             swapchain_images[TG_GRAPHICS_VULKAN_SURFACE_IMAGE_COUNT];
VkImageView         swapchain_image_views[TG_GRAPHICS_VULKAN_SURFACE_IMAGE_COUNT];



void                                      tg_graphics_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer target);
void                                      tg_graphics_vulkan_buffer_copy_to_image(u32 width, u32 height, VkBuffer source, VkImage target);
tg_vulkan_buffer                          tg_graphics_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                                      tg_graphics_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);

VkCommandBuffer                           tg_graphics_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level);
void                                      tg_graphics_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                                      tg_graphics_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkSubpassContents subpass_contents);
void                                      tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, u32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                                      tg_graphics_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer);
void                                      tg_graphics_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void                                      tg_graphics_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                                      tg_graphics_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

VkCommandPool                             tg_graphics_vulkan_command_pool_create(VkCommandPoolCreateFlags command_pool_create_flags, u32 queue_family_index);
void                                      tg_graphics_vulkan_command_pool_destroy(VkCommandPool command_pool);

VkPipeline                                tg_graphics_vulkan_compute_pipeline_create(VkShaderModule shader_module, VkPipelineLayout pipeline_layout);

VkFormat                                  tg_graphics_vulkan_depth_format_acquire();

VkDescriptorPool                          tg_graphics_vulkan_descriptor_pool_create(VkDescriptorPoolCreateFlags flags, u32 max_sets, u32 pool_size_count, const VkDescriptorPoolSize* pool_sizes);
void                                      tg_graphics_vulkan_descriptor_pool_destroy(VkDescriptorPool descriptor_pool);

VkDescriptorSetLayout                     tg_graphics_vulkan_descriptor_set_layout_create(VkDescriptorSetLayoutCreateFlags flags, u32 binding_count, const VkDescriptorSetLayoutBinding* p_bindings);
void                                      tg_graphics_vulkan_descriptor_set_layout_destroy(VkDescriptorSetLayout descriptor_set_layout);

VkDescriptorSet                           tg_graphics_vulkan_descriptor_set_allocate(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);
void                                      tg_graphics_vulkan_descriptor_set_free(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set);
void                                      tg_graphics_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                                   tg_graphics_vulkan_fence_create(VkFenceCreateFlags fence_create_flags);
void                                      tg_graphics_vulkan_fence_destroy(VkFence fence);
void                                      tg_graphics_vulkan_fence_reset(VkFence fence);
void                                      tg_graphics_vulkan_fence_wait(VkFence fence);

VkFramebuffer                             tg_graphics_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                                      tg_graphics_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);

VkPipeline                                tg_graphics_vulkan_graphics_pipeline_create(const tg_vulkan_graphics_pipeline_create_info* p_vulkan_graphics_pipeline_create_info);
void                                      tg_graphics_vulkan_graphics_pipeline_destroy(VkPipeline pipeline);
void                                      tg_graphics_vulkan_graphics_pipeline_initialize_pipeline_shader_stage_create_infos(VkShaderModule vertex_shader, VkShaderModule fragment_shader, VkPipelineShaderStageCreateInfo* p_pipeline_shader_stage_create_infos);

// TODO: many things regarding images can be put into a struct
void                                      tg_graphics_vulkan_image_create(u32 width, u32 height, u32 mip_levels, VkFormat format, VkSampleCountFlagBits sample_count_flag_bits, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkImage* p_image, VkDeviceMemory* p_device_memory);
void                                      tg_graphics_vulkan_image_destroy(VkImage image, VkDeviceMemory device_memory);
void                                      tg_graphics_vulkan_image_mipmaps_generate(VkImage image, u32 width, u32 height, VkFormat format, u32 mip_levels);
void                                      tg_graphics_vulkan_image_transition_layout(VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, u32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                                      tg_graphics_vulkan_image_view_create(VkImage image, VkFormat format, u32 mip_levels, VkImageAspectFlags image_aspect_flags, VkImageView* p_image_view);
void                                      tg_graphics_vulkan_image_view_destroy(VkImageView image_view);

VkPipelineLayout                          tg_graphics_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges);
void                                      tg_graphics_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);

// TODO: sort
VkPipelineColorBlendAttachmentState       tg_graphics_vulkan_pipeline_color_blend_attachment_state_create(VkBool32 blend_enable);
VkPipelineDepthStencilStateCreateInfo     tg_graphics_vulkan_pipeline_depth_stencil_state_create_info_create(VkBool32 depth_test_enable, VkBool32 depth_write_enable);
VkPipelineMultisampleStateCreateInfo      tg_graphics_vulkan_pipeline_multisample_state_create_info_create(VkSampleCountFlagBits sample_count);
VkPipelineRasterizationStateCreateInfo    tg_graphics_vulkan_pipeline_rasterization_state_create_info_create(VkCullModeFlags cull_mode);

VkRenderPass                              tg_graphics_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                                      tg_graphics_vulkan_render_pass_destroy(VkRenderPass render_pass);

VkSampler                                 tg_graphics_vulkan_sampler_create(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w);
void                                      tg_graphics_vulkan_sampler_destroy(VkSampler sampler);

VkSemaphore                               tg_graphics_vulkan_semaphore_create();
void                                      tg_graphics_vulkan_semaphore_destroy(VkSemaphore semaphore);

VkShaderModule                            tg_graphics_vulkan_shader_module_create(const char* p_filename);
void                                      tg_graphics_vulkan_shader_module_destroy(VkShaderModule shader_module);

VkDeviceSize                              tg_graphics_vulkan_uniform_buffer_min_offset_alignment();

#endif

#endif
