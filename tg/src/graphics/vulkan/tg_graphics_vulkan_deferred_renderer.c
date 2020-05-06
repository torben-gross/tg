#include "graphics/vulkan/tg_graphics_vulkan_deferred_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"



void tg_deferred_renderer_internal_destroy_geometry_pass(tg_deferred_renderer_h deferred_renderer_h)
{
    tg_vulkan_color_image_destroy(&deferred_renderer_h->geometry_pass.position_attachment);
    tg_vulkan_color_image_destroy(&deferred_renderer_h->geometry_pass.normal_attachment);
    tg_vulkan_color_image_destroy(&deferred_renderer_h->geometry_pass.albedo_attachment);
    tg_vulkan_render_pass_destroy(deferred_renderer_h->geometry_pass.render_pass);
    tg_vulkan_framebuffer_destroy(deferred_renderer_h->geometry_pass.framebuffer);
    tg_vulkan_command_buffer_free(graphics_command_pool, deferred_renderer_h->geometry_pass.command_buffer);
}

void tg_deferred_renderer_internal_destroy_shading_pass(tg_deferred_renderer_h deferred_renderer_h)
{
    tg_vulkan_color_image_destroy(&deferred_renderer_h->shading_pass.color_attachment);
    tg_vulkan_buffer_destroy(&deferred_renderer_h->shading_pass.vbo);
    tg_vulkan_buffer_destroy(&deferred_renderer_h->shading_pass.ibo);
    tg_vulkan_semaphore_destroy(deferred_renderer_h->shading_pass.rendering_finished_semaphore);
    tg_vulkan_render_pass_destroy(deferred_renderer_h->shading_pass.render_pass);
    tg_vulkan_framebuffer_destroy(deferred_renderer_h->shading_pass.framebuffer);
    tg_vulkan_descriptor_destroy(&deferred_renderer_h->shading_pass.descriptor);
    tg_vulkan_shader_module_destroy(deferred_renderer_h->shading_pass.vertex_shader);
    tg_vulkan_shader_module_destroy(deferred_renderer_h->shading_pass.fragment_shader);
    tg_vulkan_pipeline_layout_destroy(deferred_renderer_h->shading_pass.pipeline_layout);
    tg_vulkan_graphics_pipeline_destroy(deferred_renderer_h->shading_pass.graphics_pipeline);
    tg_vulkan_command_buffer_free(graphics_command_pool, deferred_renderer_h->shading_pass.command_buffer);
    tg_vulkan_buffer_destroy(&deferred_renderer_h->shading_pass.point_lights_ubo);
}

void tg_deferred_renderer_internal_destroy_tone_mapping_pass(tg_deferred_renderer_h deferred_renderer_h)
{
    tg_vulkan_compute_shader_destroy(&deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader);
    tg_vulkan_buffer_destroy(&deferred_renderer_h->tone_mapping_pass.exposure_storage_buffer);
    tg_vulkan_render_pass_destroy(deferred_renderer_h->tone_mapping_pass.render_pass);
    tg_vulkan_framebuffer_destroy(deferred_renderer_h->tone_mapping_pass.framebuffer);
    tg_vulkan_descriptor_destroy(&deferred_renderer_h->tone_mapping_pass.descriptor);
    tg_vulkan_shader_module_destroy(deferred_renderer_h->tone_mapping_pass.vertex_shader);
    tg_vulkan_shader_module_destroy(deferred_renderer_h->tone_mapping_pass.fragment_shader);
    tg_vulkan_pipeline_layout_destroy(deferred_renderer_h->tone_mapping_pass.pipeline_layout);
    tg_vulkan_graphics_pipeline_destroy(deferred_renderer_h->tone_mapping_pass.graphics_pipeline);
    tg_vulkan_command_buffer_free(graphics_command_pool, deferred_renderer_h->tone_mapping_pass.command_buffer);
}

void tg_deferred_renderer_internal_init_geometry_pass(tg_deferred_renderer_h deferred_renderer_h)
{
    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_FORMAT;
    vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
    vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
    vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    deferred_renderer_h->geometry_pass.position_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    vulkan_color_image_create_info.format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_FORMAT;
    deferred_renderer_h->geometry_pass.normal_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    vulkan_color_image_create_info.format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_FORMAT;
    deferred_renderer_h->geometry_pass.albedo_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &deferred_renderer_h->geometry_pass.position_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &deferred_renderer_h->geometry_pass.normal_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &deferred_renderer_h->geometry_pass.albedo_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

    VkAttachmentDescription p_attachment_descriptions[4] = { 0 };

    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].flags = 0;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_FORMAT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].flags = 0;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_FORMAT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].flags = 0;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_FORMAT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].flags = 0;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].format = TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_FORMAT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].samples = VK_SAMPLE_COUNT_1_BIT;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    p_attachment_descriptions[TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference p_color_attachment_references[TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT] = { 0 };

    p_color_attachment_references[0].attachment = TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT;
    p_color_attachment_references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_color_attachment_references[1].attachment = TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT;
    p_color_attachment_references[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    p_color_attachment_references[2].attachment = TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT;
    p_color_attachment_references[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_buffer_attachment_reference = { 0 };
    depth_buffer_attachment_reference.attachment = TG_DEFERRED_RENDERER_GEOMETRY_PASS_DEPTH_ATTACHMENT;
    depth_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = TG_NULL;
    subpass_description.colorAttachmentCount = TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
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

    deferred_renderer_h->geometry_pass.render_pass = tg_vulkan_render_pass_create(TG_DEFERRED_RENDERER_GEOMETRY_PASS_ATTACHMENT_COUNT, p_attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency);

    const VkImageView p_framebuffer_attachments[] = {
        deferred_renderer_h->geometry_pass.position_attachment.image_view,
        deferred_renderer_h->geometry_pass.normal_attachment.image_view,
        deferred_renderer_h->geometry_pass.albedo_attachment.image_view,
        deferred_renderer_h->camera_h->render_target.depth_attachment.image_view
    };
    deferred_renderer_h->geometry_pass.framebuffer = tg_vulkan_framebuffer_create(deferred_renderer_h->geometry_pass.render_pass, TG_DEFERRED_RENDERER_GEOMETRY_PASS_ATTACHMENT_COUNT, p_framebuffer_attachments, swapchain_extent.width, swapchain_extent.height);

    deferred_renderer_h->geometry_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void tg_deferred_renderer_internal_init_shading_pass(tg_deferred_renderer_h deferred_renderer_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
    TG_ASSERT(deferred_renderer_h && point_light_count <= TG_DEFERRED_RENDERER_SHADING_PASS_MAX_POINT_LIGHTS && (point_light_count == 0 || p_point_lights));

    deferred_renderer_h->shading_pass.point_lights_ubo = tg_vulkan_buffer_create(sizeof(tg_deferred_renderer_light_setup_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_deferred_renderer_light_setup_uniform_buffer* p_light_setup_uniform_buffer = deferred_renderer_h->shading_pass.point_lights_ubo.p_mapped_device_memory;
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
    vulkan_color_image_create_info.width = swapchain_extent.width;
    vulkan_color_image_create_info.height = swapchain_extent.height;
    vulkan_color_image_create_info.mip_levels = 1;
    vulkan_color_image_create_info.format = TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_FORMAT;
    vulkan_color_image_create_info.min_filter = VK_FILTER_LINEAR;
    vulkan_color_image_create_info.mag_filter = VK_FILTER_LINEAR;
    vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    deferred_renderer_h->shading_pass.color_attachment = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &deferred_renderer_h->shading_pass.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

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
    memcpy(staging_buffer.p_mapped_device_memory, p_vertices, sizeof(p_vertices));
    deferred_renderer_h->shading_pass.vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, deferred_renderer_h->shading_pass.vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    u16 p_indices[6] = { 0 };

    p_indices[0] = 0;
    p_indices[1] = 1;
    p_indices[2] = 2;
    p_indices[3] = 2;
    p_indices[4] = 3;
    p_indices[5] = 0;

    staging_buffer = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_indices, sizeof(p_indices));
    deferred_renderer_h->shading_pass.ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, deferred_renderer_h->shading_pass.ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    deferred_renderer_h->shading_pass.rendering_finished_semaphore = tg_vulkan_semaphore_create();

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_FORMAT;
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

    deferred_renderer_h->shading_pass.render_pass = tg_vulkan_render_pass_create(TG_DEFERRED_RENDERER_SHADING_PASS_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    deferred_renderer_h->shading_pass.framebuffer = tg_vulkan_framebuffer_create(deferred_renderer_h->shading_pass.render_pass, TG_DEFERRED_RENDERER_SHADING_PASS_ATTACHMENT_COUNT, &deferred_renderer_h->shading_pass.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[4] = { 0 };

    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].binding = TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].descriptorCount = 1;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT].pImmutableSamplers = TG_NULL;

    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].binding = TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].descriptorCount = 1;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT].pImmutableSamplers = TG_NULL;

    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].binding = TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].descriptorCount = 1;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_descriptor_set_layout_bindings[TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT].pImmutableSamplers = TG_NULL;

    p_descriptor_set_layout_bindings[3].binding = 3;
    p_descriptor_set_layout_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_descriptor_set_layout_bindings[3].descriptorCount = 1;
    p_descriptor_set_layout_bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_descriptor_set_layout_bindings[3].pImmutableSamplers = TG_NULL;

    deferred_renderer_h->shading_pass.descriptor = tg_vulkan_descriptor_create(TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT + 1, p_descriptor_set_layout_bindings);
    deferred_renderer_h->shading_pass.vertex_shader = tg_vulkan_shader_module_create("shaders/shading.vert");
    deferred_renderer_h->shading_pass.fragment_shader = tg_vulkan_shader_module_create("shaders/shading.frag");
    deferred_renderer_h->shading_pass.pipeline_layout = tg_vulkan_pipeline_layout_create(1, &deferred_renderer_h->shading_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(tg_vulkan_screen_vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };

    p_vertex_input_attribute_descriptions[0].binding = 0;
    p_vertex_input_attribute_descriptions[0].location = 0;
    p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_vulkan_screen_vertex, position);

    p_vertex_input_attribute_descriptions[1].binding = 0;
    p_vertex_input_attribute_descriptions[1].location = 1;
    p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_vulkan_screen_vertex, uv);

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.vertex_shader = deferred_renderer_h->shading_pass.vertex_shader;
    vulkan_graphics_pipeline_create_info.fragment_shader = deferred_renderer_h->shading_pass.fragment_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.attachment_count = 1;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
    vulkan_graphics_pipeline_create_info.pipeline_layout = deferred_renderer_h->shading_pass.pipeline_layout;
    vulkan_graphics_pipeline_create_info.render_pass = deferred_renderer_h->shading_pass.render_pass;

    deferred_renderer_h->shading_pass.graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);

    tg_vulkan_descriptor_set_update_color_image(deferred_renderer_h->shading_pass.descriptor.descriptor_set, &deferred_renderer_h->geometry_pass.position_attachment, TG_DEFERRED_RENDERER_GEOMETRY_PASS_POSITION_ATTACHMENT);
    tg_vulkan_descriptor_set_update_color_image(deferred_renderer_h->shading_pass.descriptor.descriptor_set, &deferred_renderer_h->geometry_pass.normal_attachment, TG_DEFERRED_RENDERER_GEOMETRY_PASS_NORMAL_ATTACHMENT);
    tg_vulkan_descriptor_set_update_color_image(deferred_renderer_h->shading_pass.descriptor.descriptor_set, &deferred_renderer_h->geometry_pass.albedo_attachment, TG_DEFERRED_RENDERER_GEOMETRY_PASS_ALBEDO_ATTACHMENT);
    tg_vulkan_descriptor_set_update_uniform_buffer(deferred_renderer_h->shading_pass.descriptor.descriptor_set, deferred_renderer_h->shading_pass.point_lights_ubo.buffer, 3);

    deferred_renderer_h->shading_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    tg_vulkan_command_buffer_begin(deferred_renderer_h->shading_pass.command_buffer, 0, TG_NULL);
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.position_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.normal_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.albedo_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(deferred_renderer_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferred_renderer_h->shading_pass.graphics_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(deferred_renderer_h->shading_pass.command_buffer, 0, 1, &deferred_renderer_h->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(deferred_renderer_h->shading_pass.command_buffer, deferred_renderer_h->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(deferred_renderer_h->shading_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferred_renderer_h->shading_pass.pipeline_layout, 0, 1, &deferred_renderer_h->shading_pass.descriptor.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        render_pass_begin_info.renderPass = deferred_renderer_h->shading_pass.render_pass;
        render_pass_begin_info.framebuffer = deferred_renderer_h->shading_pass.framebuffer;
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = 0;
        render_pass_begin_info.pClearValues = TG_NULL;

        vkCmdBeginRenderPass(deferred_renderer_h->shading_pass.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(deferred_renderer_h->shading_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(deferred_renderer_h->shading_pass.command_buffer);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.position_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_clear_color_image(deferred_renderer_h->shading_pass.command_buffer, &deferred_renderer_h->geometry_pass.position_attachment);
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.position_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.normal_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_clear_color_image(deferred_renderer_h->shading_pass.command_buffer, &deferred_renderer_h->geometry_pass.normal_attachment);
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.normal_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.albedo_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tg_vulkan_command_buffer_cmd_clear_color_image(deferred_renderer_h->shading_pass.command_buffer, &deferred_renderer_h->geometry_pass.albedo_attachment);
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->shading_pass.command_buffer,
            &deferred_renderer_h->geometry_pass.albedo_attachment,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    VK_CALL(vkEndCommandBuffer(deferred_renderer_h->shading_pass.command_buffer));
}

void tg_deferred_renderer_internal_init_tone_mapping_pass(tg_deferred_renderer_h deferred_renderer_h)
{
    VkDescriptorSetLayoutBinding p_acquire_exposure_descriptor_set_layout_bindings[2] = { 0 };

    p_acquire_exposure_descriptor_set_layout_bindings[0].binding = 0;
    p_acquire_exposure_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    p_acquire_exposure_descriptor_set_layout_bindings[0].descriptorCount = 1;
    p_acquire_exposure_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_acquire_exposure_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

    p_acquire_exposure_descriptor_set_layout_bindings[1].binding = 1;
    p_acquire_exposure_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    p_acquire_exposure_descriptor_set_layout_bindings[1].descriptorCount = 1;
    p_acquire_exposure_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_acquire_exposure_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;

    deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader = tg_vulkan_compute_shader_create("shaders/acquire_exposure.comp", 2, p_acquire_exposure_descriptor_set_layout_bindings);
    deferred_renderer_h->tone_mapping_pass.exposure_storage_buffer = tg_vulkan_buffer_create(sizeof(f32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkAttachmentDescription attachment_description = { 0 };
    attachment_description.flags = 0;
    attachment_description.format = TG_DEFERRED_RENDERER_SHADING_PASS_COLOR_ATTACHMENT_FORMAT;
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

    deferred_renderer_h->tone_mapping_pass.render_pass = tg_vulkan_render_pass_create(TG_DEFERRED_RENDERER_SHADING_PASS_ATTACHMENT_COUNT, &attachment_description, 1, &subpass_description, 1, &subpass_dependency);
    deferred_renderer_h->tone_mapping_pass.framebuffer = tg_vulkan_framebuffer_create(deferred_renderer_h->tone_mapping_pass.render_pass, TG_DEFERRED_RENDERER_SHADING_PASS_ATTACHMENT_COUNT, &deferred_renderer_h->camera_h->render_target.color_attachment.image_view, swapchain_extent.width, swapchain_extent.height);

    VkDescriptorSetLayoutBinding p_adapt_exposure_descriptor_set_layout_bindings[2] = { 0 };

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

    deferred_renderer_h->tone_mapping_pass.descriptor = tg_vulkan_descriptor_create(2, p_adapt_exposure_descriptor_set_layout_bindings);
    deferred_renderer_h->tone_mapping_pass.vertex_shader = tg_vulkan_shader_module_create("shaders/adapt_exposure.vert");
    deferred_renderer_h->tone_mapping_pass.fragment_shader = tg_vulkan_shader_module_create("shaders/adapt_exposure.frag");
    deferred_renderer_h->tone_mapping_pass.pipeline_layout = tg_vulkan_pipeline_layout_create(1, &deferred_renderer_h->tone_mapping_pass.descriptor.descriptor_set_layout, 0, TG_NULL);

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(tg_vulkan_screen_vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[2] = { 0 };

    p_vertex_input_attribute_descriptions[0].binding = 0;
    p_vertex_input_attribute_descriptions[0].location = 0;
    p_vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[0].offset = offsetof(tg_vulkan_screen_vertex, position);

    p_vertex_input_attribute_descriptions[1].binding = 0;
    p_vertex_input_attribute_descriptions[1].location = 1;
    p_vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    p_vertex_input_attribute_descriptions[1].offset = offsetof(tg_vulkan_screen_vertex, uv);

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    tg_vulkan_graphics_pipeline_create_info exposure_vulkan_graphics_pipeline_create_info = { 0 };
    exposure_vulkan_graphics_pipeline_create_info.vertex_shader = deferred_renderer_h->tone_mapping_pass.vertex_shader;
    exposure_vulkan_graphics_pipeline_create_info.fragment_shader = deferred_renderer_h->tone_mapping_pass.fragment_shader;
    exposure_vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_NONE;
    exposure_vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    exposure_vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.attachment_count = 1;
    exposure_vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    exposure_vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
    exposure_vulkan_graphics_pipeline_create_info.pipeline_layout = deferred_renderer_h->tone_mapping_pass.pipeline_layout;
    exposure_vulkan_graphics_pipeline_create_info.render_pass = deferred_renderer_h->tone_mapping_pass.render_pass;

    deferred_renderer_h->tone_mapping_pass.graphics_pipeline = tg_vulkan_graphics_pipeline_create(&exposure_vulkan_graphics_pipeline_create_info);

    tg_vulkan_descriptor_set_update_color_image(deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader.descriptor.descriptor_set, &deferred_renderer_h->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader.descriptor.descriptor_set, deferred_renderer_h->tone_mapping_pass.exposure_storage_buffer.buffer, 1);
    tg_vulkan_descriptor_set_update_color_image(deferred_renderer_h->tone_mapping_pass.descriptor.descriptor_set, &deferred_renderer_h->shading_pass.color_attachment, 0);
    tg_vulkan_descriptor_set_update_storage_buffer(deferred_renderer_h->tone_mapping_pass.descriptor.descriptor_set, deferred_renderer_h->tone_mapping_pass.exposure_storage_buffer.buffer, 1);

    deferred_renderer_h->tone_mapping_pass.command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(deferred_renderer_h->tone_mapping_pass.command_buffer, 0, TG_NULL);
    {
        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->tone_mapping_pass.command_buffer,
            &deferred_renderer_h->shading_pass.color_attachment,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(deferred_renderer_h->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader.compute_pipeline);
        vkCmdBindDescriptorSets(deferred_renderer_h->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader.pipeline_layout, 0, 1, &deferred_renderer_h->tone_mapping_pass.acquire_exposure_compute_shader.descriptor.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(deferred_renderer_h->tone_mapping_pass.command_buffer, 1, 1, 1);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->tone_mapping_pass.command_buffer,
            &deferred_renderer_h->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        vkCmdBindPipeline(deferred_renderer_h->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferred_renderer_h->tone_mapping_pass.graphics_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(deferred_renderer_h->tone_mapping_pass.command_buffer, 0, 1, &deferred_renderer_h->shading_pass.vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(deferred_renderer_h->tone_mapping_pass.command_buffer, deferred_renderer_h->shading_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(deferred_renderer_h->tone_mapping_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, deferred_renderer_h->tone_mapping_pass.pipeline_layout, 0, 1, &deferred_renderer_h->tone_mapping_pass.descriptor.descriptor_set, 0, TG_NULL);

        VkRenderPassBeginInfo exposure_render_pass_begin_info = { 0 };
        exposure_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        exposure_render_pass_begin_info.pNext = TG_NULL;
        exposure_render_pass_begin_info.renderPass = deferred_renderer_h->tone_mapping_pass.render_pass;
        exposure_render_pass_begin_info.framebuffer = deferred_renderer_h->tone_mapping_pass.framebuffer;
        exposure_render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        exposure_render_pass_begin_info.renderArea.extent = swapchain_extent;
        exposure_render_pass_begin_info.clearValueCount = 0;
        exposure_render_pass_begin_info.pClearValues = TG_NULL;

        vkCmdBeginRenderPass(deferred_renderer_h->tone_mapping_pass.command_buffer, &exposure_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(deferred_renderer_h->tone_mapping_pass.command_buffer, 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(deferred_renderer_h->tone_mapping_pass.command_buffer);

        tg_vulkan_command_buffer_cmd_transition_color_image_layout(
            deferred_renderer_h->tone_mapping_pass.command_buffer,
            &deferred_renderer_h->shading_pass.color_attachment,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        );
    }
    vkEndCommandBuffer(deferred_renderer_h->tone_mapping_pass.command_buffer);
}



void tg_deferred_renderer_begin(tg_deferred_renderer_h deferred_renderer_h)
{
    TG_ASSERT(deferred_renderer_h);

    tg_vulkan_command_buffer_begin(deferred_renderer_h->geometry_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_begin_render_pass(deferred_renderer_h->geometry_pass.command_buffer, deferred_renderer_h->geometry_pass.render_pass, deferred_renderer_h->geometry_pass.framebuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

tg_deferred_renderer_h tg_deferred_renderer_create(tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
    TG_ASSERT(camera_h);

    tg_deferred_renderer_h deferred_renderer_h = TG_MEMORY_ALLOC(sizeof(*deferred_renderer_h));
    deferred_renderer_h->camera_h = camera_h;
    tg_deferred_renderer_internal_init_geometry_pass(deferred_renderer_h);
    tg_deferred_renderer_internal_init_shading_pass(deferred_renderer_h, point_light_count, p_point_lights);
    tg_deferred_renderer_internal_init_tone_mapping_pass(deferred_renderer_h);
    return deferred_renderer_h;
}

void tg_deferred_renderer_destroy(tg_deferred_renderer_h deferred_renderer_h)
{
    TG_ASSERT(deferred_renderer_h);

    tg_deferred_renderer_internal_destroy_tone_mapping_pass(deferred_renderer_h);
    tg_deferred_renderer_internal_destroy_shading_pass(deferred_renderer_h);
    tg_deferred_renderer_internal_destroy_geometry_pass(deferred_renderer_h);
    TG_MEMORY_FREE(deferred_renderer_h);
}

void tg_deferred_renderer_draw(tg_deferred_renderer_h deferred_renderer_h, tg_entity* p_entity, u32 entity_graphics_data_ptr_index)
{
    TG_ASSERT(deferred_renderer_h && p_entity && p_entity->graphics_data_ptr_h->p_entity_scene_infos[entity_graphics_data_ptr_index].renderer_h == deferred_renderer_h);

    vkCmdExecuteCommands(deferred_renderer_h->geometry_pass.command_buffer, 1, &p_entity->graphics_data_ptr_h->p_entity_scene_infos[entity_graphics_data_ptr_index].command_buffer);
}

void tg_deferred_renderer_end(tg_deferred_renderer_h deferred_renderer_h)
{
    TG_ASSERT(deferred_renderer_h);

    vkCmdEndRenderPass(deferred_renderer_h->geometry_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(deferred_renderer_h->geometry_pass.command_buffer));

    tg_vulkan_fence_wait(deferred_renderer_h->camera_h->render_target.fence);
    tg_vulkan_fence_reset(deferred_renderer_h->camera_h->render_target.fence);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &deferred_renderer_h->geometry_pass.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &deferred_renderer_h->shading_pass.rendering_finished_semaphore;

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, VK_NULL_HANDLE));

    const VkPipelineStageFlags p_pipeline_stage_flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo shading_submit_info = { 0 };
    shading_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    shading_submit_info.pNext = TG_NULL;
    shading_submit_info.waitSemaphoreCount = 1;
    shading_submit_info.pWaitSemaphores = &deferred_renderer_h->shading_pass.rendering_finished_semaphore;
    shading_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    shading_submit_info.commandBufferCount = 1;
    shading_submit_info.pCommandBuffers = &deferred_renderer_h->shading_pass.command_buffer;
    shading_submit_info.signalSemaphoreCount = 1;
    shading_submit_info.pSignalSemaphores = &deferred_renderer_h->shading_pass.rendering_finished_semaphore;

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &shading_submit_info, VK_NULL_HANDLE));

    VkSubmitInfo tone_mapping_submit_info = { 0 };
    tone_mapping_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    tone_mapping_submit_info.pNext = TG_NULL;
    tone_mapping_submit_info.waitSemaphoreCount = 1;
    tone_mapping_submit_info.pWaitSemaphores = &deferred_renderer_h->shading_pass.rendering_finished_semaphore;
    tone_mapping_submit_info.pWaitDstStageMask = p_pipeline_stage_flags;
    tone_mapping_submit_info.commandBufferCount = 1;
    tone_mapping_submit_info.pCommandBuffers = &deferred_renderer_h->tone_mapping_pass.command_buffer;
    tone_mapping_submit_info.signalSemaphoreCount = 0;
    tone_mapping_submit_info.pSignalSemaphores = TG_NULL;

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &tone_mapping_submit_info, deferred_renderer_h->camera_h->render_target.fence));
}

void tg_deferred_renderer_on_window_resize(tg_deferred_renderer_h deferred_renderer_h, u32 width, u32 height)
{
}

#endif
