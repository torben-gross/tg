#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_assets.h"



typedef struct tg_raytracer_ubo
{
    v4     u_camera_position;
    v4     u_ray00;
    v4     u_ray01;
    v4     u_ray10;
    v4     u_ray11;
    u32    u_directional_light_count;
    u32    u_point_light_count;
    u32    u_sphere_count; u32 padding;
    v4     u_directional_light_directions[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     u_directional_light_colors[TG_MAX_DIRECTIONAL_LIGHTS];
    v4     u_point_light_positions[TG_MAX_POINT_LIGHTS];
    v4     u_point_light_colors[TG_MAX_POINT_LIGHTS];
    v4     u_sphere_centers_radii[512];
} tg_raytracer_ubo;

typedef struct tg_raytracer_vertices
{
    u32    u_vertex_count;
    u32    u_vertex_float_count;
    u32    u_offset_floats_position;
    u32    u_offset_floats_normal;
    u32    u_offset_floats_uv;
    u32    u_offset_floats_tangent;
    u32    u_offset_floats_bitangent;
} tg_raytracer_vertices;



tg_raytracer_h tg_raytracer_create(tg_camera* p_camera)
{
	tg_raytracer_h h_raytracer = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_raytracers, h_raytracer);
    h_raytracer->type = TG_HANDLE_TYPE_RAYTRACER;

    h_raytracer->p_camera = p_camera;
    h_raytracer->semaphore = tg_vulkan_semaphore_create();
    h_raytracer->fence = tg_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT);
    h_raytracer->storage_image = tg_vulkan_storage_image_create(swapchain_extent.width, swapchain_extent.height, VK_FORMAT_R32G32B32A32_SFLOAT, TG_NULL);

    tg_vulkan_screen_vertex p_vertices[4] = { 0 };
    p_vertices[0].position = (v2){ -1.0f,  1.0f };
    p_vertices[0].uv       = (v2){  0.0f,  1.0f };
    p_vertices[1].position = (v2){  1.0f,  1.0f };
    p_vertices[1].uv       = (v2){  1.0f,  1.0f };
    p_vertices[2].position = (v2){  1.0f, -1.0f };
    p_vertices[2].uv       = (v2){  1.0f,  0.0f };
    p_vertices[3].position = (v2){ -1.0f, -1.0f };
    p_vertices[3].uv       = (v2){  0.0f,  0.0f };

    const u16 p_indices[6] = { 0, 1, 2, 2, 3, 0 };

    // TODO: is a global staging buffer possible?
    tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(tgm_u64_max(sizeof(p_vertices), sizeof(p_indices)), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    tg_memory_copy(sizeof(p_vertices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
    tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
    h_raytracer->screen_quad_vbo = tg_vulkan_buffer_create(sizeof(p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_vertices), staging_buffer.buffer, h_raytracer->screen_quad_vbo.buffer);
    tg_memory_copy(sizeof(p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
    tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
    h_raytracer->screen_quad_ibo = tg_vulkan_buffer_create(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(sizeof(p_indices), staging_buffer.buffer, h_raytracer->screen_quad_ibo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    h_raytracer->raytrace_pass.compute_pipeline = tg_vulkan_pipeline_create_compute(&((tg_compute_shader_h)tg_assets_get_asset("shaders/raytracer.comp"))->vulkan_shader);
    h_raytracer->raytrace_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    h_raytracer->raytrace_pass.ubo = tg_vulkan_buffer_create(sizeof(tg_raytracer_ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    tg_mesh_h h_mesh = tg_mesh_load("meshes/sponza.obj", V3(0.01f));

    staging_buffer = tg_vulkan_buffer_create(h_mesh->vertex_count * sizeof(tg_vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(h_mesh->vertex_count * sizeof(tg_vertex), h_mesh->vbo.buffer, staging_buffer.buffer);

    tg_vulkan_buffer ubo = tg_vulkan_buffer_create(sizeof(tg_raytracer_vertices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_vertex_count            = h_mesh->vertex_count;
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_vertex_float_count      = sizeof(tg_vertex) / sizeof(f32);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_offset_floats_position  = offsetof(tg_vertex, position) / sizeof(f32);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_offset_floats_normal    = offsetof(tg_vertex, normal) / sizeof(f32);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_offset_floats_uv        = offsetof(tg_vertex, uv) / sizeof(f32);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_offset_floats_tangent   = offsetof(tg_vertex, tangent) / sizeof(f32);
    ((tg_raytracer_vertices*)ubo.memory.p_mapped_device_memory)->u_offset_floats_bitangent = offsetof(tg_vertex, bitangent) / sizeof(f32);

    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = h_raytracer->raytrace_pass.ubo.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = h_raytracer->raytrace_pass.compute_pipeline.descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = TG_NULL;
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set); // TODO: update color image

    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = h_raytracer->storage_image.sampler;
    descriptor_image_info.imageView = h_raytracer->storage_image.image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    write_descriptor_set.dstBinding = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set); // TODO: update color image

    descriptor_buffer_info.buffer = ubo.buffer;
    write_descriptor_set.dstBinding = 2;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    descriptor_buffer_info.buffer = staging_buffer.buffer;
    write_descriptor_set.dstBinding = 3;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set);

    h_raytracer->present_pass.image_acquired_semaphore = tg_vulkan_semaphore_create();

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

    h_raytracer->present_pass.render_pass = tg_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, TG_NULL);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        h_raytracer->present_pass.p_framebuffers[i] = tg_vulkan_framebuffer_create(h_raytracer->present_pass.render_pass, 1, &p_swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height);
    }

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.p_vertex_shader = &((tg_vertex_shader_h)tg_assets_get_asset("shaders/screen_quad.vert"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.p_fragment_shader = &((tg_fragment_shader_h)tg_assets_get_asset("shaders/present.frag"))->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    vulkan_graphics_pipeline_create_info.render_pass = h_raytracer->present_pass.render_pass;
    vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)swapchain_extent.width;
    vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)swapchain_extent.height;

    h_raytracer->present_pass.graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);
    tg_vulkan_command_buffers_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY, TG_VULKAN_SURFACE_IMAGE_COUNT, h_raytracer->present_pass.p_command_buffers);

    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    write_descriptor_set.dstSet = h_raytracer->present_pass.graphics_pipeline.descriptor_set;
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    
    tg_vulkan_descriptor_sets_update(1, &write_descriptor_set); // TODO: update color image

    tg_vulkan_command_buffer_begin(h_raytracer->raytrace_pass.command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);

    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = TG_NULL;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = h_raytracer->storage_image.image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1; // TODO: mipmapping
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(h_raytracer->raytrace_pass.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
    tg_vulkan_command_buffer_end_and_submit(h_raytracer->raytrace_pass.command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);

    tg_vulkan_command_buffer_begin(h_raytracer->raytrace_pass.command_buffer, 0, TG_NULL);
    vkCmdBindPipeline(h_raytracer->raytrace_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_raytracer->raytrace_pass.compute_pipeline.pipeline);
    vkCmdBindDescriptorSets(h_raytracer->raytrace_pass.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_raytracer->raytrace_pass.compute_pipeline.pipeline_layout, 0, 1, &h_raytracer->raytrace_pass.compute_pipeline.descriptor_set, 0, TG_NULL);
    vkCmdDispatch(h_raytracer->raytrace_pass.command_buffer, swapchain_extent.width / 32, swapchain_extent.height / 32, 1);
    vkEndCommandBuffer(h_raytracer->raytrace_pass.command_buffer);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        tg_vulkan_command_buffer_begin(h_raytracer->present_pass.p_command_buffers[i], 0, TG_NULL);

        // TODO: put all these barriers into color image transition
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = TG_NULL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = h_raytracer->storage_image.image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1; // TODO: mipmapping
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(h_raytracer->present_pass.p_command_buffers[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindPipeline(h_raytracer->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_raytracer->present_pass.graphics_pipeline.pipeline);
        vkCmdBindVertexBuffers(h_raytracer->present_pass.p_command_buffers[i], 0, 1, &h_raytracer->screen_quad_vbo.buffer, &vertex_buffer_offset);
        vkCmdBindIndexBuffer(h_raytracer->present_pass.p_command_buffers[i], h_raytracer->screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(h_raytracer->present_pass.p_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, h_raytracer->present_pass.graphics_pipeline.pipeline_layout, 0, 1, &h_raytracer->present_pass.graphics_pipeline.descriptor_set, 0, TG_NULL);
        tg_vulkan_command_buffer_cmd_begin_render_pass(h_raytracer->present_pass.p_command_buffers[i], h_raytracer->present_pass.render_pass, &h_raytracer->present_pass.p_framebuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(h_raytracer->present_pass.p_command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(h_raytracer->present_pass.p_command_buffers[i]);

        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = TG_NULL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = h_raytracer->storage_image.image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1; // TODO: mipmapping
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(h_raytracer->present_pass.p_command_buffers[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);

        VK_CALL(vkEndCommandBuffer(h_raytracer->present_pass.p_command_buffers[i]));
    }

	return h_raytracer;
}

void tg_raytracer_begin(tg_raytracer_h h_raytracer)
{
    TG_ASSERT(h_raytracer);

    tg_vulkan_fence_wait(h_raytracer->fence);
    tg_vulkan_fence_reset(h_raytracer->fence);

    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_directional_light_count = 0;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_point_light_count = 0;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_sphere_count = 0;
}

void tg_raytracer_push_directional_light(tg_raytracer_h h_raytracer, v3 direction, v3 color)
{
    TG_ASSERT(h_raytracer && tgm_v3_magsqr(direction) && tgm_v3_magsqr(color));

    const u32 i = ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_directional_light_count;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_directional_light_directions[i].xyz = direction;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_directional_light_colors[i].xyz = color;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_directional_light_count++;
}

void tg_raytracer_push_point_light(tg_raytracer_h h_raytracer, v3 position, v3 color)
{
    TG_ASSERT(h_raytracer && tgm_v3_magsqr(color));

    const u32 i = ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_point_light_count;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_point_light_positions[i].xyz = position;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_point_light_colors[i].xyz = color;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_point_light_count++;
}

void tg_raytracer_push_sphere(tg_raytracer_h h_raytracer, v3 center, f32 radius)
{
    TG_ASSERT(h_raytracer && radius > 0.0f);

    const u32 i = ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_sphere_count;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_sphere_centers_radii[i].xyz = center;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_sphere_centers_radii[i].w = radius;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_sphere_count++;
}

void tg_raytracer_end(tg_raytracer_h h_raytracer)
{
    TG_ASSERT(h_raytracer);

    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_camera_position.xyz = h_raytracer->p_camera->position;

    const m4 view = tgm_m4_mul(
        tgm_m4_inverse(tgm_m4_euler(h_raytracer->p_camera->pitch, h_raytracer->p_camera->yaw, h_raytracer->p_camera->roll)),
        tgm_m4_translate(tgm_v3_neg(h_raytracer->p_camera->position))
    );

    const m4 proj = tgm_m4_perspective(
        h_raytracer->p_camera->perspective.fov_y_in_radians,
        h_raytracer->p_camera->perspective.aspect,
        h_raytracer->p_camera->perspective.near,
        h_raytracer->p_camera->perspective.far
    );

    const m4 inv_view_proj = tgm_m4_inverse(tgm_m4_mul(proj, view));

    v4 ray00 = { -1.0f,  1.0f,  1.0f,  1.0f };
    v4 ray01 = { -1.0f, -1.0f,  1.0f,  1.0f };
    v4 ray10 = {  1.0f,  1.0f,  1.0f,  1.0f };
    v4 ray11 = {  1.0f, -1.0f,  1.0f,  1.0f };

    ray00 = tgm_m4_mulv4(inv_view_proj, ray00);
    ray01 = tgm_m4_mulv4(inv_view_proj, ray01);
    ray10 = tgm_m4_mulv4(inv_view_proj, ray10);
    ray11 = tgm_m4_mulv4(inv_view_proj, ray11);

    ray00.xyz = tgm_v3_divf(ray00.xyz, ray00.w); ray00.w = 0.0f;
    ray01.xyz = tgm_v3_divf(ray01.xyz, ray01.w); ray01.w = 0.0f;
    ray10.xyz = tgm_v3_divf(ray10.xyz, ray10.w); ray10.w = 0.0f;
    ray11.xyz = tgm_v3_divf(ray11.xyz, ray11.w); ray11.w = 0.0f;

    ray00.xyz = tgm_v3_sub(ray00.xyz, h_raytracer->p_camera->position);
    ray01.xyz = tgm_v3_sub(ray01.xyz, h_raytracer->p_camera->position);
    ray10.xyz = tgm_v3_sub(ray10.xyz, h_raytracer->p_camera->position);
    ray11.xyz = tgm_v3_sub(ray11.xyz, h_raytracer->p_camera->position);

    ray00.xyz = tgm_v3_normalized(ray00.xyz);
    ray01.xyz = tgm_v3_normalized(ray01.xyz);
    ray10.xyz = tgm_v3_normalized(ray10.xyz);
    ray11.xyz = tgm_v3_normalized(ray11.xyz);

    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_ray00 = ray00;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_ray01 = ray01;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_ray10 = ray10;
    ((tg_raytracer_ubo*)h_raytracer->raytrace_pass.ubo.memory.p_mapped_device_memory)->u_ray11 = ray11;

    tg_vulkan_buffer_flush_mapped_memory(&h_raytracer->raytrace_pass.ubo);

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &h_raytracer->raytrace_pass.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &h_raytracer->semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_COMPUTE, 1, &submit_info, VK_NULL_HANDLE);
}

void tg_raytracer_present(tg_raytracer_h h_raytracer)
{
    TG_ASSERT(h_raytracer);

    u32 current_image = TG_U32_MAX;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, h_raytracer->present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));
    TG_ASSERT(current_image != TG_U32_MAX);

    const VkSemaphore p_wait_semaphores[2] = { h_raytracer->semaphore, h_raytracer->present_pass.image_acquired_semaphore };
    const VkPipelineStageFlags p_pipeline_stage_masks[2] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

    VkSubmitInfo draw_submit_info = { 0 };
    draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    draw_submit_info.pNext = TG_NULL;
    draw_submit_info.waitSemaphoreCount = 2;
    draw_submit_info.pWaitSemaphores = p_wait_semaphores;
    draw_submit_info.pWaitDstStageMask = p_pipeline_stage_masks;
    draw_submit_info.commandBufferCount = 1;
    draw_submit_info.pCommandBuffers = &h_raytracer->present_pass.p_command_buffers[current_image];
    draw_submit_info.signalSemaphoreCount = 1;
    draw_submit_info.pSignalSemaphores = &h_raytracer->semaphore;

    tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_GRAPHICS, 1, &draw_submit_info, h_raytracer->fence);

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = TG_NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &h_raytracer->semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pResults = TG_NULL;

    tg_vulkan_queue_present(&present_info);
}

void tg_raytracer_get_render_target(tg_raytracer_h h_raytracer)
{

}

void tg_raytracer_destroy(tg_raytracer_h h_raytracer)
{

}

#endif
