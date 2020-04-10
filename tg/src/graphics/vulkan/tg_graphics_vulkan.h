#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include <vulkan/vulkan.h>



#ifdef TG_DEBUG
#define VK_CALL(x) TG_ASSERT(x == VK_SUCCESS)
#else
#define VK_CALL(x) x
#endif

#ifdef TG_WIN32

#ifdef TG_DEBUG
#define INSTANCE_EXTENSION_COUNT 3
#define INSTANCE_EXTENSION_NAMES (char*[]){ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#else
#define INSTANCE_EXTENSION_COUNT 2
#define INSTANCE_EXTENSION_NAMES (char*[]){ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#endif

#else

#define INSTANCE_EXTENSION_COUNT 0
#define INSTANCE_EXTENSION_NAMES TG_NULL

#endif

#define DEVICE_EXTENSION_COUNT 1
#define DEVICE_EXTENSION_NAMES (char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define VALIDATION_LAYER_COUNT 1
#define VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_KHRONOS_validation" }
#else
#define VALIDATION_LAYER_COUNT 0
#define VALIDATION_LAYER_NAMES TG_NULL
#endif

#define SURFACE_IMAGE_COUNT 3



typedef struct tg_material
{
    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } ubo;

    tg_vertex_shader_h       vertex_shader;
    tg_fragment_shader_h     fragment_shader;
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;
} tg_material;

typedef struct tg_mesh
{
    struct
    {
        u32               vertex_count;
        VkBuffer          buffer;
        VkDeviceMemory    device_memory;
    } vbo;

    struct
    {
        u32               index_count;
        VkBuffer          buffer;
        VkDeviceMemory    device_memory;
    } ibo;
} tg_mesh;

typedef struct tg_model
{
    tg_mesh_h          mesh;
    tg_material_h      material;
    VkCommandPool      command_pool;
    VkCommandBuffer    command_buffer;
} tg_model;



typedef struct tg_surface
{
    VkSurfaceKHR             surface;
    VkSurfaceFormatKHR       format;
    VkSampleCountFlagBits    msaa_sample_count;
} tg_surface;

typedef struct tg_queue
{
    u8         index;
    VkQueue    queue;
} tg_queue;

typedef struct tg_vulkan_image_create_info
{
    VkFormat                 format;
    u32                      width;
    u32                      height;
    u32                      mip_levels;
    VkSampleCountFlagBits    sample_count_flag_bits;
    VkImageUsageFlags        image_usage_flags;
    VkMemoryPropertyFlags    memory_property_flags;
    VkImageLayout            image_layout;
} tg_vulkan_image_create_info;
/*
TODO: use above for image creation
*/



VkInstance                  instance;
tg_surface                  surface;
VkPhysicalDevice            physical_device;
VkDevice                    device;
tg_queue                    graphics_queue;
tg_queue                    present_queue;
VkCommandPool               command_pool;

#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT    debug_utils_messenger;
#endif



VkSwapchainKHR              swapchain;
VkExtent2D                  swapchain_extent;
VkImage                     swapchain_images[SURFACE_IMAGE_COUNT];
VkImageView                 swapchain_image_views[SURFACE_IMAGE_COUNT];



/*------------------------------------------------------------+
| General utilities                                           |
+------------------------------------------------------------*/

void                     tg_graphics_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer target);
void                     tg_graphics_vulkan_buffer_copy_to_image(u32 width, u32 height, VkBuffer source, VkImage target);
void                     tg_graphics_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer* p_buffer, VkDeviceMemory* p_device_memory);
void                     tg_graphics_vulkan_buffer_destroy(VkBuffer buffer, VkDeviceMemory device_memory);
void                     tg_graphics_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, VkCommandBuffer* p_command_buffer);
void                     tg_graphics_vulkan_command_buffer_begin(VkCommandBufferUsageFlags usage_flags, VkCommandBuffer command_buffer);
void                     tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, u32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                     tg_graphics_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer);
void                     tg_graphics_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void                     tg_graphics_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                     tg_graphics_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);
VkFormat                 tg_graphics_vulkan_depth_format_acquire();
void                     tg_graphics_vulkan_descriptor_pool_create(VkDescriptorPoolCreateFlags flags, u32 max_sets, u32 pool_size_count, const VkDescriptorPoolSize* pool_sizes, VkDescriptorPool* p_descriptor_pool);
void                     tg_graphics_vulkan_descriptor_pool_destroy(VkDescriptorPool descriptor_pool);
void                     tg_graphics_vulkan_descriptor_set_layout_create(VkDescriptorSetLayoutCreateFlags flags, u32 binding_count, const VkDescriptorSetLayoutBinding* p_bindings, VkDescriptorSetLayout* p_descriptor_set_layout);
void                     tg_graphics_vulkan_descriptor_set_layout_destroy(VkDescriptorSetLayout descriptor_set_layout);
void                     tg_graphics_vulkan_descriptor_set_allocate(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet* p_descriptor_set);
void                     tg_graphics_vulkan_descriptor_set_free(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set);
void                     tg_graphics_vulkan_fence_create(VkFenceCreateFlags fence_create_flags, VkFence* p_fence);
void                     tg_graphics_vulkan_fence_destroy(VkFence fence);
void                     tg_graphics_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height, VkFramebuffer* p_framebuffer);
void                     tg_graphics_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);
void                     tg_graphics_vulkan_image_create(u32 width, u32 height, u32 mip_levels, VkFormat format, VkSampleCountFlagBits sample_count_flag_bits, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkImage* p_image, VkDeviceMemory* p_device_memory);
void                     tg_graphics_vulkan_image_destroy(VkImage image, VkDeviceMemory device_memory);
void                     tg_graphics_vulkan_image_mipmaps_generate(VkImage image, u32 width, u32 height, VkFormat format, u32 mip_levels);
void                     tg_graphics_vulkan_image_transition_layout(VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, u32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                     tg_graphics_vulkan_image_view_create(VkImage image, VkFormat format, u32 mip_levels, VkImageAspectFlags image_aspect_flags, VkImageView* p_image_view);
void                     tg_graphics_vulkan_image_view_destroy(VkImageView image_view);
u32                      tg_graphics_vulkan_memory_type_find(u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags);
b32                      tg_graphics_vulkan_physical_device_check_extension_support(VkPhysicalDevice physical_device);
b32                      tg_graphics_vulkan_physical_device_find_queue_families(VkPhysicalDevice physical_device, tg_queue* p_graphics_queue, tg_queue* p_present_queue);
VkSampleCountFlagBits    tg_graphics_vulkan_physical_device_find_max_sample_count(VkPhysicalDevice physical_device);
b32                      tg_graphics_vulkan_physical_device_is_suitable(VkPhysicalDevice physical_device);
void                     tg_graphics_vulkan_pipeline_create(VkPipelineCache pipeline_cache, const VkGraphicsPipelineCreateInfo* p_graphics_pipeline_create_info, VkPipeline* p_pipeline);
void                     tg_graphics_vulkan_pipeline_destroy(VkPipeline pipeline);
void                     tg_graphics_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges, VkPipelineLayout* p_pipeline_layout);
void                     tg_graphics_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);
void                     tg_graphics_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies, VkRenderPass* p_render_pass);
void                     tg_graphics_vulkan_render_pass_destroy(VkRenderPass render_pass);
void                     tg_graphics_vulkan_sampler_create(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w, VkSampler* p_sampler);
void                     tg_graphics_vulkan_sampler_destroy(VkSampler sampler);
void                     tg_graphics_vulkan_semaphore_create(VkSemaphore* p_semaphore);
void                     tg_graphics_vulkan_semaphore_destroy(VkSemaphore semaphore);
void                     tg_graphics_vulkan_shader_module_create(const char* p_filename, VkShaderModule* p_shader_module);
void                     tg_graphics_vulkan_shader_module_destroy(VkShaderModule shader_module);



/*------------------------------------------------------------+
| Renderer 3D                                                 |
+------------------------------------------------------------*/

void                     tg_graphics_vulkan_renderer_3d_get_geometry_framebuffer(VkFramebuffer* p_framebuffer);
void                     tg_graphics_vulkan_renderer_3d_get_geometry_render_pass(VkRenderPass* p_render_pass);



#endif

#endif
