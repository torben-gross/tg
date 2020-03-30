#include "tg/graphics/tg_graphics_vulkan_renderer_3d.h"

#ifdef TG_VULKAN

typedef struct tg_renderer_3d_geometry_pass
{
    struct
    {
        VkImage           image;
        VkDeviceMemory    device_memory;
        VkImageView       image_view;
    } g_buffer;

    struct
    {
        VkImage           image;
        VkDeviceMemory    device_memory;
        VkImageView       image_view;
    } z_buffer;

    // TODO
    //struct
    //{
    //    VkImage           image;
    //    VkDeviceMemory    device_memory;
    //    VkImageView       image_view;
    //    VkSampler         sampler;
    //} normal_buffer;

    VkFence               rendering_finished_fence;
    VkSemaphore           rendering_finished_semaphore;

    VkRenderPass          render_pass;
    VkFramebuffer         framebuffer;
} tg_renderer_3d_geometry_pass;

typedef struct tg_renderer_3d_shading_pass
{
    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } vbo;

    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } ibo;

    VkSemaphore              image_acquired_semaphore;
    VkFence                  rendering_finished_fence;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffers[SURFACE_IMAGE_COUNT];

    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;

    VkShaderModule           vertex_shader;
    VkShaderModule           fragment_shader;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;

    VkCommandBuffer          command_buffers[SURFACE_IMAGE_COUNT];
} tg_renderer_3d_shading_pass;

typedef struct tg_renderer_3d_clear_pass
{
    VkCommandBuffer    command_buffer;
} tg_renderer_3d_clear_pass;

typedef struct tg_renderer_3d_present_pass
{
    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } vbo;

    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } ibo;

    VkSemaphore              image_acquired_semaphore;
    VkFence                  rendering_finished_fence;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffers[SURFACE_IMAGE_COUNT];

    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;

    VkShaderModule           vertex_shader;
    VkShaderModule           fragment_shader;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;

    VkCommandBuffer          command_buffers[SURFACE_IMAGE_COUNT];
} tg_renderer_3d_present_pass;

typedef struct tg_renderer_3d_resolve_pass
{
    struct
    {
        VkImage           image;
        VkDeviceMemory    device_memory;
        VkImageView       image_view;
        VkSampler         sampler;
    } g_buffer;

    VkCommandBuffer       command_buffer;
} tg_renderer_3d_resolve_pass;

typedef struct tg_renderer_3d_present_pass_vertex
{
    tgm_vec2f    position;
    tgm_vec2f    uv;
} tg_renderer_3d_present_pass_vertex;



const tg_camera*                p_main_camera;
tg_renderer_3d_geometry_pass    geometry_pass;
tg_renderer_3d_shading_pass     shading_pass;
tg_renderer_3d_present_pass     present_pass;
tg_renderer_3d_clear_pass       clear_pass;
tg_renderer_3d_resolve_pass     resolve_pass;



void tg_graphics_vulkan_renderer_3d_get_geometry_framebuffer(VkFramebuffer* p_framebuffer)
{
    *p_framebuffer = geometry_pass.framebuffer;
}
void tg_graphics_vulkan_renderer_3d_get_geometry_render_pass(VkRenderPass* p_render_pass)
{
    *p_render_pass = geometry_pass.render_pass;
}

// TODO: resolve and clear pass should be part of present pass: although, presenting should only be done in main vulkan file.
void tg_graphics_renderer_3d_internal_init_geometry_pass()
{
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, surface.format.format, surface.msaa_sample_count, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.g_buffer.image, &geometry_pass.g_buffer.device_memory);
    tg_graphics_vulkan_image_view_create(geometry_pass.g_buffer.image, surface.format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &geometry_pass.g_buffer.image_view);

    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    tg_graphics_vulkan_depth_format_acquire(&depth_format);
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, depth_format, surface.msaa_sample_count, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.z_buffer.image, &geometry_pass.z_buffer.device_memory);
    tg_graphics_vulkan_image_view_create(geometry_pass.z_buffer.image, depth_format, 1, VK_IMAGE_ASPECT_DEPTH_BIT, &geometry_pass.z_buffer.image_view);

    // TODO: normal-buffer

    tg_graphics_vulkan_image_transition_layout(geometry_pass.g_buffer.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_image_transition_layout(geometry_pass.z_buffer.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &geometry_pass.rendering_finished_fence);
    tg_graphics_vulkan_semaphore_create(&geometry_pass.rendering_finished_semaphore);

    VkAttachmentReference g_buffer_attachment_reference = { 0 };
    {
        g_buffer_attachment_reference.attachment = 0;
        g_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference z_buffer_attachment_reference = { 0 };
    {
        z_buffer_attachment_reference.attachment = 1;
        z_buffer_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    // TODO: normal-buffer
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = NULL;
        subpass_description.colorAttachmentCount = 1; // TODO: this will increase
        subpass_description.pColorAttachments = &g_buffer_attachment_reference;
        subpass_description.pResolveAttachments = NULL;
        subpass_description.pDepthStencilAttachment = &z_buffer_attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = NULL;
    }
    VkAttachmentDescription attachment_descriptions[2] = { 0 };
    {
        attachment_descriptions[0].flags = 0;
        attachment_descriptions[0].format = surface.format.format;
        attachment_descriptions[0].samples = surface.msaa_sample_count;
        attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_descriptions[1].flags = 0;
        attachment_descriptions[1].format = depth_format;
        attachment_descriptions[1].samples = surface.msaa_sample_count;
        attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
    tg_graphics_vulkan_render_pass_create(sizeof(attachment_descriptions) / sizeof(*attachment_descriptions), attachment_descriptions, 1, &subpass_description, 1, &subpass_dependency, &geometry_pass.render_pass);

    const VkImageView framebuffer_attachments[] = {
        geometry_pass.g_buffer.image_view,
        geometry_pass.z_buffer.image_view
    };
    tg_graphics_vulkan_framebuffer_create(geometry_pass.render_pass, sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments), framebuffer_attachments, swapchain_extent.width, swapchain_extent.height, &geometry_pass.framebuffer);
}
void tg_graphics_renderer_3d_internal_init_shading_pass()
{
}
void tg_graphics_renderer_3d_internal_init_resolve_pass()
{
    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, surface.format.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &resolve_pass.g_buffer.image, &resolve_pass.g_buffer.device_memory);
    tg_graphics_vulkan_image_view_create(resolve_pass.g_buffer.image, surface.format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &resolve_pass.g_buffer.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &resolve_pass.g_buffer.sampler);
    tg_graphics_vulkan_image_transition_layout(resolve_pass.g_buffer.image, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &resolve_pass.command_buffer);
    tg_graphics_vulkan_command_buffer_begin(0, resolve_pass.command_buffer);

    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(resolve_pass.command_buffer, geometry_pass.g_buffer.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(resolve_pass.command_buffer, resolve_pass.g_buffer.image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageResolve region = { 0 };
    {
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffset = (VkOffset3D){ 0, 0, 0 };
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.dstOffset = (VkOffset3D){ 0, 0, 0 };
        region.extent.width = swapchain_extent.width;
        region.extent.height = swapchain_extent.height;
        region.extent.depth = 1;
    }
    vkCmdResolveImage(resolve_pass.command_buffer, geometry_pass.g_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, resolve_pass.g_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(resolve_pass.command_buffer, geometry_pass.g_buffer.image, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(resolve_pass.command_buffer, resolve_pass.g_buffer.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

    VK_CALL(vkEndCommandBuffer(resolve_pass.command_buffer));// TODO: make function for that in vulkan.h somewhere
}
void tg_graphics_renderer_3d_internal_init_present_pass()
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    void* data;

    tg_renderer_3d_present_pass_vertex vertices[4] = { 0 };
    {
        // TODO: y is inverted, because image has to be flipped. add projection matrix to present vertex shader?
        vertices[0].position.x = -1.0f;
        vertices[0].position.y = -1.0f;
        vertices[0].uv.x       =  0.0f;
        vertices[0].uv.y       =  0.0f;

        vertices[1].position.x =  1.0f;
        vertices[1].position.y = -1.0f;
        vertices[1].uv.x       =  1.0f;
        vertices[1].uv.y       =  0.0f;

        vertices[2].position.x =  1.0f;
        vertices[2].position.y =  1.0f;
        vertices[2].uv.x       =  1.0f;
        vertices[2].uv.y       =  1.0f;

        vertices[3].position.x = -1.0f;
        vertices[3].position.y =  1.0f;
        vertices[3].uv.x       =  0.0f;
        vertices[3].uv.y       =  1.0f;
    }                     
    tg_graphics_vulkan_buffer_create(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(vertices), 0, &data));
    {
        memcpy(data, vertices, sizeof(vertices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &present_pass.vbo.buffer, &present_pass.vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(vertices), staging_buffer, present_pass.vbo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    ui16 indices[6] = { 0 };
    {
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 2;
        indices[4] = 3;
        indices[5] = 0;
    }
    tg_graphics_vulkan_buffer_create(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(indices), 0, &data));
    {
        memcpy(data, indices, sizeof(indices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &present_pass.ibo.buffer, &present_pass.ibo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(indices), staging_buffer, present_pass.ibo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    tg_graphics_vulkan_semaphore_create(&present_pass.image_acquired_semaphore);
    tg_graphics_vulkan_fence_create(VK_FENCE_CREATE_SIGNALED_BIT, &present_pass.rendering_finished_fence);
    tg_graphics_vulkan_semaphore_create(&present_pass.rendering_finished_semaphore);

    VkAttachmentReference color_attachment_reference = { 0 };
    {
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = NULL;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_attachment_reference;
        subpass_description.pResolveAttachments = NULL;
        subpass_description.pDepthStencilAttachment = NULL;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = NULL;
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
    tg_graphics_vulkan_render_pass_create(1, &attachment_description, 1, &subpass_description, 0, NULL, &present_pass.render_pass);

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        tg_graphics_vulkan_framebuffer_create(present_pass.render_pass, 1, &swapchain_image_views[i], swapchain_extent.width, swapchain_extent.height, &present_pass.framebuffers[i]);
    }

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_size.descriptorCount = 1;
    }
    tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &present_pass.descriptor_pool);

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = NULL;
    }
    tg_graphics_vulkan_descriptor_set_layout_create(0, 1, &descriptor_set_layout_binding, &present_pass.descriptor_set_layout);
    tg_graphics_vulkan_descriptor_set_allocate(present_pass.descriptor_pool, present_pass.descriptor_set_layout, &present_pass.descriptor_set);
    
    tg_graphics_vulkan_shader_module_create("shaders/present_vert.spv", &present_pass.vertex_shader);
    tg_graphics_vulkan_shader_module_create("shaders/present_frag.spv", &present_pass.fragment_shader);

    tg_graphics_vulkan_pipeline_layout_create(1, &present_pass.descriptor_set_layout, 0, NULL, &present_pass.pipeline_layout);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[2] = { 0 };
    {
        pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[0].pNext = NULL;
        pipeline_shader_stage_create_infos[0].flags = 0;
        pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_shader_stage_create_infos[0].module = present_pass.vertex_shader;
        pipeline_shader_stage_create_infos[0].pName = "main";
        pipeline_shader_stage_create_infos[0].pSpecializationInfo = NULL;
        pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[1].pNext = NULL;
        pipeline_shader_stage_create_infos[1].flags = 0;
        pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_shader_stage_create_infos[1].module = present_pass.fragment_shader;
        pipeline_shader_stage_create_infos[1].pName = "main";
        pipeline_shader_stage_create_infos[1].pSpecializationInfo = NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_renderer_3d_present_pass_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[2] = { 0 };
    {
        vertex_input_attribute_descriptions[0].binding = 0;
        vertex_input_attribute_descriptions[0].location = 0;
        vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attribute_descriptions[0].offset = offsetof(tg_renderer_3d_present_pass_vertex, position);
        vertex_input_attribute_descriptions[1].binding = 0;
        vertex_input_attribute_descriptions[1].location = 1;
        vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attribute_descriptions[1].offset = offsetof(tg_renderer_3d_present_pass_vertex, uv);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = 2;
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;
    }
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    {
        pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline_input_assembly_state_create_info.pNext = NULL;
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
        pipeline_viewport_state_create_info.pNext = NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissors;
    }
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    {
        pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_rasterization_state_create_info.pNext = NULL;
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
        pipeline_multisample_state_create_info.pNext = NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        pipeline_multisample_state_create_info.minSampleShading = 0.0f;
        pipeline_multisample_state_create_info.pSampleMask = NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = NULL;
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
        pipeline_color_blend_state_create_info.pNext = NULL;
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
        graphics_pipeline_create_info.pNext = NULL;
        graphics_pipeline_create_info.flags = 0;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = NULL;
        graphics_pipeline_create_info.layout = present_pass.pipeline_layout;
        graphics_pipeline_create_info.renderPass = present_pass.render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    tg_graphics_vulkan_pipeline_create(VK_NULL_HANDLE, &graphics_pipeline_create_info, &present_pass.pipeline);

    tg_graphics_vulkan_command_buffers_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, SURFACE_IMAGE_COUNT, present_pass.command_buffers);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
    const VkDeviceSize vertex_buffer_offsets[1] = { 0 };
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = resolve_pass.g_buffer.sampler;
        descriptor_image_info.imageView = resolve_pass.g_buffer.image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = NULL;
        write_descriptor_set.dstSet = present_pass.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set.pImageInfo = &descriptor_image_info;
        write_descriptor_set.pBufferInfo = NULL;
        write_descriptor_set.pTexelBufferView = NULL;
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {

        VK_CALL(vkBeginCommandBuffer(present_pass.command_buffers[i], &command_buffer_begin_info));

        vkCmdBindPipeline(present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, present_pass.pipeline);
        vkCmdBindVertexBuffers(present_pass.command_buffers[i], 0, sizeof(vertex_buffer_offsets) / sizeof(*vertex_buffer_offsets), &present_pass.vbo.buffer, vertex_buffer_offsets);
        vkCmdBindIndexBuffer(present_pass.command_buffers[i], present_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(present_pass.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, present_pass.pipeline_layout, 0, 1, &present_pass.descriptor_set, 0, NULL);

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        {
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = NULL;
            render_pass_begin_info.renderPass = present_pass.render_pass;
            render_pass_begin_info.framebuffer = present_pass.framebuffers[i];
            render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
            render_pass_begin_info.renderArea.extent = swapchain_extent;
            render_pass_begin_info.clearValueCount = 0;
            render_pass_begin_info.pClearValues = NULL;
        }
        vkCmdBeginRenderPass(present_pass.command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(present_pass.command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(present_pass.command_buffers[i]);
        VK_CALL(vkEndCommandBuffer(present_pass.command_buffers[i]));
    }
}
void tg_graphics_renderer_3d_internal_init_clear_pass()
{
    tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &clear_pass.command_buffer);
    tg_graphics_vulkan_command_buffer_begin(0, clear_pass.command_buffer);

    const VkClearColorValue clear_color_value = { 0.5294117647f, 0.80784313725f, 0.98039215686f, 1.0f };
    const VkClearDepthStencilValue clear_depth_stencil_value = { 1.0f, 0 };
    VkImageSubresourceRange g_buffer_image_subresource_range = { 0 };
    {
        g_buffer_image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        g_buffer_image_subresource_range.baseMipLevel = 0;
        g_buffer_image_subresource_range.levelCount = 1;
        g_buffer_image_subresource_range.baseArrayLayer = 0;
        g_buffer_image_subresource_range.layerCount = 1;
    }
    VkImageSubresourceRange z_buffer_image_subresource_range = { 0 };
    {
        z_buffer_image_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        z_buffer_image_subresource_range.baseMipLevel = 0;
        z_buffer_image_subresource_range.levelCount = 1;
        z_buffer_image_subresource_range.baseArrayLayer = 0;
        z_buffer_image_subresource_range.layerCount = 1;
    }

    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(clear_pass.command_buffer, geometry_pass.g_buffer.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdClearColorImage(clear_pass.command_buffer, geometry_pass.g_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &g_buffer_image_subresource_range);
    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(clear_pass.command_buffer, geometry_pass.g_buffer.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(clear_pass.command_buffer, geometry_pass.z_buffer.image, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdClearDepthStencilImage(clear_pass.command_buffer, geometry_pass.z_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &z_buffer_image_subresource_range);
    tg_graphics_vulkan_command_buffer_cmd_transition_image_layout(clear_pass.command_buffer, geometry_pass.z_buffer.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    // TODO: normal-buffer

    VK_CALL(vkEndCommandBuffer(clear_pass.command_buffer));
}

void tg_graphics_renderer_3d_init(const tg_camera* p_camera)
{
    p_main_camera = p_camera;
    tg_graphics_renderer_3d_internal_init_geometry_pass();
    tg_graphics_renderer_3d_internal_init_shading_pass();
    tg_graphics_renderer_3d_internal_init_resolve_pass();
    tg_graphics_renderer_3d_internal_init_present_pass();
    tg_graphics_renderer_3d_internal_init_clear_pass();
}
void tg_graphics_renderer_3d_draw(const tg_model_h model_h)
{
    // TODO: full geometry pass

    tg_uniform_buffer_object* p_uniform_buffer_object = NULL;
    VK_CALL(vkMapMemory(device, model_h->material->ubo.device_memory, 0, sizeof(*p_uniform_buffer_object), 0, &p_uniform_buffer_object));
    {
        const tgm_vec3f translation_vector = { 0.0f, 0.0f, -9.0f };
        tgm_m4f_translate(&p_uniform_buffer_object->model, &translation_vector);
        p_uniform_buffer_object->view = p_main_camera->view;
        p_uniform_buffer_object->projection = p_main_camera->projection;
    }
    vkUnmapMemory(device, model_h->material->ubo.device_memory);

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &model_h->command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;
    }
    VK_CALL(vkResetFences(device, 1, &geometry_pass.rendering_finished_fence));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, geometry_pass.rendering_finished_fence));
    VK_CALL(vkWaitForFences(device, 1, &geometry_pass.rendering_finished_fence, VK_TRUE, UINT64_MAX));
}
void tg_graphics_renderer_3d_present()
{
    // TODO: shading pass
    VkSubmitInfo resolve_submit_info = { 0 };
    {
        resolve_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        resolve_submit_info.pNext = NULL;
        resolve_submit_info.waitSemaphoreCount = 0;
        resolve_submit_info.pWaitSemaphores = NULL;
        resolve_submit_info.pWaitDstStageMask = NULL;
        resolve_submit_info.commandBufferCount = 1;
        resolve_submit_info.pCommandBuffers = &resolve_pass.command_buffer;
        resolve_submit_info.signalSemaphoreCount = 0;
        resolve_submit_info.pSignalSemaphores = NULL; // TODO: add and remove queue wait idle
    }
    vkQueueSubmit(graphics_queue.queue, 1, &resolve_submit_info, NULL);
    VK_CALL(vkQueueWaitIdle(graphics_queue.queue));

    ui32 current_image;
    VK_CALL(vkWaitForFences(device, 1, &geometry_pass.rendering_finished_fence, VK_TRUE, UINT64_MAX));
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, present_pass.image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    const VkPipelineStageFlags pipeline_stage_mask = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo draw_submit_info = { 0 };
    {
        draw_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        draw_submit_info.pNext = NULL;
        draw_submit_info.waitSemaphoreCount = 1;
        draw_submit_info.pWaitSemaphores = &present_pass.image_acquired_semaphore;
        draw_submit_info.pWaitDstStageMask = &pipeline_stage_mask;
        draw_submit_info.commandBufferCount = 1;
        draw_submit_info.pCommandBuffers = &present_pass.command_buffers[current_image];
        draw_submit_info.signalSemaphoreCount = 1;
        draw_submit_info.pSignalSemaphores = &present_pass.rendering_finished_semaphore;
    }
    VK_CALL(vkResetFences(device, 1, &present_pass.rendering_finished_fence));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &draw_submit_info, present_pass.rendering_finished_fence));
    VK_CALL(vkWaitForFences(device, 1, &present_pass.rendering_finished_fence, VK_TRUE, UINT64_MAX));

    VkPresentInfoKHR present_info = { 0 };
    {
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &present_pass.rendering_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = NULL;
    }
    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
    VK_CALL(vkQueueWaitIdle(graphics_queue.queue));

    VkSubmitInfo clear_submit_info = { 0 };
    {
        clear_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        clear_submit_info.pNext = NULL;
        clear_submit_info.waitSemaphoreCount = 0;
        clear_submit_info.pWaitSemaphores = NULL;
        clear_submit_info.pWaitDstStageMask = NULL;
        clear_submit_info.commandBufferCount = 1;
        clear_submit_info.pCommandBuffers = &clear_pass.command_buffer;
        clear_submit_info.signalSemaphoreCount = 0;
        clear_submit_info.pSignalSemaphores = NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &clear_submit_info, NULL));
    VK_CALL(vkQueueWaitIdle(graphics_queue.queue));
}
void tg_graphics_renderer_3d_shutdown()
{

}

#endif
