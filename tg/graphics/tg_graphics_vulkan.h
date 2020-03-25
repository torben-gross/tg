#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "tg/graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include <stdbool.h>
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
#define INSTANCE_EXTENSION_NAMES NULL

#endif

#define DEVICE_EXTENSION_COUNT 1
#define DEVICE_EXTENSION_NAMES (char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define VALIDATION_LAYER_COUNT 1
#define VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_KHRONOS_validation" }
#else
#define VALIDATION_LAYER_COUNT 0
#define VALIDATION_LAYER_NAMES NULL
#endif

#define FRAMES_IN_FLIGHT 2
#define QUEUE_INDEX_COUNT 2
#define MAX_PRESENT_MODE_COUNT 9
#define SURFACE_IMAGE_COUNT 3



typedef struct tg_fragment_shader
{
    VkShaderModule shader_module;
} tg_fragment_shader;

typedef struct tg_image
{
    ui32               width;
    ui32               height;
    tg_image_format    format;
    ui32*              data;
    VkImage            image;
    VkDeviceMemory     device_memory;
    VkImageView        image_view;
    VkSampler          sampler;
} tg_image;

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
        ui32           vertex_count;
        VkBuffer       buffer;
        VkDeviceMemory device_memory;
    } vbo;

    struct
    {
        ui32           index_count;
        VkBuffer       buffer;
        VkDeviceMemory device_memory;
    } ibo;
} tg_mesh;

typedef struct tg_model
{
    tg_mesh_h mesh;
    tg_material_h material;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
} tg_model;

typedef struct tg_vertex_shader
{
    VkShaderModule shader_module;
} tg_vertex_shader;



typedef struct tg_surface
{
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR format;
    VkSampleCountFlagBits msaa_sample_count;
} tg_surface;

typedef struct tg_queue
{
    ui8        index;
    VkQueue    queue;
} tg_queue;

typedef struct tg_vulkan_image_create_info
{
    VkFormat                 format;
    ui32                     width;
    ui32                     height;
    ui32                     mip_levels;
    VkSampleCountFlagBits    sample_count_flag_bits;
    VkImageUsageFlags        image_usage_flags;
    VkMemoryPropertyFlags    memory_property_flags;
    VkImageLayout            image_layout;
} tg_vulkan_image_create_info;
/*
TODO: use above for image creation. also: make image_transition_layout to
tg_graphics_vulkan_command_buffer_cmd_transition_layout and use accordingly (do
not create command buffer in there)
*/



VkInstance instance;
tg_surface surface;
VkPhysicalDevice physical_device;
VkDevice device;
tg_queue graphics_queue;
tg_queue present_queue;
VkCommandPool command_pool;

#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT debug_utils_messenger;
#endif



VkSwapchainKHR swapchain;
VkExtent2D swapchain_extent;
VkImage swapchain_images[SURFACE_IMAGE_COUNT];
VkImageView swapchain_image_views[SURFACE_IMAGE_COUNT];



void tg_graphics_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer target);
void tg_graphics_vulkan_buffer_copy_to_image(ui32 width, ui32 height, VkBuffer source, VkImage target);
void tg_graphics_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer* p_buffer, VkDeviceMemory* p_device_memory);
void tg_graphics_vulkan_buffer_destroy(VkBuffer buffer, VkDeviceMemory device_memory);
void tg_graphics_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, VkCommandBuffer* p_command_buffer);
void tg_graphics_vulkan_command_buffer_begin(VkCommandBufferUsageFlags usage_flags, VkCommandBuffer command_buffer);
void tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, ui32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void tg_graphics_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer);
void tg_graphics_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void tg_graphics_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, ui32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void tg_graphics_vulkan_command_buffers_free(VkCommandPool command_pool, ui32 command_buffer_count, const VkCommandBuffer* p_command_buffers);
void tg_graphics_vulkan_depth_format_acquire(VkFormat* p_format);
void tg_graphics_vulkan_descriptor_pool_create(VkDescriptorPoolCreateFlags flags, ui32 max_sets, ui32 pool_size_count, const VkDescriptorPoolSize* pool_sizes, VkDescriptorPool* p_descriptor_pool);
void tg_graphics_vulkan_descriptor_pool_destroy(VkDescriptorPool descriptor_pool);
void tg_graphics_vulkan_descriptor_set_layout_create(VkDescriptorSetLayoutCreateFlags flags, ui32 binding_count, const VkDescriptorSetLayoutBinding* bindings, VkDescriptorSetLayout* p_descriptor_set_layout);
void tg_graphics_vulkan_descriptor_set_layout_destroy(VkDescriptorSetLayout descriptor_set_layout);
void tg_graphics_vulkan_descriptor_set_allocate(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet* p_descriptor_set);
void tg_graphics_vulkan_descriptor_set_free(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set);
void tg_graphics_vulkan_fence_create(VkFenceCreateFlags fence_create_flags, VkFence* p_fence);
void tg_graphics_vulkan_fence_destroy(VkFence fence);
void tg_graphics_vulkan_framebuffer_create(VkRenderPass render_pass, ui32 attachment_count, const VkImageView* attachments, ui32 width, ui32 height, VkFramebuffer* p_framebuffer);
void tg_graphics_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);
void tg_graphics_vulkan_image_create(ui32 width, ui32 height, ui32 mip_levels, VkFormat format, VkSampleCountFlagBits sample_count_flag_bits, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkImage* p_image, VkDeviceMemory* p_device_memory);
void tg_graphics_vulkan_image_destroy(VkImage image, VkDeviceMemory device_memory);
void tg_graphics_vulkan_image_mipmaps_generate(VkImage image, ui32 width, ui32 height, VkFormat format, ui32 mip_levels);
void tg_graphics_vulkan_image_transition_layout(VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask, ui32 mip_levels, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void tg_graphics_vulkan_image_view_create(VkImage image, VkFormat format, ui32 mip_levels, VkImageAspectFlags image_aspect_flags, VkImageView* p_image_view);
void tg_graphics_vulkan_image_view_destroy(VkImageView image_view);
void tg_graphics_vulkan_memory_type_find(ui32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags, ui32* p_memory_type);
void tg_graphics_vulkan_physical_device_check_extension_support(VkPhysicalDevice physical_device, bool* p_result);
void tg_graphics_vulkan_physical_device_find_queue_families(VkPhysicalDevice physical_device, tg_queue* p_graphics_queue, tg_queue* p_present_queue, bool* p_complete);
void tg_graphics_vulkan_physical_device_find_max_sample_count(VkPhysicalDevice physical_device, VkSampleCountFlagBits* p_max_sample_count_flag_bits);
void tg_graphics_vulkan_physical_device_is_suitable(VkPhysicalDevice physical_device, bool* p_is_suitable);
void tg_graphics_vulkan_pipeline_create(VkPipelineCache pipeline_cache, const VkGraphicsPipelineCreateInfo* p_graphics_pipeline_create_info, VkPipeline* p_pipeline);
void tg_graphics_vulkan_pipeline_destroy(VkPipeline pipeline);
void tg_graphics_vulkan_pipeline_layout_create(ui32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, ui32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges, VkPipelineLayout* p_pipeline_layout);
void tg_graphics_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);
void tg_graphics_vulkan_render_pass_create(ui32 attachment_count, const VkAttachmentDescription* attachments, ui32 subpass_count, const VkSubpassDescription* subpasses, ui32 dependency_count, const VkSubpassDependency* dependencies, VkRenderPass* p_render_pass);
void tg_graphics_vulkan_render_pass_destroy(VkRenderPass render_pass);
void tg_graphics_vulkan_sampler_create(ui32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w, VkSampler* p_sampler);
void tg_graphics_vulkan_sampler_destroy(VkSampler sampler);
void tg_graphics_vulkan_semaphore_create(VkSemaphore* p_semaphore);
void tg_graphics_vulkan_semaphore_destroy(VkSemaphore semaphore);
void tg_graphics_vulkan_shader_module_create(const char* filename, VkShaderModule* p_shader_module);
void tg_graphics_vulkan_shader_module_destroy(VkShaderModule shader_module);



#endif

#endif
