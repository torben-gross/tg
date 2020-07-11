#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_assets.h"



#define TG_CAMERA_VIEW(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[0])
#define TG_CAMERA_PROJ(view_projection_ubo)             (((m4*)(view_projection_ubo).memory.p_mapped_device_memory)[1])

#define TG_GEOMETRY_DEPTH_ATTACHMENT                    TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT
#define TG_GEOMETRY_ATTACHMENT_COUNT                    (TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1)

#define TG_SHADING_COLOR_ATTACHMENT_COUNT               1
#define TG_SHADING_DEPTH_ATTACHMENT_COUNT               0
#define TG_SHADING_ATTACHMENT_COUNT                     (TG_SHADING_COLOR_ATTACHMENT_COUNT + TG_SHADING_DEPTH_ATTACHMENT_COUNT)
#define TG_SHADING_COLOR_ATTACHMENT_FORMAT              VK_FORMAT_B8G8R8A8_UNORM



typedef struct tg_light_setup
{
    u32    directional_light_count;
    u32    point_light_count;
    u32    padding[2];

    v4     p_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     p_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];

    v4     p_point_light_positions[TG_MAX_POINT_LIGHTS];
    v4     p_point_light_colors[TG_MAX_POINT_LIGHTS];
} tg_light_setup;



static void tg__init_shadow_pass(tg_renderer_h h_renderer)
{
    h_renderer->shadow_pass.enabled = TG_TRUE;

    tg_vulkan_depth_image_create_info depth_image_create_info = { 0 };
    depth_image_create_info.width = TG_CASCADED_SHADOW_MAP_SIZE;
    depth_image_create_info.height = TG_CASCADED_SHADOW_MAP_SIZE;
    depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
    depth_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        h_renderer->shadow_pass.p_shadow_maps[i] = tg_vulkan_depth_image_create(&depth_image_create_info);
    }

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: global! also for other ones in this file!
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, &h_renderer->shadow_pass.p_shadow_maps[i], 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    }
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = VK_FORMAT_D32_SFLOAT;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = TG_NULL;
    subpass_description.colorAttachmentCount = 0;
    subpass_description.pColorAttachments = TG_NULL;
    subpass_description.pResolveAttachments = TG_NULL;
    subpass_description.pDepthStencilAttachment = &attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = TG_NULL;

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    h_renderer->shadow_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        h_renderer->shadow_pass.p_framebuffers[i] = tg_vulkan_framebuffer_create(h_renderer->shadow_pass.render_pass, 1, &h_renderer->shadow_pass.p_shadow_maps[i].image_view, TG_CASCADED_SHADOW_MAP_SIZE, TG_CASCADED_SHADOW_MAP_SIZE);
        h_renderer->shadow_pass.p_lightspace_ubos[i] = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

static void tg__init_geometry_pass(tg_renderer_h h_renderer)
{
    const VkFormat p_color_attachment_formats[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT] = {
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT
    };

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
        vulkan_color_image_create_info.width = swapchain_extent.width;
        vulkan_color_image_create_info.height = swapchain_extent.height;
        vulkan_color_image_create_info.mip_levels = 1;
        vulkan_color_image_create_info.format = p_color_attachment_formats[i];
        vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;
        h_renderer->geometry_pass.p_color_attachments[i] = tg_vulkan_color_image_create(&vulkan_color_image_create_info);
    }

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_renderer->geometry_pass.p_color_attachments[i], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    VkAttachmentDescription p_attachment_descriptions[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_attachment_descriptions[i].flags = 0;
        p_attachment_descriptions[i].format = p_color_attachment_formats[i];
        p_attachment_descriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        p_attachment_descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        p_attachment_descriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        p_attachment_descriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        p_attachment_descriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        p_attachment_descriptions[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        p_attachment_descriptions[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].flags = 0;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].format = VK_FORMAT_D32_SFLOAT;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[TG_GEOMETRY_DEPTH_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference p_color_attachment_references[TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT] = { 0 };

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_color_attachment_references[i].attachment = i;
        p_color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depth_buffer_attachment_reference = { 0 };
    depth_buffer_attachment_reference.attachment = TG_GEOMETRY_DEPTH_ATTACHMENT;
    depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = TG_NULL;
    subpass_description.colorAttachmentCount = TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT;
    subpass_description.pColorAttachments = p_color_attachment_references;
    subpass_description.pResolveAttachments = TG_NULL;
    subpass_description.pDepthStencilAttachment = &depth_buffer_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = TG_NULL;

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    h_renderer->geometry_pass.render_pass = tg_vulkan_render_pass_create(TG_GEOMETRY_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);

    VkImageView p_framebuffer_attachments[TG_GEOMETRY_ATTACHMENT_COUNT] = { 0 };
    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        p_framebuffer_attachments[i] = h_renderer->geometry_pass.p_color_attachments[i].image_view;
    }
    p_framebuffer_attachments[TG_GEOMETRY_DEPTH_ATTACHMENT] = h_renderer->render_target.depth_attachment.image_view;

    h_renderer->geometry_pass.framebuffer = tg_vulkan_framebuffer_create(h_renderer->geometry_pass.render_pass, TG_GEOMETRY_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);
    h_renderer->geometry_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

static void tg__init_shading_pass(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    h_renderer->shading_pass.camera_ubo = tg_vulkan_buffer_create(sizeof(v3), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    *((v3*)h_renderer->shading_pass.camera_ubo.memory.p_mapped_device_memory) = h_renderer->p_camera->position;

    h_renderer->shading_pass.lighting_ubo = tg_vulkan_buffer_create(sizeof(tg_light_setup), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    tg_light_setup* p_light_setup_uniform_buffer = h_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;
    p_light_setup_uniform_buffer->directional_light_count = 0;
    p_light_setup_uniform_buffer->point_light_count = 0;

    h_renderer->shading_pass.shadows_ubo = tg_vulkan_buffer_create((TG_CASCADED_SHADOW_MAPS + 1) * sizeof(f32), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    h_renderer->shading_pass.color_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_renderer->shading_pass.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    tg_vulkan_buffer staging_buffer = { 0 };

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };

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

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_vertices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
    h_renderer->shading_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, h_renderer->shading_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    const u16 p_indices[6] = { 0, 1, 2, 2, 3, 0 };

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    h_renderer->shading_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, h_renderer->shading_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    h_renderer->shading_pass.rendering_finished_semaphore = tg_vulkan_semaphore_create();

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
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

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    h_renderer->shading_pass.render_pass = tg_vulkan_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    h_renderer->shading_pass.framebuffer = tg_vulkan_framebuffer_create(h_renderer->shading_pass.render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_renderer->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/shading.vert"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/shading.frag"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.render_pass = h_renderer->shading_pass.render_pass;
    vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->shading_pass.framebuffer.width;
    vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->shading_pass.framebuffer.height;

    h_renderer->shading_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);

    for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
    {
        tg_vulkan_descriptor_set_update_color_image(h_renderer->shading_pass.graphics_pipeline.descriptor_set, &h_renderer->geometry_pass.p_color_attachments[i], i);
    }

    tg_vulkan_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.graphics_pipeline.descriptor_set, h_renderer->shading_pass.camera_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT);
    tg_vulkan_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.graphics_pipeline.descriptor_set, h_renderer->shading_pass.lighting_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 1);
    tg_vulkan_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.graphics_pipeline.descriptor_set, h_renderer->shading_pass.shadows_ubo.buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 2);
    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        tg_vulkan_descriptor_set_update_uniform_buffer(h_renderer->shading_pass.graphics_pipeline.descriptor_set, h_renderer->shadow_pass.p_lightspace_ubos[i].buffer, TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 3 + i);
        tg_vulkan_descriptor_set_update_depth_image(h_renderer->shading_pass.graphics_pipeline.descriptor_set, &h_renderer->shadow_pass.p_shadow_maps[i], TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT + 3 + TG_CASCADED_SHADOW_MAPS + i);
    }

    h_renderer->shading_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tg_vulkan_command_buffer_begin(h_renderer->shading_pass.command_buffer, 0, TG_NULL);
    {
        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }
        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
                h_renderer->shading_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            );
        }

        vkCmdBindPipeline(h_renderer->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(h_renderer->shading_pass.command_buffer, 0, 1, &h_renderer->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_renderer->shading_pass.command_buffer, h_renderer->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_renderer->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->shading_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_renderer->shading_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->shading_pass.command_buffer, h_renderer->shading_pass.render_pass, &h_renderer->shading_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->shading_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->shading_pass.command_buffer);

        for (u32 i = 0; i < TG_DEFERRED_GEOMETRY_COLOR_ATTACHMENT_COUNT; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
            );
            tg_vulkan_command_buffer_cmd_clear_color_image(h_renderer->shading_pass.command_buffer, &h_renderer->geometry_pass.p_color_attachments[i]);
            tg_vulkan_command_buffer_cmd_transition_color_image_layout(
                h_renderer->shading_pass.command_buffer,
                &h_renderer->geometry_pass.p_color_attachments[i],
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );
        }
        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
                h_renderer->shading_pass.command_buffer,
                &h_renderer->shadow_pass.p_shadow_maps[i],
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
            );
        }
    }
    VK_CALL(vkEndCommandBuffer(h_renderer->shading_pass.command_buffer));
}

static void tg__init_tone_mapping_pass(tg_renderer_h h_renderer)
{
    h_renderer->tone_mapping_pass.acquire_exposure_compute_shader = ((tg_compute_shader_h)tg_assets_get_asset("shaders/acquire_exposure.comp"))->vulkan_shader;
    h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline = tg_vulkan_pipeline_create_compute(&h_renderer->tone_mapping_pass.acquire_exposure_compute_shader);
    h_renderer->tone_mapping_pass.exposure_storage_buffer = tg_vulkan_buffer_create(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = TG_SHADING_COLOR_ATTACHMENT_FORMAT;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_reference = { 0 };
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
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

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    h_renderer->tone_mapping_pass.render_pass = tg_vulkan_render_pass_create(TG_SHADING_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    h_renderer->tone_mapping_pass.framebuffer = tg_vulkan_framebuffer_create(h_renderer->tone_mapping_pass.render_pass, TG_SHADING_ATTACHMENT_COUNT, &h_renderer->render_target.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    tg_vulkan_graphics_pipeline_create_info exposure_vulkan_graphics_pipeline_create_info = { 0 };
    exposure_vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/adapt_exposure.vert"))->vulkan_shader;
    exposure_vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/adapt_exposure.frag"))->vulkan_shader;
    exposure_vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    exposure_vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    exposure_vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.render_pass = h_renderer->tone_mapping_pass.render_pass;
    exposure_vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_renderer->tone_mapping_pass.framebuffer.width;
    exposure_vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_renderer->tone_mapping_pass.framebuffer.height;

    h_renderer->tone_mapping_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&exposure_vulkan_graphics_pipeline_create_info);

    tg_vulkan_descriptor_set_update_color_image(h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, &h_renderer->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, h_renderer->tone_mapping_pass.exposure_storage_buffer.buffer, 1);
    tg_vulkan_descriptor_set_update_color_image(h_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, &h_renderer->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(h_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, h_renderer->tone_mapping_pass.exposure_storage_buffer.buffer, 1);

    h_renderer->tone_mapping_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(h_renderer->tone_mapping_pass.command_buffer, 0, TG_NULL);
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->tone_mapping_pass.command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.acquire_exposure_compute_pipeline.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(h_renderer->tone_mapping_pass.command_buffer, 1, 1, 1);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->tone_mapping_pass.command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(h_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(h_renderer->tone_mapping_pass.command_buffer, 0, 1, &h_renderer->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_renderer->tone_mapping_pass.command_buffer, h_renderer->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_renderer->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->tone_mapping_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_renderer->tone_mapping_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->tone_mapping_pass.command_buffer, h_renderer->tone_mapping_pass.render_pass, &h_renderer->tone_mapping_pass.framebuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->tone_mapping_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->tone_mapping_pass.command_buffer);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->tone_mapping_pass.command_buffer,
            &h_renderer->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    vkEndCommandBuffer(h_renderer->tone_mapping_pass.command_buffer);
}

static void tg__init_forward_pass(tg_renderer_h h_renderer)
{
    VkAttachmentDescription p_attachment_descriptions[2] = { 0 };

    p_attachment_descriptions[0].flags = 0;
    p_attachment_descriptions[0].format = h_renderer->render_target.color_attachment.format;
    p_attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_attachment_descriptions[1].flags = 0;
    p_attachment_descriptions[1].format = VK_FORMAT_D32_SFLOAT;
    p_attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_buffer_attachment_reference = { 0 };
    depth_buffer_attachment_reference.attachment = 1;
    depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = TG_NULL;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = TG_NULL;
    subpass_description.pDepthStencilAttachment = &depth_buffer_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = TG_NULL;

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    h_renderer->forward_pass.render_pass = tg_vulkan_render_pass_create(2, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);
    const VkImageView p_image_views[2] = {
        h_renderer->render_target.color_attachment.image_view,
        h_renderer->render_target.depth_attachment.image_view
    };
    h_renderer->forward_pass.framebuffer = tg_vulkan_framebuffer_create(h_renderer->forward_pass.render_pass, 2, p_image_views, swapchain_extent.width, swapchain_extent.height);
    h_renderer->forward_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

static void tg__init_clear_pass(tg_renderer_h h_renderer)
{
    h_renderer->clear_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(h_renderer->clear_pass.command_buffer, 0, TG_NULL);

    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->clear_pass.command_buffer,
        &h_renderer->render_target.color_attachment,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_color_image(h_renderer->clear_pass.command_buffer, &h_renderer->render_target.color_attachment);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->clear_pass.command_buffer,
        &h_renderer->render_target.color_attachment,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->clear_pass.command_buffer,
        &h_renderer->render_target.depth_attachment,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_clear_depth_image(h_renderer->clear_pass.command_buffer, &h_renderer->render_target.depth_attachment);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->clear_pass.command_buffer,
        &h_renderer->render_target.depth_attachment,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    );

    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_renderer->clear_pass.command_buffer,
            &h_renderer->shadow_pass.p_shadow_maps[i],
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_clear_depth_image(h_renderer->clear_pass.command_buffer, &h_renderer->shadow_pass.p_shadow_maps[i]);
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_renderer->clear_pass.command_buffer,
            &h_renderer->shadow_pass.p_shadow_maps[i],
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );
    }

    vkEndCommandBuffer(h_renderer->clear_pass.command_buffer);
}

static void tg__init_present_pass(tg_renderer_h h_renderer)
{
    tg_vulkan_buffer staging_buffer = { 0 };

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };

    p_vertices[0].position.x = -1.0f;
    p_vertices[0].position.y = 1.0f;
    p_vertices[0].uv.x = 0.0f;
    p_vertices[0].uv.y = 1.0f;

    p_vertices[1].position.x = 1.0f;
    p_vertices[1].position.y = 1.0f;
    p_vertices[1].uv.x = 1.0f;
    p_vertices[1].uv.y = 1.0f;

    p_vertices[2].position.x = 1.0f;
    p_vertices[2].position.y = -1.0f;
    p_vertices[2].uv.x = 1.0f;
    p_vertices[2].uv.y = 0.0f;

    p_vertices[3].position.x = -1.0f;
    p_vertices[3].position.y = -1.0f;
    p_vertices[3].uv.x = 0.0f;
    p_vertices[3].uv.y = 0.0f;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_vertices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
    h_renderer->present_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, h_renderer->present_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };

    p_indices[0] = 0;
    p_indices[1] = 1;
    p_indices[2] = 2;
    p_indices[3] = 2;
    p_indices[4] = 3;
    p_indices[5] = 0;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    h_renderer->present_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, h_renderer->present_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    h_renderer->present_pass.image_acquired_semaphore = tg_vulkan_semaphore_create();
    h_renderer->present_pass.semaphore = tg_vulkan_semaphore_create();

    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = surface.format.format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkSubpassDescription subpass_description = { 0 };
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

    h_renderer->present_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        h_renderer->present_pass.p_framebuffers[i] = tg_vulkan_framebuffer_create(h_renderer->present_pass.render_pass, 1, &p_swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/present.vert"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/present.frag"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.render_pass = h_renderer->present_pass.render_pass;
    vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)swapchain_extent.width;
    vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)swapchain_extent.height;

    h_renderer->present_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);

    tg_vulkan_command_buffers_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, h_renderer->present_pass.p_command_buffers);

    const VkDeviceSize vertex_buffer_offset = 0;

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_renderer->render_target.color_attachment.sampler;
    descriptor_image_info.imageView = h_renderer->render_target.color_attachment.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_renderer->present_pass.graphics_pipeline.descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        tg_vulkan_command_buffer_begin(h_renderer->present_pass.p_command_buffers[i], 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->present_pass.p_command_buffers[i],
            &h_renderer->render_target.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
        vkCmdBindPipeline(h_renderer->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.pipeline);
        vkCmdBindVertexBuffers(h_renderer->present_pass.p_command_buffers[i], 0, 1, &h_renderer->present_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_renderer->present_pass.p_command_buffers[i], h_renderer->present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_renderer->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_renderer->present_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_renderer->present_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);

        tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->present_pass.p_command_buffers[i], h_renderer->present_pass.render_pass, &h_renderer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_renderer->present_pass.p_command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_renderer->present_pass.p_command_buffers[i]);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->present_pass.p_command_buffers[i],
            &h_renderer->render_target.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_renderer->present_pass.p_command_buffers[i],
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        tg_vulkan_command_buffer_cmd_clear_color_image(h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.color_attachment);
        tg_vulkan_command_buffer_cmd_clear_depth_image(h_renderer->present_pass.p_command_buffers[i], &h_renderer->render_target.depth_attachment);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            h_renderer->present_pass.p_command_buffers[i],
            &h_renderer->render_target.color_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
            h_renderer->present_pass.p_command_buffers[i],
            &h_renderer->render_target.depth_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        );

        VK_CALL(vkEndCommandBuffer(h_renderer->present_pass.p_command_buffers[i]));
    }
}



tg_renderer_h tg_renderer_create(tg_camera* p_camera)
{
    TG_ASSERT(p_camera);

    tg_renderer_h h_renderer = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_renderers, h_renderer);

    h_renderer->type = TG_HANDLE_TYPE_RENDERER;
    h_renderer->p_camera = p_camera;
    h_renderer->view_projection_ubo = tg_vulkan_buffer_create(2 * sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = VK_FORMAT_B8G8R8A8_UNORM;
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    vulkan_depth_image_create_info.width = swapchain_extent.width;
    vulkan_depth_image_create_info.height = swapchain_extent.height;
    vulkan_depth_image_create_info.format = VK_FORMAT_D32_SFLOAT;
    vulkan_depth_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    h_renderer->render_target = tg_vulkan_render_target_create(&vulkan_color_image_create_info, &vulkan_depth_image_create_info, VK_FENCE_CREATE_SIGNALED_BIT);
    h_renderer->render_command_count = 0;

    tg__init_shadow_pass(h_renderer);
    tg__init_geometry_pass(h_renderer);
    tg__init_shading_pass(h_renderer);
    tg__init_tone_mapping_pass(h_renderer);
    tg__init_forward_pass(h_renderer);
    tg__init_clear_pass(h_renderer);
    tg__init_present_pass(h_renderer);

    return h_renderer;
}

void tg_renderer_destroy(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    TG_INVALID_CODEPATH();
}

void tg_renderer_enable_shadows(tg_renderer_h h_renderer, b32 enable)
{
    TG_ASSERT(h_renderer);

    h_renderer->shadow_pass.enabled = enable;
}

void tg_renderer_begin(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    h_renderer->render_command_count = 0;
}

void tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(direction) && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_light_setup* p_light_setup = (tg_light_setup*)h_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;

    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].x = direction.x;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].y = direction.y;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].z = direction.z;
    p_light_setup->p_directional_light_directions[p_light_setup->directional_light_count].w = 0.0f;

    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].x = color.x;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].y = color.y;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].z = color.z;
    p_light_setup->p_directional_light_colors[p_light_setup->directional_light_count].w = 1.0f;

    p_light_setup->directional_light_count++;
}

void tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color)
{
    TG_ASSERT(h_renderer && tgm_v3_mag(color));

    // TODO: forward renderer
    tg_light_setup* p_light_setup = (tg_light_setup*)h_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;

    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].x = position.x;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].y = position.y;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].z = position.z;
    p_light_setup->p_point_light_positions[p_light_setup->point_light_count].w = 0.0f;

    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].x = color.x;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].y = color.y;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].z = color.z;
    p_light_setup->p_point_light_colors[p_light_setup->point_light_count].w = 1.0f;

    p_light_setup->point_light_count++;
}

void tg_renderer_execute(tg_renderer_h h_renderer, tg_render_command_h h_render_command)
{
    TG_ASSERT(h_renderer && h_render_command);

    tg_vulkan_render_command_renderer_info* p_renderer_info = TG_NULL;
    for (u32 i = 0; i < h_render_command->renderer_info_count; i++)
    {
        if (h_render_command->p_renderer_infos[i].h_renderer == h_renderer)
        {
            p_renderer_info = &h_render_command->p_renderer_infos[i];
            break;
        }
    }

    // TODO: this crashes, when a render command is created before the camera. that way,
    // the camera info is not set up for that specific camera. think of threading in
    // this case!
    TG_ASSERT(p_renderer_info);
    TG_ASSERT(h_renderer->render_command_count < TG_MAX_RENDER_COMMANDS);

    h_renderer->ph_render_commands[h_renderer->render_command_count++] = h_render_command;
}

void tg_renderer_end(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    TG_CAMERA_VIEW(h_renderer->view_projection_ubo) = tgm_m4_mul(
        tgm_m4_inverse(tgm_m4_euler(h_renderer->p_camera->pitch, h_renderer->p_camera->yaw, h_renderer->p_camera->roll)),
        tgm_m4_translate(tgm_v3_neg(h_renderer->p_camera->position))
    );
    switch (h_renderer->p_camera->type)
    {
    case TG_CAMERA_TYPE_ORTHOGRAPHIC:
    {
        TG_CAMERA_PROJ(h_renderer->view_projection_ubo) = tgm_m4_orthographic(
            h_renderer->p_camera->orthographic.left,
            h_renderer->p_camera->orthographic.right,
            h_renderer->p_camera->orthographic.bottom,
            h_renderer->p_camera->orthographic.top,
            h_renderer->p_camera->orthographic.far,
            h_renderer->p_camera->orthographic.near
        );
    } break;
    case TG_CAMERA_TYPE_PERSPECTIVE:
    {
        TG_CAMERA_PROJ(h_renderer->view_projection_ubo) = tgm_m4_perspective(
            h_renderer->p_camera->perspective.fov_y_in_radians,
            h_renderer->p_camera->perspective.aspect,
            h_renderer->p_camera->perspective.near,
            h_renderer->p_camera->perspective.far
        );
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    if (h_renderer->shadow_pass.enabled)
    {
        tg_vulkan_command_buffer_begin(h_renderer->shadow_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);

        const f32 p_percentiles[TG_CASCADED_SHADOW_MAPS + 1] = { 0.0f, 0.003f, 0.01f, 0.04f };

        for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
        {
            tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->shadow_pass.command_buffer, h_renderer->shadow_pass.render_pass, &h_renderer->shadow_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            // TODO: decide, which lights cast/receive shadows!
            // TODO: penumbra

            const v4 p_corners[8] = {
                { -1.0f,  1.0f,  0.0f,  1.0f },
                {  1.0f,  1.0f,  0.0f,  1.0f },
                { -1.0f, -1.0f,  0.0f,  1.0f },
                {  1.0f, -1.0f,  0.0f,  1.0f },
                { -1.0f,  1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f,  1.0f },
                { -1.0f, -1.0f,  1.0f,  1.0f },
                {  1.0f, -1.0f,  1.0f,  1.0f }
            };

            const m4 ivp = tgm_m4_inverse(tgm_m4_mul(TG_CAMERA_PROJ(h_renderer->view_projection_ubo), TG_CAMERA_VIEW(h_renderer->view_projection_ubo)));
            v3 p_corners_ws[8] = { 0 };
            for (u8 i = 0; i < 8; i++)
            {
                p_corners_ws[i] = tgm_v4_to_v3(tgm_m4_mulv4(ivp, p_corners[i]));
            }

            const v3 v04 = tgm_v3_sub(p_corners_ws[4], p_corners_ws[0]);
            const f32 d = tgm_v3_mag(v04);
            const f32 d0 = d * p_percentiles[i];
            const f32 d1 = d * p_percentiles[i + 1];
            ((f32*)h_renderer->shading_pass.shadows_ubo.memory.p_mapped_device_memory)[i] = d0;
            ((f32*)h_renderer->shading_pass.shadows_ubo.memory.p_mapped_device_memory)[i + 1] = d1;

            for (u8 i = 0; i < 4; i++)
            {
                const v3 ndir = tgm_v3_normalized(tgm_v3_sub(p_corners_ws[i + 4], p_corners_ws[i]));
                p_corners_ws[i + 4] = tgm_v3_add(p_corners_ws[i], tgm_v3_mulf(ndir, d1));
                p_corners_ws[i] = tgm_v3_add(p_corners_ws[i], tgm_v3_mulf(ndir, d0));
            }

            v3 center; f32 radius;
            tgm_enclosing_sphere(8, p_corners_ws, &center, &radius);

            const m4 t = tgm_m4_translate(tgm_v3_neg(center));
            const m4 r = tgm_m4_inverse(tgm_m4_euler(-(f32)TGM_PI * 0.4f, -(f32)TGM_PI * 0.4f, 0.0f));
            const m4 v = tgm_m4_mul(r, t);
            const m4 p = tgm_m4_orthographic(-radius, radius, -radius, radius, -radius, radius);
            const m4 vp = tgm_m4_mul(p, v);

            *((m4*)h_renderer->shadow_pass.p_lightspace_ubos[i].memory.p_mapped_device_memory) = vp;

            for (u32 j = 0; j < h_renderer->render_command_count; j++)
            {
                tg_render_command_h h_render_command = h_renderer->ph_render_commands[j];
                if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
                {
                    for (u32 k = 0; k < h_render_command->renderer_info_count; k++)
                    {
                        tg_vulkan_render_command_renderer_info* p_renderer_info = &h_render_command->p_renderer_infos[k];
                        if (p_renderer_info->h_renderer == h_renderer)
                        {
                            vkCmdExecuteCommands(h_renderer->shadow_pass.command_buffer, 1, &p_renderer_info->p_shadow_command_buffers[i]);
                            break;
                        }
                    }
                }
            }

            vkCmdEndRenderPass(h_renderer->shadow_pass.command_buffer);
        }

        tg_vulkan_buffer_flush_mapped_memory(&h_renderer->shading_pass.shadows_ubo);
        VK_CALL(vkEndCommandBuffer(h_renderer->shadow_pass.command_buffer));

        VkSubmitInfo submit_info = { 0 };
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &h_renderer->shadow_pass.command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;

        tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);
    }

    tg_vulkan_command_buffer_begin(h_renderer->geometry_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->geometry_pass.command_buffer, h_renderer->geometry_pass.render_pass, &h_renderer->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    for (u32 i = 0; i < h_renderer->render_command_count; i++)
    {
        tg_render_command_h h_render_command = h_renderer->ph_render_commands[i];
        if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED)
        {
            for (u32 j = 0; j < h_render_command->renderer_info_count; j++)
            {
                tg_vulkan_render_command_renderer_info* p_renderer_info = &h_render_command->p_renderer_infos[j];
                if (p_renderer_info->h_renderer == h_renderer)
                {
                    vkCmdExecuteCommands(h_renderer->geometry_pass.command_buffer, 1, &p_renderer_info->command_buffer);
                    // TODO: lod
                    //{
                    //    const tg_bounds* p_bounds = &h_render_command->ph_lod_meshes[0]->bounds;
                    //
                    //    v4 p_points[8] = { 0 };
                    //    p_points[0] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                    //    p_points[1] = (v4){ p_bounds->min.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                    //    p_points[2] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                    //    p_points[3] = (v4){ p_bounds->min.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                    //    p_points[4] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->min.z, 1.0f };
                    //    p_points[5] = (v4){ p_bounds->max.x, p_bounds->min.y, p_bounds->max.z, 1.0f };
                    //    p_points[6] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->min.z, 1.0f };
                    //    p_points[7] = (v4){ p_bounds->max.x, p_bounds->max.y, p_bounds->max.z, 1.0f };
                    //
                    //    v4 p_homogenous_points_clip_space[8] = { 0 };
                    //    v3 p_cartesian_points_clip_space[8] = { 0 };
                    //    for (u8 i = 0; i < 8; i++)
                    //    {
                    //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(h_render_command), p_points[i]);
                    //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_VIEW(h_primary_camera), p_homogenous_points_clip_space[i]);
                    //        p_homogenous_points_clip_space[i] = tgm_m4_mulv4(TG_CAMERA_PROJ(h_primary_camera), p_homogenous_points_clip_space[i]);// TODO: SIMD
                    //        p_cartesian_points_clip_space[i] = tgm_v4_to_v3(p_homogenous_points_clip_space[i]);
                    //    }
                    //
                    //    v3 min = p_cartesian_points_clip_space[0];
                    //    v3 max = p_cartesian_points_clip_space[0];
                    //    for (u8 i = 1; i < 8; i++)
                    //    {
                    //        min = tgm_v3_min(min, p_cartesian_points_clip_space[i]);
                    //        max = tgm_v3_max(max, p_cartesian_points_clip_space[i]);
                    //    }
                    //
                    //    const b32 cull = min.x >= 1.0f || min.y >= 1.0f || max.x <= -1.0f || max.y <= -1.0f || min.z >= 1.0f || max.z <= 0.0f;
                    //    if (!cull)
                    //    {
                    //        const v3 clamp_min = { -1.0f, -1.0f, 0.0f };
                    //        const v3 clamp_max = { 1.0f,  1.0f, 0.0f };
                    //        const v3 clamped_min = tgm_v3_clamp(min, clamp_min, clamp_max);
                    //        const v3 clamped_max = tgm_v3_clamp(max, clamp_min, clamp_max);
                    //        const f32 area = (clamped_max.x - clamped_min.x) * (clamped_max.y - clamped_min.y);
                    //
                    //        if (area >= 0.9f)
                    //        {
                    //            tg_vulkan_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[2]);
                    //        }
                    //        else
                    //        {
                    //            tg_vulkan_deferred_renderer_execute(h_primary_camera->capture_pass.h_deferred_renderer, p_camera_info->p_command_buffers[3]);
                    //        }
                    //    }
                    //}
                    break;
                }
            }
        }
    }

    vkCmdEndRenderPass(h_renderer->geometry_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(h_renderer->geometry_pass.command_buffer));

    tg_vulkan_fence_wait(h_renderer->render_target.fence);
    tg_vulkan_fence_reset(h_renderer->render_target.fence);

    tg_vulkan_buffer_flush_mapped_memory(&h_renderer->shading_pass.lighting_ubo);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_renderer->geometry_pass.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &h_renderer->shading_pass.rendering_finished_semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, VK_NULL_HANDLE);

    *((v3*)h_renderer->shading_pass.camera_ubo.memory.p_mapped_device_memory) = h_renderer->p_camera->position;

    const VkPipelineStageFlags p_pipeline_stage_flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &h_renderer->shading_pass.rendering_finished_semaphore;
    shading_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &h_renderer->shading_pass.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &h_renderer->shading_pass.rendering_finished_semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &shading_submit_info, VK_NULL_HANDLE);

    VkSubmitInfo tone_mapping_submit_info = { 0 };
    tone_mapping_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    tone_mapping_submit_info.pNext = TG_NULL;
    tone_mapping_submit_info.waitSemaphoreCount = 1;
    tone_mapping_submit_info.pWaitSemaphores = &h_renderer->shading_pass.rendering_finished_semaphore;
    tone_mapping_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    tone_mapping_submit_info.commandBufferCount = 1;
    tone_mapping_submit_info.pCommandBuffers = &h_renderer->tone_mapping_pass.command_buffer;
    tone_mapping_submit_info.signalSemaphoreCount = 0;
    tone_mapping_submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &tone_mapping_submit_info, h_renderer->render_target.fence);

    tg_vulkan_command_buffer_begin(h_renderer->forward_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(h_renderer->forward_pass.command_buffer, h_renderer->forward_pass.render_pass, &h_renderer->forward_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // TODO: this loop can be abstracted
    for (u32 i = 0; i < h_renderer->render_command_count; i++)
    {
        tg_render_command_h h_render_command = h_renderer->ph_render_commands[i];
        if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD)
        {
            for (u32 j = 0; j < h_render_command->renderer_info_count; j++)
            {
                if (h_render_command->p_renderer_infos[j].h_renderer == h_renderer)
                {
                    vkCmdExecuteCommands(h_renderer->forward_pass.command_buffer, 1, &h_render_command->p_renderer_infos[j].command_buffer);
                    break;
                }
            }
        }
    }

    vkCmdEndRenderPass(h_renderer->forward_pass.command_buffer);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.color_attachment,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.color_attachment_copy,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.depth_attachment,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.depth_attachment_copy,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkImageBlit color_image_blit = { 0 };
    color_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_blit.srcSubresource.mipLevel = 0;
    color_image_blit.srcSubresource.baseArrayLayer = 0;
    color_image_blit.srcSubresource.layerCount = 1;
    color_image_blit.srcOffsets[0].x = 0;
    color_image_blit.srcOffsets[0].y = h_renderer->render_target.color_attachment.height;
    color_image_blit.srcOffsets[0].z = 0;
    color_image_blit.srcOffsets[1].x = h_renderer->render_target.color_attachment.width;
    color_image_blit.srcOffsets[1].y = 0;
    color_image_blit.srcOffsets[1].z = 1;
    color_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_blit.dstSubresource.mipLevel = 0;
    color_image_blit.dstSubresource.baseArrayLayer = 0;
    color_image_blit.dstSubresource.layerCount = 1;
    color_image_blit.dstOffsets[0].x = 0;
    color_image_blit.dstOffsets[0].y = 0;
    color_image_blit.dstOffsets[0].z = 0;
    color_image_blit.dstOffsets[1].x = h_renderer->render_target.color_attachment.width;
    color_image_blit.dstOffsets[1].y = h_renderer->render_target.color_attachment.height;
    color_image_blit.dstOffsets[1].z = 1;

    tg_vulkan_command_buffer_cmd_blit_color_image(h_renderer->forward_pass.command_buffer, &h_renderer->render_target.color_attachment, &h_renderer->render_target.color_attachment_copy, &color_image_blit);

    VkImageBlit depth_image_blit = { 0 };
    depth_image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_blit.srcSubresource.mipLevel = 0;
    depth_image_blit.srcSubresource.baseArrayLayer = 0;
    depth_image_blit.srcSubresource.layerCount = 1;
    depth_image_blit.srcOffsets[0].x = 0;
    depth_image_blit.srcOffsets[0].y = h_renderer->render_target.depth_attachment.height;
    depth_image_blit.srcOffsets[0].z = 0;
    depth_image_blit.srcOffsets[1].x = h_renderer->render_target.depth_attachment.width;
    depth_image_blit.srcOffsets[1].y = 0;
    depth_image_blit.srcOffsets[1].z = 1;
    depth_image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_blit.dstSubresource.mipLevel = 0;
    depth_image_blit.dstSubresource.baseArrayLayer = 0;
    depth_image_blit.dstSubresource.layerCount = 1;
    depth_image_blit.dstOffsets[0].x = 0;
    depth_image_blit.dstOffsets[0].y = 0;
    depth_image_blit.dstOffsets[0].z = 0;
    depth_image_blit.dstOffsets[1].x = h_renderer->render_target.depth_attachment.width;
    depth_image_blit.dstOffsets[1].y = h_renderer->render_target.depth_attachment.height;
    depth_image_blit.dstOffsets[1].z = 1;

    tg_vulkan_command_buffer_cmd_blit_depth_image(h_renderer->forward_pass.command_buffer, &h_renderer->render_target.depth_attachment, &h_renderer->render_target.depth_attachment_copy, &depth_image_blit);

    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.color_attachment,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.color_attachment_copy,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.depth_attachment,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    );
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(
        h_renderer->forward_pass.command_buffer,
        &h_renderer->render_target.depth_attachment_copy,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    VK_CALL(vkEndCommandBuffer(h_renderer->forward_pass.command_buffer));

    tg_vulkan_fence_wait(h_renderer->render_target.fence);
    tg_vulkan_fence_reset(h_renderer->render_target.fence);

    VkSubmitInfo forward_submit_info = { 0 };
    forward_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    forward_submit_info.pNext = TG_NULL;
    forward_submit_info.waitSemaphoreCount = 0;
    forward_submit_info.pWaitSemaphores = TG_NULL;
    forward_submit_info.pWaitDstStageMask = TG_NULL;
    forward_submit_info.commandBufferCount = 1;
    forward_submit_info.pCommandBuffers = &h_renderer->forward_pass.command_buffer;
    forward_submit_info.signalSemaphoreCount = 0;
    forward_submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &forward_submit_info, h_renderer->render_target.fence);
}

void tg_renderer_present(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    u32 current_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, h_renderer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    tg_vulkan_fence_wait(h_renderer->render_target.fence);
    tg_vulkan_fence_reset(h_renderer->render_target.fence);

    const VkSemaphore p_wait_semaphores[1] = { h_renderer->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[1] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

    VkSubmitInfo draw_submit_info = { 0 };
    draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    draw_submit_info.pNext = TG_NULL;
    draw_submit_info.waitSemaphoreCount = 1;
    draw_submit_info.pWaitSemaphores = p_wait_semaphores;
    draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
    draw_submit_info.commandBufferCount = 1;
    draw_submit_info.pCommandBuffers = &h_renderer->present_pass.p_command_buffers[current_image];
    draw_submit_info.signalSemaphoreCount = 1;
    draw_submit_info.pSignalSemaphores = &h_renderer->present_pass.semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &draw_submit_info, h_renderer->render_target.fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &h_renderer->present_pass.semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    tg_vulkan_queue_present(&present_info);
}

void tg_renderer_clear(tg_renderer_h h_renderer)
{
    TG_ASSERT(h_renderer);

    tg_vulkan_fence_wait(h_renderer->render_target.fence);
    tg_vulkan_fence_reset(h_renderer->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_renderer->clear_pass.command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_renderer->render_target.fence);

    // TODO: forward renderer
    tg_light_setup* p_light_setup = (tg_light_setup*)h_renderer->shading_pass.lighting_ubo.memory.p_mapped_device_memory;

    p_light_setup->directional_light_count = 0;
    p_light_setup->point_light_count = 0;
}

tg_render_target_h tg_renderer_get_render_target(tg_renderer_h h_renderer)
{
    return &h_renderer->render_target;
}

#endif
