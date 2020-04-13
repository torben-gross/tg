#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"
#include "util/tg_list.h"
#include "tg_entity_internal.h"



#define TG_RENDERER_3D_MAX_ENTITY_COUNT                        65536

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



typedef struct tg_renderer_3d_screen_vertex
{
    v2    position;
    v2    uv;
} tg_renderer_3d_screen_vertex;



typedef struct tg_renderer_3d_attachment
{
    VkImage           image;
    VkDeviceMemory    device_memory;
    VkImageView       image_view;
    VkSampler         sampler;
} tg_renderer_3d_attachment;

typedef struct tg_renderer_3d_buffer
{
    VkBuffer          buffer;
    VkDeviceMemory    device_memory;
} tg_renderer_3d_buffer;

typedef struct tg_camera_uniform_buffer
{
    m4    view;
    m4    projection;
} tg_camera_uniform_buffer;



typedef struct tg_renderer_3d_geometry_pass
{
    tg_renderer_3d_attachment    position_attachment;
    tg_renderer_3d_attachment    normal_attachment;
    tg_renderer_3d_attachment    albedo_attachment;
    tg_renderer_3d_attachment    depth_attachment;

    VkFence                      rendering_finished_fence;
    VkSemaphore                  rendering_finished_semaphore;

    tg_renderer_3d_buffer        model_ubo;
    u64                          model_ubo_element_size;
    u8*                          model_ubo_mapped_memory;
    tg_renderer_3d_buffer        view_projection_ubo;
    tg_camera_uniform_buffer*    view_projection_ubo_mapped_memory;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    tg_list_h                    models;
    VkCommandBuffer              main_command_buffer;
    tg_list_h                    secondary_command_buffers;
} tg_renderer_3d_geometry_pass;

typedef struct tg_renderer_3d_shading_pass
{
    tg_renderer_3d_attachment    color_attachment;

    tg_renderer_3d_buffer        vbo;
    tg_renderer_3d_buffer        ibo;

    VkFence                      rendering_finished_fence;
    VkSemaphore                  rendering_finished_semaphore;
    VkFence                      geometry_pass_attachments_cleared_fence;

    VkRenderPass                 render_pass;
    VkFramebuffer                framebuffer;

    VkDescriptorPool             descriptor_pool;
    VkDescriptorSetLayout        descriptor_set_layout;
    VkDescriptorSet              descriptor_set;

    VkShaderModule               vertex_shader_h;
    VkShaderModule               fragment_shader_h;
    VkPipelineLayout             pipeline_layout;
    VkPipeline                   pipeline;

    VkCommandBuffer              command_buffer;
} tg_renderer_3d_shading_pass;

typedef struct tg_renderer_3d_present_pass
{
    tg_renderer_3d_buffer    vbo;
    tg_renderer_3d_buffer    ibo;

    VkSemaphore              image_acquired_semaphore;
    VkFence                  rendering_finished_fence;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffers[SURFACE_IMAGE_COUNT];

    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;

    VkShaderModule           vertex_shader_h;
    VkShaderModule           fragment_shader_h;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;

    VkCommandBuffer          command_buffers[SURFACE_IMAGE_COUNT];
} tg_renderer_3d_present_pass;

typedef struct tg_renderer_3d
{
    tg_camera_h                     main_camera_h;

    tg_renderer_3d_geometry_pass    geometry_pass;
    tg_renderer_3d_shading_pass     shading_pass;
    tg_renderer_3d_present_pass     present_pass;
} tg_renderer_3d;


// TODO: presenting should only be done in main vulkan file.
void tg_graphics_renderer_3d_internal_init_geometry_pass(tg_renderer_3d_h renderer_3d_h)
{
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, TG_RENDERER_3D_GEOMETRY_PASS_POSITION_FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->geometry_pass.position_attachment.image, &renderer_3d_h->geometry_pass.position_attachment.device_memory);
    tg_graphics_vulkan_image_transition_layout(renderer_3d_h->geometry_pass.position_attachment.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_image_view_create(renderer_3d_h->geometry_pass.position_attachment.image, TG_RENDERER_3D_GEOMETRY_PASS_POSITION_FORMAT, 1, VK_IMAGE_ASPECT_COLOR_BIT, &renderer_3d_h->geometry_pass.position_attachment.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &renderer_3d_h->geometry_pass.position_attachment.sampler);

    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->geometry_pass.normal_attachment.image, &renderer_3d_h->geometry_pass.normal_attachment.device_memory);
    tg_graphics_vulkan_image_transition_layout(renderer_3d_h->geometry_pass.normal_attachment.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_image_view_create(renderer_3d_h->geometry_pass.normal_attachment.image, TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_FORMAT, 1, VK_IMAGE_ASPECT_COLOR_BIT, &renderer_3d_h->geometry_pass.normal_attachment.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &renderer_3d_h->geometry_pass.normal_attachment.sampler);
    
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->geometry_pass.albedo_attachment.image, &renderer_3d_h->geometry_pass.albedo_attachment.device_memory);
    tg_graphics_vulkan_image_transition_layout(renderer_3d_h->geometry_pass.albedo_attachment.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_image_view_create(renderer_3d_h->geometry_pass.albedo_attachment.image, TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_FORMAT, 1, VK_IMAGE_ASPECT_COLOR_BIT, &renderer_3d_h->geometry_pass.albedo_attachment.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &renderer_3d_h->geometry_pass.albedo_attachment.sampler);

    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->geometry_pass.depth_attachment.image, &renderer_3d_h->geometry_pass.depth_attachment.device_memory);
    tg_graphics_vulkan_image_transition_layout(renderer_3d_h->geometry_pass.depth_attachment.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
    tg_graphics_vulkan_image_view_create(renderer_3d_h->geometry_pass.depth_attachment.image, TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_FORMAT, 1, VK_IMAGE_ASPECT_DEPTH_BIT, &renderer_3d_h->geometry_pass.depth_attachment.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &renderer_3d_h->geometry_pass.depth_attachment.sampler);

    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &renderer_3d_h->geometry_pass.rendering_finished_fence);
    tg_graphics_vulkan_semaphore_create(&renderer_3d_h->geometry_pass.rendering_finished_semaphore);

    const u64 min_uniform_buffer_offset_alignment = (u64)tg_graphics_vulkan_physical_device_find_min_uniform_buffer_offset_alignment(physical_device);
    renderer_3d_h->geometry_pass.model_ubo_element_size = tgm_u64_max(sizeof(m4), min_uniform_buffer_offset_alignment);
    tg_graphics_vulkan_buffer_create(TG_RENDERER_3D_MAX_ENTITY_COUNT * (u64)renderer_3d_h->geometry_pass.model_ubo_element_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer_3d_h->geometry_pass.model_ubo.buffer, &renderer_3d_h->geometry_pass.model_ubo.device_memory);
    vkMapMemory(device, renderer_3d_h->geometry_pass.model_ubo.device_memory, 0, TG_RENDERER_3D_MAX_ENTITY_COUNT * renderer_3d_h->geometry_pass.model_ubo_element_size, 0, &renderer_3d_h->geometry_pass.model_ubo_mapped_memory);

    tg_graphics_vulkan_buffer_create(sizeof(tg_camera_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer_3d_h->geometry_pass.view_projection_ubo.buffer, &renderer_3d_h->geometry_pass.view_projection_ubo.device_memory);
    vkMapMemory(device, renderer_3d_h->geometry_pass.view_projection_ubo.device_memory, 0, sizeof(tg_camera_uniform_buffer), 0, &renderer_3d_h->geometry_pass.view_projection_ubo_mapped_memory);

    VkAttachmentDescription p_attachment_descriptions[4] = { 0 };
    {
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].flags = 0;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].format = TG_RENDERER_3D_GEOMETRY_PASS_POSITION_FORMAT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].flags = 0;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].format = TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_FORMAT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].flags = 0;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].format = TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_FORMAT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].flags = 0;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].format = TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_FORMAT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference p_color_attachment_references[TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT] = { 0 };
    {
        p_color_attachment_references[0].attachment = TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT;
        p_color_attachment_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_color_attachment_references[1].attachment = TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT;
        p_color_attachment_references[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_color_attachment_references[2].attachment = TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT;
        p_color_attachment_references[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    };
    VkAttachmentReference depth_buffer_attachment_reference = { 0 };
    {
        depth_buffer_attachment_reference.attachment = TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_ATTACHMENT;
        depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = TG_NULL;
        subpass_description.colorAttachmentCount = TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
        subpass_description.pColorAttachments = p_color_attachment_references;
        subpass_description.pResolveAttachments = TG_NULL;
        subpass_description.pDepthStencilAttachment = &depth_buffer_attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = TG_NULL;
    }
    VkSubpassDependency subpass_dependency = { 0 };
    {
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    tg_graphics_vulkan_render_pass_create(TG_RENDERER_3D_GEOMETRY_PASS_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency, &renderer_3d_h->geometry_pass.render_pass);

    const VkImageView p_framebuffer_attachments[] = {
        renderer_3d_h->geometry_pass.position_attachment.image_view,
        renderer_3d_h->geometry_pass.normal_attachment.image_view,
        renderer_3d_h->geometry_pass.albedo_attachment.image_view,
        renderer_3d_h->geometry_pass.depth_attachment.image_view
    };
    tg_graphics_vulkan_framebuffer_create(renderer_3d_h->geometry_pass.render_pass, TG_RENDERER_3D_GEOMETRY_PASS_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height, &renderer_3d_h->geometry_pass.framebuffer);

    tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &renderer_3d_h->geometry_pass.main_command_buffer);
}
void tg_graphics_renderer_3d_internal_init_shading_pass(tg_renderer_3d_h renderer_3d_h)
{
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_FORMAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->shading_pass.color_attachment.image, &renderer_3d_h->shading_pass.color_attachment.device_memory);
    tg_graphics_vulkan_image_transition_layout(renderer_3d_h->shading_pass.color_attachment.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_image_view_create(renderer_3d_h->shading_pass.color_attachment.image, TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_FORMAT, 1, VK_IMAGE_ASPECT_COLOR_BIT, &renderer_3d_h->shading_pass.color_attachment.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &renderer_3d_h->shading_pass.color_attachment.sampler);
    
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    void* p_data = TG_NULL;

    tg_renderer_3d_screen_vertex p_vertices[4] = { 0 };
    {
        // TODO: y is inverted, because image has to be flipped. add projection matrix to present vertex shader?
        p_vertices[0].position.x = -1.0f;
        p_vertices[0].position.y = -1.0f;
        p_vertices[0].uv.x = 0.0f;
        p_vertices[0].uv.y = 0.0f;

        p_vertices[1].position.x = 1.0f;
        p_vertices[1].position.y = -1.0f;
        p_vertices[1].uv.x = 1.0f;
        p_vertices[1].uv.y = 0.0f;

        p_vertices[2].position.x = 1.0f;
        p_vertices[2].position.y = 1.0f;
        p_vertices[2].uv.x = 1.0f;
        p_vertices[2].uv.y = 1.0f;

        p_vertices[3].position.x = -1.0f;
        p_vertices[3].position.y = 1.0f;
        p_vertices[3].uv.x = 0.0f;
        p_vertices[3].uv.y = 1.0f;
    }
    tg_graphics_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(p_vertices), 0, &p_data));
    {
        memcpy(p_data, p_vertices, sizeof(p_vertices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->shading_pass.vbo.buffer, &renderer_3d_h->shading_pass.vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer, renderer_3d_h->shading_pass.vbo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    u16 p_indices[6] = { 0 };
    {
        p_indices[0] = 0;
        p_indices[1] = 1;
        p_indices[2] = 2;
        p_indices[3] = 2;
        p_indices[4] = 3;
        p_indices[5] = 0;
    }
    tg_graphics_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(p_indices), 0, &p_data));
    {
        memcpy(p_data, p_indices, sizeof(p_indices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->shading_pass.ibo.buffer, &renderer_3d_h->shading_pass.ibo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(p_indices), staging_buffer, renderer_3d_h->shading_pass.ibo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &renderer_3d_h->shading_pass.rendering_finished_fence);
    tg_graphics_vulkan_semaphore_create(&renderer_3d_h->shading_pass.rendering_finished_semaphore);
    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence);

    VkAttachmentDescription attachment_description = { 0 };
    {
        attachment_description.flags = 0;
        attachment_description.format = TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_FORMAT;
        attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference attachment_reference = { 0 };
    {
        attachment_reference.attachment = 0;
        attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = TG_NULL;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &attachment_reference;
        subpass_description.pResolveAttachments = TG_NULL;
        subpass_description.pDepthStencilAttachment = TG_NULL;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = TG_NULL;
    }
    VkSubpassDependency subpass_dependency = { 0 };
    {
        subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dependency.dstSubpass = 0;
        subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dependency.srcAccessMask = 0;
        subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    tg_graphics_vulkan_render_pass_create(TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency, &renderer_3d_h->shading_pass.render_pass);

    tg_graphics_vulkan_framebuffer_create(renderer_3d_h->shading_pass.render_pass, TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &renderer_3d_h->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height, &renderer_3d_h->shading_pass.framebuffer);

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_size.descriptorCount = 1;
    }
    tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &renderer_3d_h->shading_pass.descriptor_pool);

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[3] = { 0 };
    {
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].binding = TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorCount = 1;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].pImmutableSamplers = TG_NULL;

        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].binding = TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorCount = 1;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].pImmutableSamplers = TG_NULL;

        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].binding = TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorCount = 1;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_descriptor_set_layout_bindings[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pImmutableSamplers = TG_NULL;
    }
    tg_graphics_vulkan_descriptor_set_layout_create(0, TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT, p_descriptor_set_layout_bindings, &renderer_3d_h->shading_pass.descriptor_set_layout);
    tg_graphics_vulkan_descriptor_set_allocate(renderer_3d_h->shading_pass.descriptor_pool, renderer_3d_h->shading_pass.descriptor_set_layout, &renderer_3d_h->shading_pass.descriptor_set);

    tg_graphics_vulkan_shader_module_create("shaders/shading.vert.spv", &renderer_3d_h->shading_pass.vertex_shader_h);
    tg_graphics_vulkan_shader_module_create("shaders/shading.frag.spv", &renderer_3d_h->shading_pass.fragment_shader_h);

    tg_graphics_vulkan_pipeline_layout_create(1, &renderer_3d_h->shading_pass.descriptor_set_layout, 0, TG_NULL, &renderer_3d_h->shading_pass.pipeline_layout);

    // TODO: most of the following can propably be abstracted into some screen-rendering thing
    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };
    {
        p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[0].flags = 0;
        p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        p_pipeline_shader_stage_create_infos[0].module = renderer_3d_h->shading_pass.vertex_shader_h;
        p_pipeline_shader_stage_create_infos[0].pName = "main";
        p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

        p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[1].flags = 0;
        p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_pipeline_shader_stage_create_infos[1].module = renderer_3d_h->shading_pass.fragment_shader_h;
        p_pipeline_shader_stage_create_infos[1].pName = "main";
        p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_renderer_3d_screen_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };
    {
        p_vertex_input_attribute_descriptions[0].binding = 0;
        p_vertex_input_attribute_descriptions[0].location = 0;
        p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_renderer_3d_screen_vertex, position);

        p_vertex_input_attribute_descriptions[1].binding = 0;
        p_vertex_input_attribute_descriptions[1].location = 1;
        p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_renderer_3d_screen_vertex, uv);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = TG_NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;
    }
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    {
        pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline_input_assembly_state_create_info.pNext = TG_NULL;
        pipeline_input_assembly_state_create_info.flags = 0;
        pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    }
    VkViewport viewport = { 0 };
    {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)swapchain_extent.width;
        viewport.height = (f32)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    VkRect2D scissors = { 0 };
    {
        scissors.offset = (VkOffset2D){ 0, 0 };
        scissors.extent = swapchain_extent;
    }
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    {
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.pNext = TG_NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissors;
    }
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    {
        pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_rasterization_state_create_info.pNext = TG_NULL;
        pipeline_rasterization_state_create_info.flags = 0;
        pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
        pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
        pipeline_rasterization_state_create_info.lineWidth = 1.0f;
    }
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    {
        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pNext = TG_NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        pipeline_multisample_state_create_info.minSampleShading = 0.0f;
        pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
        pipeline_depth_stencil_state_create_info.flags = 0;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.front.compareMask = 0;
        pipeline_depth_stencil_state_create_info.front.writeMask = 0;
        pipeline_depth_stencil_state_create_info.front.reference = 0;
        pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.back.compareMask = 0;
        pipeline_depth_stencil_state_create_info.back.writeMask = 0;
        pipeline_depth_stencil_state_create_info.back.reference = 0;
        pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
        pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.0f;
    }
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = { 0 };
    {
        pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
        pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    {
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipeline_color_blend_state_create_info.pNext = TG_NULL;
        pipeline_color_blend_state_create_info.flags = 0;
        pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        pipeline_color_blend_state_create_info.attachmentCount = 1;
        pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
        pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;
    }
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    {
        graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.pNext = TG_NULL;
        graphics_pipeline_create_info.flags = 0;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = TG_NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = TG_NULL;
        graphics_pipeline_create_info.layout = renderer_3d_h->shading_pass.pipeline_layout;
        graphics_pipeline_create_info.renderPass = renderer_3d_h->shading_pass.render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    tg_graphics_vulkan_pipeline_create(VK_NULL_HANDLE, &graphics_pipeline_create_info, &renderer_3d_h->shading_pass.pipeline);

    VkDescriptorImageInfo position_buffer_descriptor_image_info = { 0 };
    {
        position_buffer_descriptor_image_info.sampler = renderer_3d_h->geometry_pass.position_attachment.sampler;
        position_buffer_descriptor_image_info.imageView = renderer_3d_h->geometry_pass.position_attachment.image_view;
        position_buffer_descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkDescriptorImageInfo normal_buffer_descriptor_image_info = { 0 };
    {
        normal_buffer_descriptor_image_info.sampler = renderer_3d_h->geometry_pass.normal_attachment.sampler;
        normal_buffer_descriptor_image_info.imageView = renderer_3d_h->geometry_pass.normal_attachment.image_view;
        normal_buffer_descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkDescriptorImageInfo albedo_buffer_descriptor_image_info = { 0 };
    {
        albedo_buffer_descriptor_image_info.sampler = renderer_3d_h->geometry_pass.albedo_attachment.sampler;
        albedo_buffer_descriptor_image_info.imageView = renderer_3d_h->geometry_pass.albedo_attachment.image_view;
        albedo_buffer_descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet p_write_descriptor_sets[3] = { 0 };
    {
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].pNext = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].dstSet = renderer_3d_h->shading_pass.descriptor_set;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].dstBinding = TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].dstArrayElement = 0;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorCount = 1;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].pImageInfo = &position_buffer_descriptor_image_info;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].pBufferInfo = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].pTexelBufferView = TG_NULL;

        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].pNext = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].dstSet = renderer_3d_h->shading_pass.descriptor_set;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].dstBinding = TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].dstArrayElement = 0;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorCount = 1;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].pImageInfo = &normal_buffer_descriptor_image_info;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].pBufferInfo = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].pTexelBufferView = TG_NULL;

        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pNext = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].dstSet = renderer_3d_h->shading_pass.descriptor_set;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].dstBinding = TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].dstArrayElement = 0;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorCount = 1;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pImageInfo = &albedo_buffer_descriptor_image_info;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pBufferInfo = TG_NULL;
        p_write_descriptor_sets[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pTexelBufferView = TG_NULL;
    }
    vkUpdateDescriptorSets(device, TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT, p_write_descriptor_sets, 0, TG_NULL); // TODO: move this call up to the rest of the descriptor calls

    tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &renderer_3d_h->shading_pass.command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = TG_NULL;
    }
    VK_CALL(vkBeginCommandBuffer(renderer_3d_h->shading_pass.command_buffer, &command_buffer_begin_info));
    {
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.position_attachment.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.normal_attachment.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.albedo_attachment.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

        vkCmdBindPipeline(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(renderer_3d_h->shading_pass.command_buffer, 0, 1, &renderer_3d_h->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.pipeline_layout, 0, 1, &renderer_3d_h->shading_pass.descriptor_set, 0, TG_NULL);
        
        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        {
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = TG_NULL;
            render_pass_begin_info.renderPass = renderer_3d_h->shading_pass.render_pass;
            render_pass_begin_info.framebuffer = renderer_3d_h->shading_pass.framebuffer;
            render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
            render_pass_begin_info.renderArea.extent = swapchain_extent;
            render_pass_begin_info.clearValueCount = 0;
            render_pass_begin_info.pClearValues = TG_NULL;
        }
        vkCmdBeginRenderPass(renderer_3d_h->shading_pass.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(renderer_3d_h->shading_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(renderer_3d_h->shading_pass.command_buffer);
        
        const VkClearColorValue clear_color_value = { 0.0f, 0.0f, 0.0f, 0.0f };
        const VkClearDepthStencilValue clear_depth_stencil_value = { 1.0f, 0 };
        VkImageSubresourceRange color_image_subresource_range = { 0 };
        {
            color_image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_image_subresource_range.baseMipLevel = 0;
            color_image_subresource_range.levelCount = 1;
            color_image_subresource_range.baseArrayLayer = 0;
            color_image_subresource_range.layerCount = 1;
        }
        VkImageSubresourceRange depth_image_subresource_range = { 0 };
        {
            depth_image_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depth_image_subresource_range.baseMipLevel = 0;
            depth_image_subresource_range.levelCount = 1;
            depth_image_subresource_range.baseArrayLayer = 0;
            depth_image_subresource_range.layerCount = 1;
        }

        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.position_attachment.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearColorImage(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.position_attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &color_image_subresource_range);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.position_attachment.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.normal_attachment.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearColorImage(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.normal_attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &color_image_subresource_range);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.normal_attachment.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.albedo_attachment.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearColorImage(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.albedo_attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &color_image_subresource_range);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.albedo_attachment.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.depth_attachment.image, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearDepthStencilImage(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.depth_attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &depth_image_subresource_range);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->geometry_pass.depth_attachment.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    }
    VK_CALL(vkEndCommandBuffer(renderer_3d_h->shading_pass.command_buffer));
}
void tg_graphics_renderer_3d_internal_init_present_pass(tg_renderer_3d_h renderer_3d_h)
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    void* p_data = TG_NULL;

    tg_renderer_3d_screen_vertex p_vertices[4] = { 0 };
    {
        // TODO: y is inverted, because image has to be flipped. add projection matrix to present vertex shader?
        p_vertices[0].position.x = -1.0f;
        p_vertices[0].position.y = -1.0f;
        p_vertices[0].uv.x       =  0.0f;
        p_vertices[0].uv.y       =  0.0f;

        p_vertices[1].position.x =  1.0f;
        p_vertices[1].position.y = -1.0f;
        p_vertices[1].uv.x       =  1.0f;
        p_vertices[1].uv.y       =  0.0f;

        p_vertices[2].position.x =  1.0f;
        p_vertices[2].position.y =  1.0f;
        p_vertices[2].uv.x       =  1.0f;
        p_vertices[2].uv.y       =  1.0f;

        p_vertices[3].position.x = -1.0f;
        p_vertices[3].position.y =  1.0f;
        p_vertices[3].uv.x       =  0.0f;
        p_vertices[3].uv.y       =  1.0f;
    }                     
    tg_graphics_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(p_vertices), 0, &p_data));
    {
        memcpy(p_data, p_vertices, sizeof(p_vertices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->present_pass.vbo.buffer, &renderer_3d_h->present_pass.vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer, renderer_3d_h->present_pass.vbo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    u16 p_indices[6] = { 0 };
    {
        p_indices[0] = 0;
        p_indices[1] = 1;
        p_indices[2] = 2;
        p_indices[3] = 2;
        p_indices[4] = 3;
        p_indices[5] = 0;
    }
    tg_graphics_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(p_indices), 0, &p_data));
    {
        memcpy(p_data, p_indices, sizeof(p_indices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_3d_h->present_pass.ibo.buffer, &renderer_3d_h->present_pass.ibo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(p_indices), staging_buffer, renderer_3d_h->present_pass.ibo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    tg_graphics_vulkan_semaphore_create(&renderer_3d_h->present_pass.image_acquired_semaphore);
    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &renderer_3d_h->present_pass.rendering_finished_fence);
    tg_graphics_vulkan_semaphore_create(&renderer_3d_h->present_pass.rendering_finished_semaphore);

    VkAttachmentReference color_attachment_reference = { 0 };
    {
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentDescription attachment_description = { 0 };
    {
        attachment_description.flags = 0;
        attachment_description.format = surface.format.format;
        attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = TG_NULL;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_attachment_reference;
        subpass_description.pResolveAttachments = TG_NULL;
        subpass_description.pDepthStencilAttachment = TG_NULL;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = TG_NULL;
    }
    tg_graphics_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL, &renderer_3d_h->present_pass.render_pass);

    for (u32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        tg_graphics_vulkan_framebuffer_create(renderer_3d_h->present_pass.render_pass, 1, &swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height, &renderer_3d_h->present_pass.framebuffers[i]);
    }

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_size.descriptorCount = 1;
    }
    tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &renderer_3d_h->present_pass.descriptor_pool);

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;
    }
    tg_graphics_vulkan_descriptor_set_layout_create(0, 1, &descriptor_set_layout_binding, &renderer_3d_h->present_pass.descriptor_set_layout);
    tg_graphics_vulkan_descriptor_set_allocate(renderer_3d_h->present_pass.descriptor_pool, renderer_3d_h->present_pass.descriptor_set_layout, &renderer_3d_h->present_pass.descriptor_set);
    
    tg_graphics_vulkan_shader_module_create("shaders/present.vert.spv", &renderer_3d_h->present_pass.vertex_shader_h);
    tg_graphics_vulkan_shader_module_create("shaders/present.frag.spv", &renderer_3d_h->present_pass.fragment_shader_h);

    tg_graphics_vulkan_pipeline_layout_create(1, &renderer_3d_h->present_pass.descriptor_set_layout, 0, TG_NULL, &renderer_3d_h->present_pass.pipeline_layout);

    // TODO: abstract                                                                                         BEGIN
    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };
    {
        p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[0].flags = 0;
        p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        p_pipeline_shader_stage_create_infos[0].module = renderer_3d_h->present_pass.vertex_shader_h;
        p_pipeline_shader_stage_create_infos[0].pName = "main";
        p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

        p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[1].flags = 0;
        p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_pipeline_shader_stage_create_infos[1].module = renderer_3d_h->present_pass.fragment_shader_h;
        p_pipeline_shader_stage_create_infos[1].pName = "main";
        p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_renderer_3d_screen_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };                        // UNIQUE
    {
        p_vertex_input_attribute_descriptions[0].binding = 0;
        p_vertex_input_attribute_descriptions[0].location = 0;
        p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_renderer_3d_screen_vertex, position);

        p_vertex_input_attribute_descriptions[1].binding = 0;
        p_vertex_input_attribute_descriptions[1].location = 1;
        p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_renderer_3d_screen_vertex, uv);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = TG_NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;
    }
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    {
        pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline_input_assembly_state_create_info.pNext = TG_NULL;
        pipeline_input_assembly_state_create_info.flags = 0;
        pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    }
    VkViewport viewport = { 0 };
    {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)swapchain_extent.width;
        viewport.height = (f32)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    VkRect2D scissors = { 0 };
    {
        scissors.offset = (VkOffset2D){ 0, 0 };
        scissors.extent = swapchain_extent;
    }
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    {
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.pNext = TG_NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissors;
    }
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    {
        pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_rasterization_state_create_info.pNext = TG_NULL;
        pipeline_rasterization_state_create_info.flags = 0;
        pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;                                       // UNIQUE
        pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
        pipeline_rasterization_state_create_info.lineWidth = 1.0f;
    }
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    {
        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pNext = TG_NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        pipeline_multisample_state_create_info.minSampleShading = 0.0f;
        pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
        pipeline_depth_stencil_state_create_info.flags = 0;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_FALSE;                                              // UNIQUE
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;                                             // UNIQUE
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.front.compareMask = 0;
        pipeline_depth_stencil_state_create_info.front.writeMask = 0;
        pipeline_depth_stencil_state_create_info.front.reference = 0;
        pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.back.compareMask = 0;
        pipeline_depth_stencil_state_create_info.back.writeMask = 0;
        pipeline_depth_stencil_state_create_info.back.reference = 0;
        pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
        pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.0f;
    }
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = { 0 };
    {
        pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
        pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    {
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipeline_color_blend_state_create_info.pNext = TG_NULL;
        pipeline_color_blend_state_create_info.flags = 0;
        pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        pipeline_color_blend_state_create_info.attachmentCount = 1;
        pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
        pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;
    }
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    {
        graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.pNext = TG_NULL;
        graphics_pipeline_create_info.flags = 0;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = TG_NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = TG_NULL;
        graphics_pipeline_create_info.layout = renderer_3d_h->present_pass.pipeline_layout;                        // UNIQUE
        graphics_pipeline_create_info.renderPass = renderer_3d_h->present_pass.render_pass;                        // UNIQUE
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    tg_graphics_vulkan_pipeline_create(VK_NULL_HANDLE, &graphics_pipeline_create_info, &renderer_3d_h->present_pass.pipeline);

    tg_graphics_vulkan_command_buffers_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, SURFACE_IMAGE_COUNT, renderer_3d_h->present_pass.command_buffers);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = TG_NULL;
    }
    const VkDeviceSize vertex_buffer_offset = 0;
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = renderer_3d_h->shading_pass.color_attachment.sampler;
        descriptor_image_info.imageView = renderer_3d_h->shading_pass.color_attachment.image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = TG_NULL;
        write_descriptor_set.dstSet = renderer_3d_h->present_pass.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set.pImageInfo = &descriptor_image_info;
        write_descriptor_set.pBufferInfo = TG_NULL;
        write_descriptor_set.pTexelBufferView = TG_NULL;
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);

    for (u32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {

        VK_CALL(vkBeginCommandBuffer(renderer_3d_h->present_pass.command_buffers[i], &command_buffer_begin_info));

        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->present_pass.command_buffers[i], renderer_3d_h->shading_pass.color_attachment.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        vkCmdBindPipeline(renderer_3d_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->present_pass.pipeline);
        vkCmdBindVertexBuffers(renderer_3d_h->present_pass.command_buffers[i], 0, 1, &renderer_3d_h->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(renderer_3d_h->present_pass.command_buffers[i], renderer_3d_h->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer_3d_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->present_pass.pipeline_layout, 0, 1, &renderer_3d_h->present_pass.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        {
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = TG_NULL;
            render_pass_begin_info.renderPass = renderer_3d_h->present_pass.render_pass;
            render_pass_begin_info.framebuffer = renderer_3d_h->present_pass.framebuffers[i];
            render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
            render_pass_begin_info.renderArea.extent = swapchain_extent;
            render_pass_begin_info.clearValueCount = 0;
            render_pass_begin_info.pClearValues = TG_NULL;
        }
        vkCmdBeginRenderPass(renderer_3d_h->present_pass.command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(renderer_3d_h->present_pass.command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(renderer_3d_h->present_pass.command_buffers[i]);
        tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(renderer_3d_h->present_pass.command_buffers[i], renderer_3d_h->shading_pass.color_attachment.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        VK_CALL(vkEndCommandBuffer(renderer_3d_h->present_pass.command_buffers[i]));
    }
}

tg_renderer_3d_h tg_graphics_renderer_3d_create(const tg_camera_h camera_h)
{
    TG_ASSERT(camera_h);

    tg_renderer_3d_h renderer_3d_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*renderer_3d_h));

    renderer_3d_h->main_camera_h = camera_h;
    tg_graphics_renderer_3d_internal_init_geometry_pass(renderer_3d_h);
    tg_graphics_renderer_3d_internal_init_shading_pass(renderer_3d_h);
    tg_graphics_renderer_3d_internal_init_present_pass(renderer_3d_h);
    renderer_3d_h->geometry_pass.models = tg_list_create__capacity(tg_model_h, 256);
    renderer_3d_h->geometry_pass.secondary_command_buffers = tg_list_create__capacity(VkCommandBuffer, 256);

    return renderer_3d_h;
}
void tg_graphics_renderer_3d_register(tg_renderer_3d_h renderer_3d_h, tg_entity_h entity_h)
{
    TG_ASSERT(renderer_3d_h && entity_h);

    tg_list_insert(renderer_3d_h->geometry_pass.models, &entity_h->model_h);
    entity_h->p_model_matrix = (m4*)&renderer_3d_h->geometry_pass.model_ubo_mapped_memory[entity_h->id * renderer_3d_h->geometry_pass.model_ubo_element_size];
    *entity_h->p_model_matrix = tgm_m4_identity();

    tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, &entity_h->model_h->render_data.command_buffer);
    tg_list_insert(renderer_3d_h->geometry_pass.secondary_command_buffers, &entity_h->model_h->render_data.command_buffer);

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_pool_size.descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = TG_NULL;
        descriptor_pool_create_info.flags = 0;
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.poolSizeCount = 1;
        descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, TG_NULL, &entity_h->model_h->render_data.descriptor_pool));

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };
    {
        p_descriptor_set_layout_bindings[0].binding = 0;
        p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        p_descriptor_set_layout_bindings[0].descriptorCount = 1;
        p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;
        
        p_descriptor_set_layout_bindings[1].binding = 1;
        p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_descriptor_set_layout_bindings[1].descriptorCount = 1;
        p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = TG_NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = 2;
        descriptor_set_layout_create_info.pBindings = p_descriptor_set_layout_bindings;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, TG_NULL, &entity_h->model_h->render_data.descriptor_set_layout));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = TG_NULL;
        descriptor_set_allocate_info.descriptorPool = entity_h->model_h->render_data.descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &entity_h->model_h->render_data.descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &entity_h->model_h->render_data.descriptor_set));

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = TG_NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = 1; // TODO: ask mesh for additional descriptor sets
        pipeline_layout_create_info.pSetLayouts = &entity_h->model_h->render_data.descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = TG_NULL;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, TG_NULL, &entity_h->model_h->render_data.pipeline_layout));

    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };
    {
        p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[0].flags = 0;
        p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        p_pipeline_shader_stage_create_infos[0].module = entity_h->model_h->material_h->vertex_shader_h->shader_module;
        p_pipeline_shader_stage_create_infos[0].pName = "main";
        p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

        p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[1].flags = 0;
        p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_pipeline_shader_stage_create_infos[1].module = entity_h->model_h->material_h->fragment_shader_h->shader_module;
        p_pipeline_shader_stage_create_infos[1].pName = "main";
        p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_vertex_3d);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[5] = { 0 };
    {
        p_vertex_input_attribute_descriptions[0].binding = 0;
        p_vertex_input_attribute_descriptions[0].location = 0;
        p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_vertex_3d, position);

        p_vertex_input_attribute_descriptions[1].binding = 0;
        p_vertex_input_attribute_descriptions[1].location = 1;
        p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_vertex_3d, normal);

        p_vertex_input_attribute_descriptions[2].binding = 0;
        p_vertex_input_attribute_descriptions[2].location = 2;
        p_vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        p_vertex_input_attribute_descriptions[2].offset = offsetof(tg_vertex_3d, uv);

        p_vertex_input_attribute_descriptions[3].binding = 0;
        p_vertex_input_attribute_descriptions[3].location = 3;
        p_vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        p_vertex_input_attribute_descriptions[3].offset = offsetof(tg_vertex_3d, tangent);

        p_vertex_input_attribute_descriptions[4].binding = 0;
        p_vertex_input_attribute_descriptions[4].location = 4;
        p_vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        p_vertex_input_attribute_descriptions[4].offset = offsetof(tg_vertex_3d, bitangent);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = TG_NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 5;
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;
    }
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    {
        pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline_input_assembly_state_create_info.pNext = TG_NULL;
        pipeline_input_assembly_state_create_info.flags = 0;
        pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    }
    VkViewport viewport = { 0 };
    {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)swapchain_extent.width;
        viewport.height = (f32)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    VkRect2D scissors = { 0 };
    {
        scissors.offset = (VkOffset2D){ 0, 0 };
        scissors.extent = swapchain_extent;
    }
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    {
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.pNext = TG_NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissors;
    }
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    {
        pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_rasterization_state_create_info.pNext = TG_NULL;
        pipeline_rasterization_state_create_info.flags = 0;
        pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
        pipeline_rasterization_state_create_info.lineWidth = 1.0f;
    }
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    {
        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pNext = TG_NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        pipeline_multisample_state_create_info.minSampleShading = 0.0f;
        pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
        pipeline_depth_stencil_state_create_info.flags = 0;
        pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
        pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
        pipeline_depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.front.compareMask = 0;
        pipeline_depth_stencil_state_create_info.front.writeMask = 0;
        pipeline_depth_stencil_state_create_info.front.reference = 0;
        pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
        pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_NEVER;
        pipeline_depth_stencil_state_create_info.back.compareMask = 0;
        pipeline_depth_stencil_state_create_info.back.writeMask = 0;
        pipeline_depth_stencil_state_create_info.back.reference = 0;
        pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
        pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.0f;
    }
    VkPipelineColorBlendAttachmentState p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT] = { 0 };
    {
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].blendEnable = VK_FALSE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].blendEnable = VK_FALSE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].blendEnable = VK_FALSE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    {
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipeline_color_blend_state_create_info.pNext = TG_NULL;
        pipeline_color_blend_state_create_info.flags = 0;
        pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        pipeline_color_blend_state_create_info.attachmentCount = TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
        pipeline_color_blend_state_create_info.pAttachments = p_pipeline_color_blend_attachment_states;
        pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;
    }
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    {
        graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.pNext = TG_NULL;
        graphics_pipeline_create_info.flags = 0;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = TG_NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = TG_NULL;
        graphics_pipeline_create_info.layout = entity_h->model_h->render_data.pipeline_layout;
        graphics_pipeline_create_info.renderPass = renderer_3d_h->geometry_pass.render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, TG_NULL, &entity_h->model_h->render_data.pipeline));

    VkDescriptorBufferInfo model_descriptor_buffer_info = { 0 };
    {
        model_descriptor_buffer_info.buffer = renderer_3d_h->geometry_pass.model_ubo.buffer;
        model_descriptor_buffer_info.offset = 0;
        model_descriptor_buffer_info.range = 65536;
    }
    VkDescriptorBufferInfo view_projection_descriptor_buffer_info = { 0 };
    {
        view_projection_descriptor_buffer_info.buffer = renderer_3d_h->geometry_pass.view_projection_ubo.buffer;
        view_projection_descriptor_buffer_info.offset = 0;
        view_projection_descriptor_buffer_info.range = sizeof(tg_camera_uniform_buffer);
    }
    VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };
    {
        p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[0].pNext = TG_NULL;
        p_write_descriptor_sets[0].dstSet = entity_h->model_h->render_data.descriptor_set;
        p_write_descriptor_sets[0].dstBinding = 0;
        p_write_descriptor_sets[0].dstArrayElement = 0;
        p_write_descriptor_sets[0].descriptorCount = 1;
        p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        p_write_descriptor_sets[0].pImageInfo = TG_NULL;
        p_write_descriptor_sets[0].pBufferInfo = &model_descriptor_buffer_info;
        p_write_descriptor_sets[0].pTexelBufferView = TG_NULL;

        p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[1].pNext = TG_NULL;
        p_write_descriptor_sets[1].dstSet = entity_h->model_h->render_data.descriptor_set;
        p_write_descriptor_sets[1].dstBinding = 1;
        p_write_descriptor_sets[1].dstArrayElement = 0;
        p_write_descriptor_sets[1].descriptorCount = 1;
        p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_write_descriptor_sets[1].pImageInfo = TG_NULL;
        p_write_descriptor_sets[1].pBufferInfo = &view_projection_descriptor_buffer_info;
        p_write_descriptor_sets[1].pTexelBufferView = TG_NULL;
    }
    vkUpdateDescriptorSets(device, 2, p_write_descriptor_sets, 0, TG_NULL);

    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    {
        command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        command_buffer_inheritance_info.pNext = TG_NULL;
        command_buffer_inheritance_info.renderPass = renderer_3d_h->geometry_pass.render_pass;
        command_buffer_inheritance_info.subpass = 0;
        command_buffer_inheritance_info.framebuffer = renderer_3d_h->geometry_pass.framebuffer;
        command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
        command_buffer_inheritance_info.queryFlags = 0;
        command_buffer_inheritance_info.pipelineStatistics = 0;
    }
    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        command_buffer_begin_info.pInheritanceInfo = &command_buffer_inheritance_info;
    }
    VK_CALL(vkBeginCommandBuffer(entity_h->model_h->render_data.command_buffer, &command_buffer_begin_info));

    const u32 model_count = tg_list_count(renderer_3d_h->geometry_pass.models);
    vkCmdBindPipeline(entity_h->model_h->render_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_h->model_h->render_data.pipeline);

    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(entity_h->model_h->render_data.command_buffer, 0, 1, &entity_h->model_h->mesh_h->vbo.buffer, &vertex_buffer_offset);
    if (entity_h->model_h->mesh_h->ibo.index_count != 0) // TODO: do we accept no indices atm? check below aswell
    {
        vkCmdBindIndexBuffer(entity_h->model_h->render_data.command_buffer, entity_h->model_h->mesh_h->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    }

    const u32 dynamic_offset = entity_h->id * (u32)renderer_3d_h->geometry_pass.model_ubo_element_size;
    vkCmdBindDescriptorSets(entity_h->model_h->render_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_h->model_h->render_data.pipeline_layout, 0, 1, &entity_h->model_h->render_data.descriptor_set, 1, &dynamic_offset);

    if (entity_h->model_h->mesh_h->ibo.index_count != 0)
    {
        vkCmdDrawIndexed(entity_h->model_h->render_data.command_buffer, entity_h->model_h->mesh_h->ibo.index_count, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(entity_h->model_h->render_data.command_buffer, entity_h->model_h->mesh_h->vbo.vertex_count, 1, 0, 0);
    }
    VK_CALL(vkEndCommandBuffer(entity_h->model_h->render_data.command_buffer));
}
void tg_graphics_renderer_3d_draw(tg_renderer_3d_h renderer_3d_h)
{
    TG_ASSERT(renderer_3d_h);

    renderer_3d_h->geometry_pass.view_projection_ubo_mapped_memory->view = tg_graphics_camera_get_view(renderer_3d_h->main_camera_h);
    renderer_3d_h->geometry_pass.view_projection_ubo_mapped_memory->projection = tg_graphics_camera_get_projection(renderer_3d_h->main_camera_h);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        command_buffer_begin_info.pInheritanceInfo = TG_NULL;
    }
    VK_CALL(vkWaitForFences(device, 1, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence, VK_TRUE, UINT64_MAX));
    VK_CALL(vkResetFences(device, 1, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence));
    VK_CALL(vkBeginCommandBuffer(renderer_3d_h->geometry_pass.main_command_buffer, &command_buffer_begin_info));

    VkClearValue clear_values[2] = { 0 };
    {
        clear_values[0].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 1.0f };
        clear_values[0].depthStencil = (VkClearDepthStencilValue){ 0.0f, 0 };
        clear_values[1].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 0.0f };
        clear_values[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };
    }
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    {
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        render_pass_begin_info.renderPass = renderer_3d_h->geometry_pass.render_pass;
        render_pass_begin_info.framebuffer = renderer_3d_h->geometry_pass.framebuffer;
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values; // TODO: TG_NULL?
    }
    vkCmdBeginRenderPass(renderer_3d_h->geometry_pass.main_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    const u32 secondary_command_buffer_count = tg_list_count(renderer_3d_h->geometry_pass.secondary_command_buffers);
    vkCmdExecuteCommands(renderer_3d_h->geometry_pass.main_command_buffer, secondary_command_buffer_count, tg_list_pointer_to(renderer_3d_h->geometry_pass.secondary_command_buffers, 0));
    vkCmdEndRenderPass(renderer_3d_h->geometry_pass.main_command_buffer);
    VK_CALL(vkEndCommandBuffer(renderer_3d_h->geometry_pass.main_command_buffer));

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &renderer_3d_h->geometry_pass.main_command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence));





#if 0
    const u32 model_count = tg_list_count(renderer_3d_h->models);
    for (u32 i = 0; i < model_count; i++)
    {
        tg_model_h model_h = *(tg_model_h*)tg_list_pointer_to(renderer_3d_h->models, i);
        tg_uniform_buffer_object* p_uniform_buffer_object = TG_NULL;
        VK_CALL(vkMapMemory(device, model_h->material->ubo.device_memory, 0, sizeof(*p_uniform_buffer_object), 0, &p_uniform_buffer_object));
        {
            p_uniform_buffer_object->view = tg_graphics_camera_get_view(renderer_3d_h->main_camera_h);
            p_uniform_buffer_object->projection = tg_graphics_camera_get_projection(renderer_3d_h->main_camera_h);
        }
        vkUnmapMemory(device, model_h->material->ubo.device_memory);

        VkSubmitInfo submit_info = { 0 };
        {
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = TG_NULL;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = TG_NULL;
            submit_info.pWaitDstStageMask = TG_NULL;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &model_h->command_buffer;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = TG_NULL;
        }
        VK_CALL(vkWaitForFences(device, 1, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence, VK_TRUE, UINT64_MAX));
        VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, VK_NULL_HANDLE));
    }
#endif
}
void tg_graphics_renderer_3d_present(tg_renderer_3d_h renderer_3d_h)
{
    VkSubmitInfo shading_submit_info = { 0 };
    {
        shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        shading_submit_info.pNext = TG_NULL;
        shading_submit_info.waitSemaphoreCount = 0;
        shading_submit_info.pWaitSemaphores = TG_NULL;
        shading_submit_info.pWaitDstStageMask = TG_NULL;
        shading_submit_info.commandBufferCount = 1;
        shading_submit_info.pCommandBuffers = &renderer_3d_h->shading_pass.command_buffer;
        shading_submit_info.signalSemaphoreCount = 1;
        shading_submit_info.pSignalSemaphores = &renderer_3d_h->shading_pass.rendering_finished_semaphore;
    }
    VK_CALL(vkWaitForFences(device, 1, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence, VK_TRUE, UINT64_MAX));
    VK_CALL(vkResetFences(device, 1, &renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &shading_submit_info, renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence));

    u32 current_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, renderer_3d_h->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    const VkSemaphore p_wait_semaphores[2] = { renderer_3d_h->shading_pass.rendering_finished_semaphore, renderer_3d_h->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[2] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo draw_submit_info = { 0 };
    {
        draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        draw_submit_info.pNext = TG_NULL;
        draw_submit_info.waitSemaphoreCount = 2;
        draw_submit_info.pWaitSemaphores = p_wait_semaphores;
        draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
        draw_submit_info.commandBufferCount = 1;
        draw_submit_info.pCommandBuffers = &renderer_3d_h->present_pass.command_buffers[current_image];
        draw_submit_info.signalSemaphoreCount = 1;
        draw_submit_info.pSignalSemaphores = &renderer_3d_h->present_pass.rendering_finished_semaphore;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &draw_submit_info, VK_NULL_HANDLE));

    VkPresentInfoKHR present_info = { 0 };
    {
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = TG_NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &renderer_3d_h->present_pass.rendering_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = TG_NULL;
    }
    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
}
void tg_graphics_renderer_3d_shutdown(tg_renderer_3d_h renderer_3d_h)
{
    TG_ASSERT(renderer_3d_h);

    tg_list_destroy(renderer_3d_h->geometry_pass.secondary_command_buffers);
    tg_list_destroy(renderer_3d_h->geometry_pass.models);
    TG_MEMORY_ALLOCATOR_FREE(renderer_3d_h);
}

#endif
