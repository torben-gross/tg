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

#define TG_SURFACE_IMAGE_COUNT 3





typedef struct tg_vulkan_descriptor
{
    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;
} tg_vulkan_descriptor;



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
    VkPipeline               pipeline;
} tg_vulkan_compute_shader;

typedef struct tg_vulkan_image_create_info
{
    u32                      width;
    u32                      height;
    u32                      mip_levels;

    VkFormat                 format;
    VkImageAspectFlags       aspect_mask;
    VkImageLayout            layout;

    VkSampleCountFlagBits    sample_count;
    VkImageUsageFlags        image_usage;
    VkMemoryPropertyFlags    memory_properties;

    VkFilter                 min_filter;
    VkFilter                 mag_filter;
    VkSamplerAddressMode     address_mode_u;
    VkSamplerAddressMode     address_mode_v;
    VkSamplerAddressMode     address_mode_w;
} tg_vulkan_image_create_info;

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
    u32                         input_element_count;
    tg_shader_input_element*    p_input_elements;
    tg_vulkan_compute_shader    compute_shader;
    VkCommandBuffer             command_buffer;
} tg_compute_shader;

typedef struct tg_fragment_shader
{
    VkShaderModule    shader_module;
} tg_fragment_shader;

typedef struct tg_image
{
    u32                   width;
    u32                   height;
    u32                   mip_levels;
    
    VkFormat              format;
    VkImageAspectFlags    aspect_mask;
    VkImageLayout         layout;

    VkImage               image;
    VkDeviceMemory        device_memory;
    void*                 p_mapped_device_memory;
    VkImageView           image_view;
    VkSampler             sampler;
} tg_image;

typedef struct tg_material
{
    tg_vertex_shader_h       vertex_shader_h;
    tg_fragment_shader_h     fragment_shader_h;
    tg_vulkan_descriptor     descriptor;
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
        tg_vulkan_descriptor     descriptor;
        VkPipelineLayout         pipeline_layout;
        VkPipeline               pipeline;
        VkCommandBuffer          command_buffer;
    } render_data;

} tg_model;

typedef struct tg_texture_atlas
{
    tg_image    texture_atlas;
} tg_texture_atlas;

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
VkCommandPool        graphics_command_pool;
VkCommandPool        compute_command_pool;



VkSwapchainKHR       swapchain;
VkExtent2D           swapchain_extent;
VkImage              swapchain_images[TG_SURFACE_IMAGE_COUNT];
VkImageView          swapchain_image_views[TG_SURFACE_IMAGE_COUNT];



void                        tgg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination);
tg_vulkan_buffer            tgg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                        tgg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);

VkCommandBuffer             tgg_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level);
void                        tgg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                        tgg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkSubpassContents subpass_contents);
void                        tgg_vulkan_command_buffer_cmd_clear_image(VkCommandBuffer command_buffer, tg_image* p_image);
void                        tgg_vulkan_command_buffer_cmd_copy_buffer_to_image(VkCommandBuffer command_buffer, VkBuffer source, tg_image* p_destination);
void                        tgg_vulkan_command_buffer_cmd_copy_image_to_buffer(VkCommandBuffer command_buffer, tg_image* p_source, VkImageLayout source_image_layout_on_access, VkBuffer destination);
void                        tgg_vulkan_command_buffer_cmd_transition_image_layout(VkCommandBuffer command_buffer, tg_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                        tgg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue* p_vulkan_queue);
void                        tgg_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer);
void                        tgg_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                        tgg_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

VkPipeline                  tgg_vulkan_compute_pipeline_create(VkShaderModule shader_module, VkPipelineLayout pipeline_layout);
void                        tgg_vulkan_compute_pipeline_destroy(VkPipeline compute_pipeline);

tg_vulkan_compute_shader    tgg_vulkan_compute_shader_create(const char* filename, u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                        tgg_vulkan_compute_shader_destroy(tg_vulkan_compute_shader* p_vulkan_compute_shader);

tg_vulkan_descriptor        tgg_vulkan_descriptor_create(u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings);
void                        tgg_vulkan_descriptor_destroy(tg_vulkan_descriptor* p_vulkan_descriptor);

void                        tgg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_shader_input_element* p_shader_input_element, tg_handle shader_input_element_handle, u32 dst_binding);
void                        tgg_vulkan_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                        tgg_vulkan_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                        tgg_vulkan_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding);
void                        tgg_vulkan_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index);
void                        tgg_vulkan_descriptor_set_update_image(VkDescriptorSet descriptor_set, tg_image* p_image, VkImageLayout image_layout_on_access, u32 dst_binding);
void                        tgg_vulkan_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tg_image* p_image, VkImageLayout image_layout_on_access, u32 dst_binding, u32 array_index);
void                        tgg_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets);

VkFence                     tgg_vulkan_fence_create(VkFenceCreateFlags fence_create_flags);
void                        tgg_vulkan_fence_destroy(VkFence fence);
void                        tgg_vulkan_fence_reset(VkFence fence);
void                        tgg_vulkan_fence_wait(VkFence fence);

VkFramebuffer               tgg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                        tgg_vulkan_framebuffer_destroy(VkFramebuffer framebuffer);

VkPipeline                  tgg_vulkan_graphics_pipeline_create(const tg_vulkan_graphics_pipeline_create_info* p_vulkan_graphics_pipeline_create_info);
void                        tgg_vulkan_graphics_pipeline_destroy(VkPipeline graphics_pipeline);

tg_image                    tgg_vulkan_image_create(const tg_vulkan_image_create_info* p_vulkan_image_create_info);
void                        tgg_vulkan_image_destroy(tg_image* p_image);

VkPipelineLayout            tgg_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges);
void                        tgg_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout);

VkRenderPass                tgg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                        tgg_vulkan_render_pass_destroy(VkRenderPass render_pass);

VkSampler                   tgg_vulkan_sampler_create(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w);
void                        tgg_vulkan_sampler_destroy(VkSampler sampler);

VkSemaphore                 tgg_vulkan_semaphore_create();
void                        tgg_vulkan_semaphore_destroy(VkSemaphore semaphore);

VkDescriptorType            tgg_vulkan_shader_input_element_type_convert(tg_shader_input_element_type type);

VkShaderModule              tgg_vulkan_shader_module_create(const char* p_filename);
void                        tgg_vulkan_shader_module_destroy(VkShaderModule shader_module);

VkDeviceSize                tgg_vulkan_uniform_buffer_min_offset_alignment();

#endif

#endif
