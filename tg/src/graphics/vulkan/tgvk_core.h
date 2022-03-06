#ifndef TGVK_CORE_H
#define TGVK_CORE_H

#include "graphics/tg_graphics_core.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tgvk_common.h"

#include "graphics/tg_spirv.h"
#include "graphics/vulkan/tgvk_memory.h"
#include "memory/tg_memory.h"
#include "util/tg_bitmap.h"



#ifdef TG_DEBUG

#define TGVK_BUFFER_CREATE(size, buffer_usage_flags, memory_type) \
    tgvk_buffer_create(size, buffer_usage_flags, memory_type, __LINE__, __FILE__)

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

#else

#define TGVK_BUFFER_CREATE(size, buffer_usage_flags, memory_type) \
    tgvk_buffer_create(size, buffer_usage_flags, memory_type)

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

#endif

#define TGVK_BUFFER_CREATE_IBO(size) \
    TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE)

#define TGVK_BUFFER_CREATE_STORAGE(size, visible) \
    TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, (visible) ? TGVK_MEMORY_HOST : TGVK_MEMORY_DEVICE)

#define TGVK_BUFFER_CREATE_UBO(size) \
    TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, TGVK_MEMORY_HOST)

#define TGVK_BUFFER_CREATE_VBO(size) \
    TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE)



typedef enum tgvk_command_pool_type
{
    TGVK_COMMAND_POOL_TYPE_COMPUTE,
    TGVK_COMMAND_POOL_TYPE_GRAPHICS,
    TGVK_COMMAND_POOL_TYPE_PRESENT
} tgvk_command_pool_type;

typedef enum tgvk_image_layout_type
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
} tgvk_image_layout_type;

typedef enum tgvk_image_type
{
    TGVK_IMAGE_TYPE_COLOR      = (1 << 0),
    TGVK_IMAGE_TYPE_DEPTH      = (1 << 1),
    TGVK_IMAGE_TYPE_STORAGE    = (1 << 2)
} tgvk_image_type;
typedef u32 tgvk_image_type_flags;

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



typedef struct tgvk_buffer
{
    VkBuffer             buffer;
    tgvk_memory_block    memory;
} tgvk_buffer;

typedef struct tgvk_command_buffer
{
    VkCommandBuffer           buffer;
    tgvk_command_pool_type    pool_type;
    VkCommandBufferLevel      level;
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

typedef struct tgvk_descriptor_set
{
    VkDescriptorPool    pool;
    VkDescriptorSet     set;
} tgvk_descriptor_set;

typedef struct tgvk_framebuffer
{
    u32              width;
    u32              height;
    u32              layers;
    VkFramebuffer    framebuffer; // TODO: this should contain its render_pass, as its needed later anyway (tgvk_cmd_begin_render_pass)
} tgvk_framebuffer;

typedef struct tgvk_shader
{
    tg_spirv_layout    spirv_layout;
    TG_BITMAP32        instanced_input_location_bitmap;
    VkShaderModule     shader_module;
} tgvk_shader;

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

typedef struct tgvk_pipeline_layout
{
    u32                             global_resource_count;
    tg_spirv_global_resource        p_global_resources[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkShaderStageFlags              p_shader_stages[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkDescriptorSetLayoutBinding    p_descriptor_set_layout_bindings[TG_MAX_SHADER_GLOBAL_RESOURCES];
    VkDescriptorSetLayout           descriptor_set_layout;
    VkPipelineLayout                pipeline_layout;
} tgvk_pipeline_layout;

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

typedef struct tgvk_sampler_create_info
{
    tg_image_filter          min_filter;
    tg_image_filter          mag_filter;
    tg_image_address_mode    address_mode_u;
    tg_image_address_mode    address_mode_v;
    tg_image_address_mode    address_mode_w;
} tgvk_sampler_create_info;

typedef struct tgvk_staging_buffer
{
    tgvk_buffer*    p_staging_buffer;
    tg_size         size_to_copy;     // The number of bytes to copy in total
    tg_size         dst_offset;       // The offset in bytes into the destination buffer
    tgvk_buffer*    p_dst;
    tg_size         copied_size;      // The total number of bytes copied into the destination buffer
    tg_size         filled_size;      // The number of bytes currently filled into the staging buffer
} tgvk_staging_buffer;

typedef struct tgvk_surface
{
    VkSurfaceKHR          surface;
    VkSurfaceFormatKHR    format;
} tgvk_surface;



VkInstance                      instance;
tgvk_surface                    surface;
VkPhysicalDevice                physical_device;
VkDevice                        device;

VkSwapchainKHR                  swapchain;
VkExtent2D                      swapchain_extent;
VkImage                         p_swapchain_images[TG_MAX_SWAPCHAIN_IMAGES];
VkImageView                     p_swapchain_image_views[TG_MAX_SWAPCHAIN_IMAGES];

tgvk_buffer                     screen_quad_ibo;
tgvk_buffer                     screen_quad_positions_vbo;
tgvk_buffer                     screen_quad_uvs_vbo;



void                    tgvk_buffer_copy(VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
void                    tgvk_buffer_copy2(VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
tgvk_buffer             tgvk_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, tgvk_memory_type memory_type TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
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
void                    tgvk_cmd_copy_buffer2(tgvk_command_buffer* p_command_buffer, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst);
void                    tgvk_cmd_copy_buffer_to_cube_map(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_cube_map* p_destination);
void                    tgvk_cmd_copy_buffer_to_image(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_buffer_to_image2(tgvk_command_buffer* p_command_buffer, u32 image_offset_x, u32 image_offset_y, u32 image_extent_x, u32 image_extent_y, tgvk_buffer* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_buffer_to_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image_3d* p_destination);
void                    tgvk_cmd_copy_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination);
void                    tgvk_cmd_copy_color_image_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination);
void                    tgvk_cmd_copy_depth_image_pixel_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination, u32 x, u32 y);
void                    tgvk_cmd_copy_image_3d_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_source, tgvk_buffer* p_destination);
void                    tgvk_cmd_draw_indexed(tgvk_command_buffer* p_command_buffer, u32 index_count);
void                    tgvk_cmd_draw_indexed_instanced(tgvk_command_buffer* p_command_buffer, u32 index_count, u32 instance_count);
void                    tgvk_cmd_draw_instanced(tgvk_command_buffer* p_command_buffer, u32 vertex_count, u32 instance_count);
void                    tgvk_cmd_transition_cube_map_layout(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type);
void                    tgvk_cmd_transition_cube_map_layout2(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type);
void                    tgvk_cmd_transition_image_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_image_3d_layout(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type);
void                    tgvk_cmd_transition_image_3d_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits);
void                    tgvk_cmd_transition_layered_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type);
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

tgvk_cube_map           tgvk_cube_map_create(u32 dimension, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_cube_map_destroy(tgvk_cube_map* p_cube_map);

tgvk_descriptor_set     tgvk_descriptor_set_create(const tgvk_pipeline* p_pipeline);
void                    tgvk_descriptor_set_destroy(tgvk_descriptor_set* p_descriptor_set);

void                    tgvk_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tgvk_cube_map* p_cube_map, u32 dst_binding);
void                    tgvk_descriptor_set_update_image(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding);
void                    tgvk_descriptor_set_update_image2(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, VkDescriptorType descriptor_type);
void                    tgvk_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, u32 array_index);
void                    tgvk_descriptor_set_update_image_3d(VkDescriptorSet descriptor_set, tgvk_image_3d* p_image_3d, u32 dst_binding);
void                    tgvk_descriptor_set_update_layered_image(VkDescriptorSet descriptor_set, tgvk_layered_image* p_image, u32 dst_binding);
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
tg_size                 tgvk_global_staging_buffer_size(void);

tgvk_image              tgvk_image_create(tgvk_image_type_flags type_flags, u32 width, u32 height, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
tgvk_image              tgvk_image_create2(tgvk_image_type type, const char* p_filename, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2));
void                    tgvk_image_destroy(tgvk_image* p_image);
b32                     tgvk_image_serialize(tgvk_image* p_image, const char* p_filename);
b32                     tgvk_image_deserialize(const char* p_filename, TG_OUT tgvk_image* p_image TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2));
b32                     tgvk_image_store_to_disc(tgvk_image* p_image, const char* p_filename, b32 force_alpha_one, b32 replace_existing);
void                    tgvk_image_set_data(tgvk_image* p_image, tg_size size, void* p_data);

tgvk_image_3d           tgvk_image_3d_create(tgvk_image_type type, u32 width, u32 height, u32 depth, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                    tgvk_image_3d_destroy(tgvk_image_3d* p_image_3d);
b32                     tgvk_image_3d_store_slice_to_disc(tgvk_image_3d* p_image_3d, u32 slice_depth, const char* p_filename, b32 force_alpha_one, b32 replace_existing);

tgvk_layered_image      tgvk_layered_image_create(tgvk_image_type type, u32 width, u32 height, u32 layers, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
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

tgvk_sampler            tgvk_sampler_create(void);
tgvk_sampler            tgvk_sampler_create2(const tgvk_sampler_create_info* p_sampler_create_info);
void                    tgvk_sampler_destroy(tgvk_sampler* p_sampler);

VkSemaphore             tgvk_semaphore_create(void);
void                    tgvk_semaphore_destroy(VkSemaphore semaphore);

tgvk_shader             tgvk_shader_create(const char* p_filename);
tgvk_shader             tgvk_shader_create_from_glsl(tg_shader_type type, const char* p_source);
tgvk_shader             tgvk_shader_create_from_spirv(u32 size, const char* p_source);
void                    tgvk_shader_destroy(tgvk_shader* p_shader);

void                    tgvk_staging_buffer_take(tg_size size, tgvk_buffer* p_dst, TG_OUT tgvk_staging_buffer* p_staging_buffer);
void                    tgvk_staging_buffer_take2(tg_size size, tg_size dst_offset, tgvk_buffer* p_dst, TG_OUT tgvk_staging_buffer* p_staging_buffer);
void                    tgvk_staging_buffer_reinit(tgvk_staging_buffer* p_staging_buffer, tg_size size, tgvk_buffer* p_dst);
void                    tgvk_staging_buffer_reinit2(tgvk_staging_buffer* p_staging_buffer, tg_size size, tg_size dst_offset, tgvk_buffer* p_dst);
void                    tgvk_staging_buffer_push(tgvk_staging_buffer* p_staging_buffer, tg_size size, const void* p_data);
void                    tgvk_staging_buffer_push_u8(tgvk_staging_buffer* p_staging_buffer, u8 v);
void                    tgvk_staging_buffer_push_u32(tgvk_staging_buffer* p_staging_buffer, u32 v);
void                    tgvk_staging_buffer_release(tgvk_staging_buffer* p_staging_buffer);

void                    tgvk_util_copy_to_buffer(tg_size size, const void* p_data, tgvk_buffer* p_buffer);
void                    tgvk_util_copy_to_image(tgvk_image_layout_type old_layout, tgvk_image_layout_type new_layout, tg_size size, const void* p_data, tgvk_image* p_image);

#endif

#endif
