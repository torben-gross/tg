#ifndef TG_GRAPHICS_VULKAN_H
#define TG_GRAPHICS_VULKAN_H

#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include "graphics/tg_spirv.h"
#include "graphics/vulkan/tg_vulkan_memory_allocator.h"
#include "platform/tg_platform.h"
#include <vulkan/vulkan.h>
#undef near
#undef far
#undef min
#undef max

#ifdef TG_DEBUG
#define VK_CALL(x) TG_ASSERT(x == VK_SUCCESS)
#else
#define VK_CALL(x) x
#endif

#define TG_VULKAN_SURFACE_IMAGE_COUNT    3
#define TG_VULKAN_COLOR_IMAGE_FORMAT     VK_FORMAT_R8G8B8A8_SRGB
#define TG_VULKAN_COLOR_IMAGE_LAYOUT     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
#define TG_VULKAN_DEPTH_IMAGE_LAYOUT     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL



#define TG_VULKAN_MAX_LOD_COUNT     8



#define TG_VULKAN_TAKE_HANDLE(p_array, p_result)                                   \
    const u32 array_element_count = sizeof(p_array) / sizeof(*p_array);            \
    for (u32 handle_index = 0; handle_index < array_element_count; handle_index++) \
    {                                                                              \
        if (p_array[handle_index].type == TG_HANDLE_TYPE_INVALID)                  \
        {                                                                          \
            p_result = &p_array[handle_index];                                     \
            break;                                                                 \
        }                                                                          \
    }                                                                              \
    TG_ASSERT(p_result)

#define TG_VULKAN_RELEASE_HANDLE(handle)                             TG_ASSERT(handle && handle->type != TG_HANDLE_TYPE_INVALID); handle->type = TG_HANDLE_TYPE_INVALID



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
    u64                       size;
    VkBuffer                  buffer;
    tg_vulkan_memory_block    memory;
} tg_vulkan_buffer;

typedef struct tg_vulkan_shader
{
    tg_spirv_layout    spirv_layout;
    VkShaderModule     shader_module;
} tg_vulkan_shader;

typedef struct tg_vulkan_framebuffer
{
    u32              width;
    u32              height;
    VkFramebuffer    framebuffer;
} tg_vulkan_framebuffer;

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
    tg_lock    lock;
    u32        index;
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
    tg_vulkan_shader            vulkan_shader;
    tg_vulkan_pipeline          compute_pipeline;
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

typedef struct tg_vulkan_render_command_renderer_info
{
    tg_renderer_h         h_renderer;
    tg_vulkan_pipeline    graphics_pipeline;
    VkCommandBuffer       command_buffer;
    tg_vulkan_pipeline    p_shadow_graphics_pipelines[TG_CASCADED_SHADOW_MAPS];
    VkCommandBuffer       p_shadow_command_buffers[TG_CASCADED_SHADOW_MAPS];
} tg_vulkan_render_command_renderer_info;

typedef struct tg_render_command
{
    tg_handle_type                            type;
    tg_mesh_h                                 h_mesh;
    tg_material_h                             h_material;
    tg_vulkan_buffer                          model_ubo;
    u32                                       renderer_info_count;
    tg_vulkan_render_command_renderer_info    p_renderer_infos[TG_MAX_RENDERERS];
} tg_render_command;

typedef struct tg_render_target
{
    tg_handle_type    type;
    tg_color_image    color_attachment;
    tg_depth_image    depth_attachment;
    tg_color_image    color_attachment_copy;
    tg_depth_image    depth_attachment_copy;
    VkFence           fence;
} tg_render_target;

typedef struct tg_fragment_shader
{
    tg_handle_type      type;
    tg_vulkan_shader    vulkan_shader;
} tg_fragment_shader;

typedef struct tg_material
{
    tg_handle_type             type;
    tg_vulkan_material_type    material_type;
    tg_vertex_shader_h         h_vertex_shader;
    tg_fragment_shader_h       h_fragment_shader;
} tg_material;

typedef struct tg_mesh
{
    tg_handle_type      type;
    tg_bounds           bounds;
    u32                 vertex_count;
    tg_vulkan_buffer    vbo;
    u32                 index_count;
    tg_vulkan_buffer    ibo;
} tg_mesh;

typedef struct tg_renderer
{
    tg_handle_type               type;
    const tg_camera*             p_camera;
    tg_vulkan_buffer             view_projection_ubo;
    tg_render_target             render_target;

    u32                          render_command_count;
    tg_render_command_h          ph_render_commands[TG_MAX_RENDER_COMMANDS];

    struct
    {
        b32                      enabled;
        tg_depth_image           p_shadow_maps[TG_CASCADED_SHADOW_MAPS];
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    p_framebuffers[TG_CASCADED_SHADOW_MAPS];
        tg_vulkan_pipeline       graphics_pipeline;
        VkCommandBuffer          command_buffer;
        tg_vulkan_buffer         p_lightspace_ubos[TG_CASCADED_SHADOW_MAPS];
    } shadow_pass;

    struct
    {
        tg_color_image           p_color_attachments[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT];
        VkRenderPass             render_pass;
        tg_vulkan_framebuffer    framebuffer;
        VkCommandBuffer          command_buffer;
    } geometry_pass;

    struct
    {
        tg_color_image           color_attachment;
        tg_vulkan_buffer         vbo;
        tg_vulkan_buffer         ibo;
        VkSemaphore              rendering_finished_semaphore;
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
        tg_vulkan_buffer         vbo;
        tg_vulkan_buffer         ibo;
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
} tg_renderer;

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
    tg_handle_type      type;
    tg_vulkan_shader    vulkan_shader;
} tg_vertex_shader;



VkInstance            instance;
tg_vulkan_surface     surface;
VkPhysicalDevice      physical_device;
VkDevice              device;



VkSwapchainKHR        swapchain;
VkExtent2D            swapchain_extent;
VkImage               p_swapchain_images[TG_VULKAN_SURFACE_IMAGE_COUNT];
VkImageView           p_swapchain_image_views[TG_VULKAN_SURFACE_IMAGE_COUNT];



tg_compute_shader     p_compute_shaders[TG_MAX_COMPUTE_SHADERS];
tg_fragment_shader    p_fragment_shaders[TG_MAX_FRAGMENT_SHADERS];
tg_material           p_materials[TG_MAX_MATERIALS];
tg_mesh               p_meshes[TG_MAX_MESHES];
tg_render_command     p_render_commands[TG_MAX_RENDER_COMMANDS];
tg_renderer           p_renderers[TG_MAX_RENDERERS];
tg_storage_buffer     p_storage_buffers[TG_MAX_STORAGE_BUFFERS];
tg_uniform_buffer     p_uniform_buffers[TG_MAX_UNIFORM_BUFFERS];
tg_vertex_shader      p_vertex_shaders[TG_MAX_VERTEX_SHADERS];



void                          tg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination);
tg_vulkan_buffer              tg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags);
void                          tg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer);
void                          tg_vulkan_buffer_flush_mapped_memory(tg_vulkan_buffer* p_vulkan_buffer);

tg_color_image                tg_vulkan_color_image_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info);
VkFormat                      tg_vulkan_color_image_convert_format(tg_color_image_format format);
void                          tg_vulkan_color_image_destroy(tg_color_image* p_color_image);

VkCommandBuffer               tg_vulkan_command_buffer_allocate(tg_vulkan_command_pool_type type, VkCommandBufferLevel level);
void                          tg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info);
void                          tg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, tg_vulkan_framebuffer* p_framebuffer, VkSubpassContents subpass_contents);
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
void                          tg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue_type type);
void                          tg_vulkan_command_buffer_free(tg_vulkan_command_pool_type type, VkCommandBuffer command_buffer); // TODO: save command pool in command buffers?
void                          tg_vulkan_command_buffers_allocate(tg_vulkan_command_pool_type type, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers);
void                          tg_vulkan_command_buffers_free(tg_vulkan_command_pool_type type, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers);

tg_depth_image                tg_vulkan_depth_image_create(const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info);
VkFormat                      tg_vulkan_depth_image_convert_format(tg_depth_image_format format);
void                          tg_vulkan_depth_image_destroy(tg_depth_image* p_depth_image);

void                          tg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle shader_input_element_handle, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_color_image(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_color_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding, u32 array_index);
void                          tg_vulkan_descriptor_set_update_depth_image(VkDescriptorSet descriptor_set, tg_depth_image* p_depth_image, u32 dst_binding);
void                          tg_vulkan_descriptor_set_update_depth_image_array(VkDescriptorSet descriptor_set, tg_depth_image* p_depth_image, u32 dst_binding, u32 array_index);
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

tg_vulkan_framebuffer         tg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height);
void                          tg_vulkan_framebuffer_destroy(tg_vulkan_framebuffer* p_framebuffer);
void                          tg_vulkan_framebuffers_destroy(u32 count, tg_vulkan_framebuffer* p_framebuffers);

VkDescriptorType              tg_vulkan_handle_type_convert_to_descriptor_type(tg_handle_type type);

VkFilter                      tg_vulkan_image_convert_filter(tg_image_filter filter);
VkSamplerAddressMode          tg_vulkan_image_convert_address_mode(tg_image_address_mode address_mode);

VkPhysicalDeviceProperties    tg_vulkan_physical_device_get_properties();

tg_vulkan_pipeline            tg_vulkan_pipeline_create_compute(const tg_vulkan_shader* p_compute_shader);
tg_vulkan_pipeline            tg_vulkan_pipeline_create_graphics(const tg_vulkan_graphics_pipeline_create_info* p_create_info);
tg_vulkan_pipeline            tg_vulkan_pipeline_create_graphics2(const tg_vulkan_graphics_pipeline_create_info* p_create_info, VkVertexInputBindingDescription vertex_bindings, u32 vertex_attrib_count, VkVertexInputAttributeDescription* p_vertex_attribs);
void                          tg_vulkan_pipeline_destroy(tg_vulkan_pipeline* p_pipeline);

void                          tg_vulkan_queue_present(VkPresentInfoKHR* p_present_info);
void                          tg_vulkan_queue_submit(tg_vulkan_queue_type type, u32 submit_count, VkSubmitInfo* p_submit_infos, VkFence fence);
void                          tg_vulkan_queue_wait_idle(tg_vulkan_queue_type type);

VkRenderPass                  tg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies);
void                          tg_vulkan_render_pass_destroy(VkRenderPass render_pass);

tg_render_target              tg_vulkan_render_target_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info, const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info, VkFenceCreateFlags fence_create_flags);
void                          tg_vulkan_render_target_destroy(tg_render_target* p_render_target);

VkSemaphore                   tg_vulkan_semaphore_create();
void                          tg_vulkan_semaphore_destroy(VkSemaphore semaphore);

tg_vulkan_shader              tg_vulkan_shader_create(const char* p_filename);
void                          tg_vulkan_shader_destroy(tg_vulkan_shader* p_vulkan_shader);

tg_storage_image_3d           tg_vulkan_storage_image_3d_create(u32 width, u32 height, u32 depth, VkFormat format);
void                          tg_vulkan_storage_image_3d_destroy(tg_storage_image_3d* p_storage_image_3d);

VkFormat                      tg_vulkan_storage_image_convert_format(tg_storage_image_format format);

#endif

#endif
