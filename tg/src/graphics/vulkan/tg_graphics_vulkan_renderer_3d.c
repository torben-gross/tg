#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "util/tg_list.h"
#include "tg_entity_internal.h"



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



typedef struct tg_renderer_3d_screen_vertex
{
    v2    position;
    v2    uv;
} tg_renderer_3d_screen_vertex;

typedef struct tg_renderer_3d_camera_uniform_buffer
{
    m4    view;
    m4    projection;
} tg_renderer_3d_camera_uniform_buffer;

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
    VkPipeline                   pipeline;

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
        VkPipeline                   pipeline;
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
    VkPipeline               pipeline;

    VkCommandBuffer          command_buffers[TG_VULKAN_SURFACE_IMAGE_COUNT];
} tg_renderer_3d_present_pass;

typedef struct tg_renderer_3d
{
    tg_camera_h                     main_camera_h;

    tg_renderer_3d_geometry_pass    geometry_pass;
    tg_renderer_3d_shading_pass     shading_pass;
    tg_renderer_3d_present_pass     present_pass;
} tg_renderer_3d;


// TODO: presenting should only be done in main vulkan file.
void tgg_renderer_3d_internal_init_geometry_pass(tg_renderer_3d_h renderer_3d_h)
{
    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    {
        vulkan_color_image_create_info.width = swapchain_extent.width;
        vulkan_color_image_create_info.height = swapchain_extent.height;
        vulkan_color_image_create_info.mip_levels = 1;
        vulkan_color_image_create_info.format = TG_RENDERER_3D_GEOMETRY_PASS_POSITION_FORMAT;
        vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    renderer_3d_h->geometry_pass.position_attachment = tgg_vulkan_color_image_create(&vulkan_color_image_create_info);
    
    vulkan_color_image_create_info.format = TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_FORMAT;
    renderer_3d_h->geometry_pass.normal_attachment = tgg_vulkan_color_image_create(&vulkan_color_image_create_info);
    
    vulkan_color_image_create_info.format = TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_FORMAT;
    renderer_3d_h->geometry_pass.albedo_attachment = tgg_vulkan_color_image_create(&vulkan_color_image_create_info);
    
    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    {
        vulkan_depth_image_create_info.width = swapchain_extent.width;
        vulkan_depth_image_create_info.height = swapchain_extent.height;
        vulkan_depth_image_create_info.format = TG_RENDERER_3D_GEOMETRY_PASS_DEPTH_FORMAT;
        vulkan_depth_image_create_info.min_filter = VK_FILTER_LINEAR;
        vulkan_depth_image_create_info.mag_filter = VK_FILTER_LINEAR;
        vulkan_depth_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_depth_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_depth_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    renderer_3d_h->geometry_pass.depth_attachment = tgg_vulkan_depth_image_create(&vulkan_depth_image_create_info);

    VkCommandBuffer command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tgg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &renderer_3d_h->geometry_pass.position_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &renderer_3d_h->geometry_pass.normal_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &renderer_3d_h->geometry_pass.albedo_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, &renderer_3d_h->geometry_pass.depth_attachment, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    tgg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

    renderer_3d_h->geometry_pass.rendering_finished_fence = tgg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);
    renderer_3d_h->geometry_pass.rendering_finished_semaphore = tgg_vulkan_semaphore_create();

    renderer_3d_h->geometry_pass.view_projection_ubo = tgg_vulkan_buffer_create(sizeof(tg_renderer_3d_camera_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
    renderer_3d_h->geometry_pass.render_pass = tgg_vulkan_render_pass_create(TG_RENDERER_3D_GEOMETRY_PASS_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);

    const VkImageView p_framebuffer_attachments[] = {
        renderer_3d_h->geometry_pass.position_attachment.image_view,
        renderer_3d_h->geometry_pass.normal_attachment.image_view,
        renderer_3d_h->geometry_pass.albedo_attachment.image_view,
        renderer_3d_h->geometry_pass.depth_attachment.image_view
    };
    renderer_3d_h->geometry_pass.framebuffer = tgg_vulkan_framebuffer_create(renderer_3d_h->geometry_pass.render_pass, TG_RENDERER_3D_GEOMETRY_PASS_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);

    renderer_3d_h->geometry_pass.command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}
void tgg_renderer_3d_internal_init_shading_pass(tg_renderer_3d_h renderer_3d_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
    TG_ASSERT(renderer_3d_h && point_light_count <= TG_RENDERER_3D_SHADING_PASS_MAX_POINT_LIGHTS && (point_light_count == 0 || p_point_lights));

    renderer_3d_h->shading_pass.point_lights_ubo = tgg_vulkan_buffer_create(sizeof(tg_renderer_3d_light_setup_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_renderer_3d_light_setup_uniform_buffer* p_light_setup_uniform_buffer = renderer_3d_h->shading_pass.point_lights_ubo.p_mapped_device_memory;
    p_light_setup_uniform_buffer->point_light_count = point_light_count;
    for (u32 i = 0; i < point_light_count; i++)
    {
        p_light_setup_uniform_buffer->point_light_positions_radii[i].x = p_point_lights[i].position.x;
        p_light_setup_uniform_buffer->point_light_positions_radii[i].y = p_point_lights[i].position.y;
        p_light_setup_uniform_buffer->point_light_positions_radii[i].z = p_point_lights[i].position.z;
        p_light_setup_uniform_buffer->point_light_positions_radii[i].w = p_point_lights[i].radius;
        p_light_setup_uniform_buffer->point_light_colors[i].x = p_point_lights[i].color.x;
        p_light_setup_uniform_buffer->point_light_colors[i].y = p_point_lights[i].color.y;
        p_light_setup_uniform_buffer->point_light_colors[i].z = p_point_lights[i].color.z;
    }

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    {
        vulkan_color_image_create_info.width = swapchain_extent.width;
        vulkan_color_image_create_info.height = swapchain_extent.height;
        vulkan_color_image_create_info.mip_levels = 1;
        vulkan_color_image_create_info.format = TG_RENDERER_3D_SHADING_PASS_COLOR_ATTACHMENT_FORMAT;
        vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
        vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    renderer_3d_h->shading_pass.color_attachment = tgg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tgg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &renderer_3d_h->shading_pass.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

    tg_vulkan_buffer staging_buffer = { 0 };

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
    staging_buffer = tgg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_vertices, sizeof(p_vertices));
    renderer_3d_h->shading_pass.vbo = tgg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, renderer_3d_h->shading_pass.vbo.buffer);
    tgg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };
    {
        p_indices[0] = 0;
        p_indices[1] = 1;
        p_indices[2] = 2;
        p_indices[3] = 2;
        p_indices[4] = 3;
        p_indices[5] = 0;
    }
    staging_buffer = tgg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_indices, sizeof(p_indices));
    renderer_3d_h->shading_pass.ibo = tgg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, renderer_3d_h->shading_pass.ibo.buffer);
    tgg_vulkan_buffer_destroy(&staging_buffer);

    renderer_3d_h->shading_pass.rendering_finished_fence = tgg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);
    renderer_3d_h->shading_pass.rendering_finished_semaphore = tgg_vulkan_semaphore_create();
    renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence = tgg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);

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
    renderer_3d_h->shading_pass.render_pass = tgg_vulkan_render_pass_create(TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);

    renderer_3d_h->shading_pass.framebuffer = tgg_vulkan_framebuffer_create(renderer_3d_h->shading_pass.render_pass, TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &renderer_3d_h->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[4] = { 0 };
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

        p_descriptor_set_layout_bindings[3].binding = 3;
        p_descriptor_set_layout_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_descriptor_set_layout_bindings[3].descriptorCount = 1;
        p_descriptor_set_layout_bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_descriptor_set_layout_bindings[3].pImmutableSamplers = TG_NULL;
    }
    renderer_3d_h->shading_pass.descriptor = tgg_vulkan_descriptor_create(TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT + 1, p_descriptor_set_layout_bindings);

    renderer_3d_h->shading_pass.vertex_shader_h = tgg_vulkan_shader_module_create("shaders/shading.vert");
    renderer_3d_h->shading_pass.fragment_shader_h = tgg_vulkan_shader_module_create("shaders/shading.frag");

    renderer_3d_h->shading_pass.pipeline_layout = tgg_vulkan_pipeline_layout_create(1, &renderer_3d_h->shading_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

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
    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    {
        vulkan_graphics_pipeline_create_info.vertex_shader = renderer_3d_h->shading_pass.vertex_shader_h;
        vulkan_graphics_pipeline_create_info.fragment_shader = renderer_3d_h->shading_pass.fragment_shader_h;
        vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
        vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.attachment_count = 1;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        vulkan_graphics_pipeline_create_info.pipeline_layout = renderer_3d_h->shading_pass.pipeline_layout;
        vulkan_graphics_pipeline_create_info.render_pass = renderer_3d_h->shading_pass.render_pass;
    }
    renderer_3d_h->shading_pass.pipeline = tgg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);














    // exposure
    VkDescriptorSetLayoutBinding p_find_exposure_descriptor_set_layout_bindings[2] = { 0 };
    {
        p_find_exposure_descriptor_set_layout_bindings[0].binding = 0;
        p_find_exposure_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_find_exposure_descriptor_set_layout_bindings[0].descriptorCount = 1;
        p_find_exposure_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        p_find_exposure_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

        p_find_exposure_descriptor_set_layout_bindings[1].binding = 1;
        p_find_exposure_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_find_exposure_descriptor_set_layout_bindings[1].descriptorCount = 1;
        p_find_exposure_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        p_find_exposure_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
    }
    renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader = tgg_vulkan_compute_shader_create("shaders/find_exposure.comp", 2, p_find_exposure_descriptor_set_layout_bindings);
    renderer_3d_h->shading_pass.exposure.exposure_compute_buffer = tgg_vulkan_buffer_create(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    renderer_3d_h->shading_pass.exposure.color_attachment = tgg_vulkan_color_image_create(&vulkan_color_image_create_info);

    command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO combine
    tgg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tgg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &renderer_3d_h->shading_pass.exposure.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

    renderer_3d_h->shading_pass.exposure.render_pass = tgg_vulkan_render_pass_create(TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    renderer_3d_h->shading_pass.exposure.framebuffer = tgg_vulkan_framebuffer_create(renderer_3d_h->shading_pass.exposure.render_pass, TG_RENDERER_3D_SHADING_PASS_ATTACHMENT_COUNT, &renderer_3d_h->shading_pass.exposure.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    VkDescriptorSetLayoutBinding p_adapt_exposure_descriptor_set_layout_bindings[2] = { 0 };
    {
        p_adapt_exposure_descriptor_set_layout_bindings[0].binding = 0;
        p_adapt_exposure_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        p_adapt_exposure_descriptor_set_layout_bindings[0].descriptorCount = 1;
        p_adapt_exposure_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_adapt_exposure_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

        p_adapt_exposure_descriptor_set_layout_bindings[1].binding = 1;
        p_adapt_exposure_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_adapt_exposure_descriptor_set_layout_bindings[1].descriptorCount = 1;
        p_adapt_exposure_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_adapt_exposure_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
    }
    renderer_3d_h->shading_pass.exposure.descriptor = tgg_vulkan_descriptor_create(2, p_adapt_exposure_descriptor_set_layout_bindings);
    renderer_3d_h->shading_pass.exposure.vertex_shader_h = tgg_vulkan_shader_module_create("shaders/adapt_exposure.vert");
    renderer_3d_h->shading_pass.exposure.fragment_shader_h = tgg_vulkan_shader_module_create("shaders/adapt_exposure.frag");
    renderer_3d_h->shading_pass.exposure.pipeline_layout = tgg_vulkan_pipeline_layout_create(1, &renderer_3d_h->shading_pass.exposure.descriptor.descriptor_set_layout, 0, TG_NULL);

    tg_vulkan_graphics_pipeline_create_info exposure_vulkan_graphics_pipeline_create_info = { 0 };
    {
        exposure_vulkan_graphics_pipeline_create_info.vertex_shader = renderer_3d_h->shading_pass.exposure.vertex_shader_h;
        exposure_vulkan_graphics_pipeline_create_info.fragment_shader = renderer_3d_h->shading_pass.exposure.fragment_shader_h;
        exposure_vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
        exposure_vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        exposure_vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
        exposure_vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
        exposure_vulkan_graphics_pipeline_create_info.attachment_count = 1;
        exposure_vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        exposure_vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        exposure_vulkan_graphics_pipeline_create_info.pipeline_layout = renderer_3d_h->shading_pass.exposure.pipeline_layout;
        exposure_vulkan_graphics_pipeline_create_info.render_pass = renderer_3d_h->shading_pass.exposure.render_pass;
    }
    renderer_3d_h->shading_pass.exposure.pipeline = tgg_vulkan_graphics_pipeline_create(&exposure_vulkan_graphics_pipeline_create_info);























    // command buffer stuff
    tgg_vulkan_descriptor_set_update_color_image(renderer_3d_h->shading_pass.descriptor.descriptor_set, &renderer_3d_h->geometry_pass.position_attachment, TG_RENDERER_3D_GEOMETRY_PASS_POSITION_ATTACHMENT);
    tgg_vulkan_descriptor_set_update_color_image(renderer_3d_h->shading_pass.descriptor.descriptor_set, &renderer_3d_h->geometry_pass.normal_attachment, TG_RENDERER_3D_GEOMETRY_PASS_NORMAL_ATTACHMENT);
    tgg_vulkan_descriptor_set_update_color_image(renderer_3d_h->shading_pass.descriptor.descriptor_set, &renderer_3d_h->geometry_pass.albedo_attachment, TG_RENDERER_3D_GEOMETRY_PASS_ALBEDO_ATTACHMENT);
    tgg_vulkan_descriptor_set_update_uniform_buffer(renderer_3d_h->shading_pass.descriptor.descriptor_set, renderer_3d_h->shading_pass.point_lights_ubo.buffer, 3);

    tgg_vulkan_descriptor_set_update_color_image(renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader.descriptor.descriptor_set, &renderer_3d_h->shading_pass.color_attachment, 0);
    tgg_vulkan_descriptor_set_update_storage_buffer(renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader.descriptor.descriptor_set, renderer_3d_h->shading_pass.exposure.exposure_compute_buffer.buffer, 1);
    tgg_vulkan_descriptor_set_update_color_image(renderer_3d_h->shading_pass.exposure.descriptor.descriptor_set, &renderer_3d_h->shading_pass.color_attachment, 0);
    tgg_vulkan_descriptor_set_update_storage_buffer(renderer_3d_h->shading_pass.exposure.descriptor.descriptor_set, renderer_3d_h->shading_pass.exposure.exposure_compute_buffer.buffer, 1);

    renderer_3d_h->shading_pass.command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tgg_vulkan_command_buffer_begin(renderer_3d_h->shading_pass.command_buffer, 0, TG_NULL);
    {
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.position_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.normal_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.albedo_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );

        vkCmdBindPipeline(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(renderer_3d_h->shading_pass.command_buffer, 0, 1, &renderer_3d_h->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.pipeline_layout, 0, 1, &renderer_3d_h->shading_pass.descriptor.descriptor_set, 0, TG_NULL);
        
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

        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.position_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgg_vulkan_command_buffer_cmd_clear_color_image(renderer_3d_h->shading_pass.command_buffer, &renderer_3d_h->geometry_pass.position_attachment);
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.position_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.normal_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgg_vulkan_command_buffer_cmd_clear_color_image(renderer_3d_h->shading_pass.command_buffer, &renderer_3d_h->geometry_pass.normal_attachment);
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.normal_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.albedo_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgg_vulkan_command_buffer_cmd_clear_color_image(renderer_3d_h->shading_pass.command_buffer, &renderer_3d_h->geometry_pass.albedo_attachment);
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.albedo_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        tgg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.depth_attachment,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgg_vulkan_command_buffer_cmd_clear_depth_image(renderer_3d_h->shading_pass.command_buffer, &renderer_3d_h->geometry_pass.depth_attachment);
        tgg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->geometry_pass.depth_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );








        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->shading_pass.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader.pipeline);
        vkCmdBindDescriptorSets(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader.pipeline_layout, 0, 1, &renderer_3d_h->shading_pass.exposure.find_exposure_compute_shader.descriptor.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(renderer_3d_h->shading_pass.command_buffer, 1, 1, 1);








        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.exposure.pipeline);

        vkCmdBindVertexBuffers(renderer_3d_h->shading_pass.command_buffer, 0, 1, &renderer_3d_h->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(renderer_3d_h->shading_pass.command_buffer, renderer_3d_h->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer_3d_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->shading_pass.exposure.pipeline_layout, 0, 1, &renderer_3d_h->shading_pass.exposure.descriptor.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo exposure_render_pass_begin_info = { 0 };
        {
            exposure_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            exposure_render_pass_begin_info.pNext = TG_NULL;
            exposure_render_pass_begin_info.renderPass = renderer_3d_h->shading_pass.exposure.render_pass;
            exposure_render_pass_begin_info.framebuffer = renderer_3d_h->shading_pass.exposure.framebuffer;
            exposure_render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
            exposure_render_pass_begin_info.renderArea.extent = swapchain_extent;
            exposure_render_pass_begin_info.clearValueCount = 0;
            exposure_render_pass_begin_info.pClearValues = TG_NULL;
        }
        vkCmdBeginRenderPass(renderer_3d_h->shading_pass.command_buffer, &exposure_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(renderer_3d_h->shading_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(renderer_3d_h->shading_pass.command_buffer);

        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->shading_pass.command_buffer,
            &renderer_3d_h->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );







    }
    VK_CALL(vkEndCommandBuffer(renderer_3d_h->shading_pass.command_buffer));
}
void tgg_renderer_3d_internal_init_present_pass(tg_renderer_3d_h renderer_3d_h)
{
    tg_vulkan_buffer staging_buffer = { 0 };

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
    staging_buffer = tgg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_vertices, sizeof(p_vertices));
    renderer_3d_h->present_pass.vbo = tgg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, renderer_3d_h->present_pass.vbo.buffer);
    tgg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };
    {
        p_indices[0] = 0;
        p_indices[1] = 1;
        p_indices[2] = 2;
        p_indices[3] = 2;
        p_indices[4] = 3;
        p_indices[5] = 0;
    }
    staging_buffer = tgg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_indices, sizeof(p_indices));
    renderer_3d_h->present_pass.ibo = tgg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tgg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, renderer_3d_h->present_pass.ibo.buffer);
    tgg_vulkan_buffer_destroy(&staging_buffer);

    renderer_3d_h->present_pass.image_acquired_semaphore = tgg_vulkan_semaphore_create();
    renderer_3d_h->present_pass.rendering_finished_fence = tgg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);
    renderer_3d_h->present_pass.rendering_finished_semaphore = tgg_vulkan_semaphore_create();

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
    renderer_3d_h->present_pass.render_pass = tgg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        renderer_3d_h->present_pass.framebuffers[i] = tgg_vulkan_framebuffer_create(renderer_3d_h->present_pass.render_pass, 1, &swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;
    }
    renderer_3d_h->present_pass.descriptor = tgg_vulkan_descriptor_create(1, &descriptor_set_layout_binding);

    renderer_3d_h->present_pass.vertex_shader_h = tgg_vulkan_shader_module_create("shaders/present.vert");
    renderer_3d_h->present_pass.fragment_shader_h = tgg_vulkan_shader_module_create("shaders/present.frag");

    renderer_3d_h->present_pass.pipeline_layout = tgg_vulkan_pipeline_layout_create(1, &renderer_3d_h->present_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

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
    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    {
        vulkan_graphics_pipeline_create_info.vertex_shader = renderer_3d_h->present_pass.vertex_shader_h;
        vulkan_graphics_pipeline_create_info.fragment_shader = renderer_3d_h->present_pass.fragment_shader_h;
        vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
        vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.attachment_count = 1;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        vulkan_graphics_pipeline_create_info.pipeline_layout = renderer_3d_h->present_pass.pipeline_layout;
        vulkan_graphics_pipeline_create_info.render_pass = renderer_3d_h->present_pass.render_pass;
    }
    renderer_3d_h->present_pass.pipeline = tgg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);

    tgg_vulkan_command_buffers_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, renderer_3d_h->present_pass.command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = renderer_3d_h->shading_pass.exposure.color_attachment.sampler;
        descriptor_image_info.imageView = renderer_3d_h->shading_pass.exposure.color_attachment.image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = TG_NULL;
        write_descriptor_set.dstSet = renderer_3d_h->present_pass.descriptor.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set.pImageInfo = &descriptor_image_info;
        write_descriptor_set.pBufferInfo = TG_NULL;
        write_descriptor_set.pTexelBufferView = TG_NULL;
    }
    tgg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        tgg_vulkan_command_buffer_begin(renderer_3d_h->present_pass.command_buffers[i], 0, TG_NULL);

        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->present_pass.command_buffers[i],
            &renderer_3d_h->shading_pass.exposure.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        vkCmdBindPipeline(renderer_3d_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->present_pass.pipeline);
        vkCmdBindVertexBuffers(renderer_3d_h->present_pass.command_buffers[i], 0, 1, &renderer_3d_h->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(renderer_3d_h->present_pass.command_buffers[i], renderer_3d_h->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(renderer_3d_h->present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_3d_h->present_pass.pipeline_layout, 0, 1, &renderer_3d_h->present_pass.descriptor.descriptor_set, 0, TG_NULL);

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
        tgg_vulkan_command_buffer_cmd_transition_color_image_layout(
            renderer_3d_h->present_pass.command_buffers[i],
            &renderer_3d_h->shading_pass.exposure.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        VK_CALL(vkEndCommandBuffer(renderer_3d_h->present_pass.command_buffers[i]));
    }
}

// TODO: move clean into draw+shading+clean
tg_renderer_3d_h tgg_renderer_3d_create(const tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
    TG_ASSERT(camera_h);

    tg_renderer_3d_h renderer_3d_h = TG_MEMORY_ALLOC(sizeof(*renderer_3d_h));

    renderer_3d_h->main_camera_h = camera_h;
    tgg_renderer_3d_internal_init_geometry_pass(renderer_3d_h);
    tgg_renderer_3d_internal_init_shading_pass(renderer_3d_h, point_light_count, p_point_lights);
    tgg_renderer_3d_internal_init_present_pass(renderer_3d_h);

    return renderer_3d_h;
}
void tgg_renderer_3d_register(tg_renderer_3d_h renderer_3d_h, tg_entity_h entity_h)
{
    TG_ASSERT(renderer_3d_h && entity_h);

    const b32 has_custom_descriptor_set = entity_h->model_h->material_h->descriptor.descriptor_pool != VK_NULL_HANDLE;

    entity_h->model_h->render_data.command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };
    {
        p_descriptor_set_layout_bindings[0].binding = 0;
        p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_descriptor_set_layout_bindings[0].descriptorCount = 1;
        p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;
        
        p_descriptor_set_layout_bindings[1].binding = 1;
        p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_descriptor_set_layout_bindings[1].descriptorCount = 1;
        p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
    }
    entity_h->model_h->render_data.descriptor = tgg_vulkan_descriptor_create(2, p_descriptor_set_layout_bindings);

    if (has_custom_descriptor_set)
    {
        const VkDescriptorSetLayout p_descriptor_set_layouts[2] = { entity_h->model_h->render_data.descriptor.descriptor_set_layout, entity_h->model_h->material_h->descriptor.descriptor_set_layout };
        entity_h->model_h->render_data.pipeline_layout = tgg_vulkan_pipeline_layout_create(2, p_descriptor_set_layouts, 0, TG_NULL);
    }
    else
    {
        entity_h->model_h->render_data.pipeline_layout = tgg_vulkan_pipeline_layout_create(1, &entity_h->model_h->render_data.descriptor.descriptor_set_layout, 0, TG_NULL);
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
    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    {
        vulkan_graphics_pipeline_create_info.vertex_shader = entity_h->model_h->material_h->vertex_shader_h->shader_module;
        vulkan_graphics_pipeline_create_info.fragment_shader = entity_h->model_h->material_h->fragment_shader_h->shader_module;
        vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
        vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        vulkan_graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
        vulkan_graphics_pipeline_create_info.depth_write_enable = VK_TRUE;
        vulkan_graphics_pipeline_create_info.attachment_count = TG_RENDERER_3D_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        vulkan_graphics_pipeline_create_info.pipeline_layout = entity_h->model_h->render_data.pipeline_layout;
        vulkan_graphics_pipeline_create_info.render_pass = renderer_3d_h->geometry_pass.render_pass;
    }
    entity_h->model_h->render_data.pipeline = tgg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);

    VkDescriptorBufferInfo model_descriptor_buffer_info = { 0 };
    {
        model_descriptor_buffer_info.buffer = entity_h->transform.uniform_buffer_h->buffer.buffer;
        model_descriptor_buffer_info.offset = 0;
        model_descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkDescriptorBufferInfo view_projection_descriptor_buffer_info = { 0 };
    {
        view_projection_descriptor_buffer_info.buffer = renderer_3d_h->geometry_pass.view_projection_ubo.buffer;
        view_projection_descriptor_buffer_info.offset = 0;
        view_projection_descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };
    {
        p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[0].pNext = TG_NULL;
        p_write_descriptor_sets[0].dstSet = entity_h->model_h->render_data.descriptor.descriptor_set;
        p_write_descriptor_sets[0].dstBinding = 0;
        p_write_descriptor_sets[0].dstArrayElement = 0;
        p_write_descriptor_sets[0].descriptorCount = 1;
        p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_write_descriptor_sets[0].pImageInfo = TG_NULL;
        p_write_descriptor_sets[0].pBufferInfo = &model_descriptor_buffer_info;
        p_write_descriptor_sets[0].pTexelBufferView = TG_NULL;

        p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[1].pNext = TG_NULL;
        p_write_descriptor_sets[1].dstSet = entity_h->model_h->render_data.descriptor.descriptor_set;
        p_write_descriptor_sets[1].dstBinding = 1;
        p_write_descriptor_sets[1].dstArrayElement = 0;
        p_write_descriptor_sets[1].descriptorCount = 1;
        p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_write_descriptor_sets[1].pImageInfo = TG_NULL;
        p_write_descriptor_sets[1].pBufferInfo = &view_projection_descriptor_buffer_info;
        p_write_descriptor_sets[1].pTexelBufferView = TG_NULL;
    }
    tgg_vulkan_descriptor_sets_update(2, p_write_descriptor_sets);

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
    tgg_vulkan_command_buffer_begin(entity_h->model_h->render_data.command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &command_buffer_inheritance_info);
    {
        vkCmdBindPipeline(entity_h->model_h->render_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_h->model_h->render_data.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(entity_h->model_h->render_data.command_buffer, 0, 1, &entity_h->model_h->mesh_h->vbo.buffer, &vertex_buffer_offset);
        if (entity_h->model_h->mesh_h->ibo.size != 0)
        {
            vkCmdBindIndexBuffer(entity_h->model_h->render_data.command_buffer, entity_h->model_h->mesh_h->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        }

        const u32 descriptor_set_count = has_custom_descriptor_set ? 2 : 1;
        VkDescriptorSet p_descriptor_sets[2] = { 0 };
        {
            p_descriptor_sets[0] = entity_h->model_h->render_data.descriptor.descriptor_set;
            if (has_custom_descriptor_set)
            {
                p_descriptor_sets[1] = entity_h->model_h->material_h->descriptor.descriptor_set;
            }
        }
        vkCmdBindDescriptorSets(entity_h->model_h->render_data.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_h->model_h->render_data.pipeline_layout, 0, descriptor_set_count, p_descriptor_sets, 0, TG_NULL);

        if (entity_h->model_h->mesh_h->ibo.size != 0)
        {
            vkCmdDrawIndexed(entity_h->model_h->render_data.command_buffer, (u32)(entity_h->model_h->mesh_h->ibo.size / sizeof(u16)), 1, 0, 0, 0); // TODO: u16
        }
        else
        {
            vkCmdDraw(entity_h->model_h->render_data.command_buffer, (u32)(entity_h->model_h->mesh_h->vbo.size / sizeof(tg_vertex_3d)), 1, 0, 0);
        }
    }
    VK_CALL(vkEndCommandBuffer(entity_h->model_h->render_data.command_buffer));
}
void tgg_renderer_3d_begin(tg_renderer_3d_h renderer_3d_h)
{
    TG_ASSERT(renderer_3d_h);

    ((tg_renderer_3d_camera_uniform_buffer*)renderer_3d_h->geometry_pass.view_projection_ubo.p_mapped_device_memory)->view = tgg_camera_get_view(renderer_3d_h->main_camera_h);
    ((tg_renderer_3d_camera_uniform_buffer*)renderer_3d_h->geometry_pass.view_projection_ubo.p_mapped_device_memory)->projection = tgg_camera_get_projection(renderer_3d_h->main_camera_h);

    tgg_vulkan_fence_wait(renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence);
    tgg_vulkan_fence_reset(renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence);
    tgg_vulkan_command_buffer_begin(renderer_3d_h->geometry_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tgg_vulkan_command_buffer_cmd_begin_render_pass(renderer_3d_h->geometry_pass.command_buffer, renderer_3d_h->geometry_pass.render_pass, renderer_3d_h->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}
void tgg_renderer_3d_draw_entity(tg_renderer_3d_h renderer_3d_h, tg_entity_h entity_h)
{
    TG_ASSERT(renderer_3d_h && entity_h);

    vkCmdExecuteCommands(renderer_3d_h->geometry_pass.command_buffer, 1, &entity_h->model_h->render_data.command_buffer);
}
void tgg_renderer_3d_end(tg_renderer_3d_h renderer_3d_h)
{
    TG_ASSERT(renderer_3d_h);

    vkCmdEndRenderPass(renderer_3d_h->geometry_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(renderer_3d_h->geometry_pass.command_buffer));

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &renderer_3d_h->geometry_pass.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence));
}
void tgg_renderer_3d_present(tg_renderer_3d_h renderer_3d_h)
{
    tgg_vulkan_fence_wait(renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence);// TODO: i feel like the fence setup is odd
    tgg_vulkan_fence_reset(renderer_3d_h->shading_pass.geometry_pass_attachments_cleared_fence);

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
void tgg_renderer_3d_shutdown(tg_renderer_3d_h renderer_3d_h)
{
    TG_ASSERT(renderer_3d_h);

    TG_MEMORY_FREE(renderer_3d_h);
}

#endif
