#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_renderer_3d_geometry_pass
{
    struct
    {
        VkImage              image;
        VkDeviceMemory       device_memory;
        VkImageView          image_view;
        VkSampler            sampler;
    } g_buffer;

    struct
    {
        VkImage              image;
        VkDeviceMemory       device_memory;
        VkImageView          image_view;
        VkSampler            sampler;
    } z_buffer;

    //struct
    //{
    //    VkImage              image;
    //    VkDeviceMemory       device_memory;
    //    VkImageView          image_view;
    //    VkSampler            sampler;
    //} normal_buffer;

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

    struct
    {
        VkBuffer             buffer;
        VkDeviceMemory       device_memory;
    } ubo;

    VkFence                  rendering_finished_fence;
    VkSemaphore              rendering_finished_semaphore;

    VkRenderPass             render_pass;
    VkFramebuffer            framebuffer;

    VkDescriptorPool         descriptor_pool;
    VkDescriptorSetLayout    descriptor_set_layout;
    VkDescriptorSet          descriptor_set;

    VkShaderModule           vertex_shader;
    VkShaderModule           fragment_shader;
    VkPipelineLayout         pipeline_layout;
    VkPipeline               pipeline;

    VkCommandBuffer          command_buffer;
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

typedef struct tg_renderer_3d_present_pass_vertex
{
    tgm_vec2f    position;
    tgm_vec2f    uv;
} tg_renderer_3d_present_pass_vertex;

typedef struct tg_renderer_3d_geometry_pass_vertex
{
    tgm_vec3f    position;
    tgm_vec3f    normal;
    tgm_vec2f    uv;
} tg_renderer_3d_geometry_pass_vertex;



tg_renderer_3d_geometry_pass    geometry_pass;
tg_renderer_3d_shading_pass     shading_pass;
tg_renderer_3d_present_pass     present_pass;
tg_renderer_3d_clear_pass       clear_pass;



void tg_graphics_renderer_3d_internal_init_geometry_pass()
{
    VkImageCreateInfo g_buffer_attachment_create_info = { 0 };
    {
        g_buffer_attachment_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        g_buffer_attachment_create_info.pNext = NULL;
        g_buffer_attachment_create_info.flags = 0;
        g_buffer_attachment_create_info.imageType = VK_IMAGE_TYPE_2D;
        g_buffer_attachment_create_info.format = surface.format.format;
        g_buffer_attachment_create_info.extent.width = swapchain_extent.width;
        g_buffer_attachment_create_info.extent.height = swapchain_extent.height;
        g_buffer_attachment_create_info.extent.depth = 1;
        g_buffer_attachment_create_info.mipLevels = 1;
        g_buffer_attachment_create_info.arrayLayers = 1;
        g_buffer_attachment_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        g_buffer_attachment_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        g_buffer_attachment_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        g_buffer_attachment_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        g_buffer_attachment_create_info.queueFamilyIndexCount = 0;
        g_buffer_attachment_create_info.pQueueFamilyIndices = NULL;
        g_buffer_attachment_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VK_CALL(vkCreateImage(device, &g_buffer_attachment_create_info, NULL, &geometry_pass.g_buffer.image));
    tg_graphics_vulkan_image_allocate_memory(geometry_pass.g_buffer.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.g_buffer.device_memory);
    tg_graphics_vulkan_image_view_create(geometry_pass.g_buffer.image, surface.format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &geometry_pass.g_buffer.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &geometry_pass.g_buffer.sampler);

    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    tg_graphics_vulkan_depth_format_acquire(&depth_format);
    VkImageCreateInfo z_buffer_attachment_create_info = { 0 };
    {
        z_buffer_attachment_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        z_buffer_attachment_create_info.pNext = NULL;
        z_buffer_attachment_create_info.flags = 0;
        z_buffer_attachment_create_info.imageType = VK_IMAGE_TYPE_2D;
        z_buffer_attachment_create_info.format = depth_format;
        z_buffer_attachment_create_info.extent.width = swapchain_extent.width;
        z_buffer_attachment_create_info.extent.height = swapchain_extent.height;
        z_buffer_attachment_create_info.extent.depth = 1;
        z_buffer_attachment_create_info.mipLevels = 1;
        z_buffer_attachment_create_info.arrayLayers = 1;
        z_buffer_attachment_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        z_buffer_attachment_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        z_buffer_attachment_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        z_buffer_attachment_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        z_buffer_attachment_create_info.queueFamilyIndexCount = 0;
        z_buffer_attachment_create_info.pQueueFamilyIndices = NULL;
        z_buffer_attachment_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VK_CALL(vkCreateImage(device, &z_buffer_attachment_create_info, NULL, &geometry_pass.z_buffer.image));
    tg_graphics_vulkan_image_allocate_memory(geometry_pass.z_buffer.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.z_buffer.device_memory);
    tg_graphics_vulkan_image_view_create(geometry_pass.z_buffer.image, depth_format, 1, VK_IMAGE_ASPECT_DEPTH_BIT, &geometry_pass.z_buffer.image_view);
    tg_graphics_vulkan_sampler_create(1, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &geometry_pass.z_buffer.sampler);

    // TODO: normal-buffer

    VkCommandBuffer barrier_command_buffer;
    VkCommandBufferAllocateInfo barrier_command_buffer_allocate_info = { 0 };
    {
        barrier_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        barrier_command_buffer_allocate_info.pNext = NULL;
        barrier_command_buffer_allocate_info.commandPool = command_pool;
        barrier_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        barrier_command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &barrier_command_buffer_allocate_info, &barrier_command_buffer));

    VkCommandBufferBeginInfo barrier_command_buffer_begin_info = { 0 };
    {
        barrier_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        barrier_command_buffer_begin_info.pNext = NULL;
        barrier_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        barrier_command_buffer_begin_info.pInheritanceInfo = NULL;
    }
    VK_CALL(vkBeginCommandBuffer(barrier_command_buffer, &barrier_command_buffer_begin_info));

    VkImageMemoryBarrier g_buffer_attachment_memory_barrier = { 0 };
    {
        g_buffer_attachment_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        g_buffer_attachment_memory_barrier.pNext = NULL;
        g_buffer_attachment_memory_barrier.srcAccessMask = 0;
        g_buffer_attachment_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        g_buffer_attachment_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        g_buffer_attachment_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        g_buffer_attachment_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        g_buffer_attachment_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        g_buffer_attachment_memory_barrier.image = geometry_pass.g_buffer.image;
        g_buffer_attachment_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        g_buffer_attachment_memory_barrier.subresourceRange.baseMipLevel = 0;
        g_buffer_attachment_memory_barrier.subresourceRange.levelCount = 1;
        g_buffer_attachment_memory_barrier.subresourceRange.baseArrayLayer = 0;
        g_buffer_attachment_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(barrier_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &g_buffer_attachment_memory_barrier);

    VkImageMemoryBarrier z_buffer_image_memory_barrier = { 0 };
    {
        z_buffer_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        z_buffer_image_memory_barrier.pNext = NULL;
        z_buffer_image_memory_barrier.srcAccessMask = 0;
        z_buffer_image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        z_buffer_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        z_buffer_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        z_buffer_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        z_buffer_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        z_buffer_image_memory_barrier.image = geometry_pass.z_buffer.image;
        z_buffer_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        z_buffer_image_memory_barrier.subresourceRange.baseMipLevel = 0;
        z_buffer_image_memory_barrier.subresourceRange.levelCount = 1;
        z_buffer_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        z_buffer_image_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(barrier_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &z_buffer_image_memory_barrier);
    VK_CALL(vkEndCommandBuffer(barrier_command_buffer));

    // TODO: normal-buffer

    VkSubmitInfo barrier_submit_info = { 0 };
    {
        barrier_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        barrier_submit_info.pNext = NULL;
        barrier_submit_info.waitSemaphoreCount = 0;
        barrier_submit_info.pWaitSemaphores = NULL;
        barrier_submit_info.pWaitDstStageMask = NULL;
        barrier_submit_info.commandBufferCount = 1;
        barrier_submit_info.pCommandBuffers = &barrier_command_buffer;
        barrier_submit_info.signalSemaphoreCount = 0;
        barrier_submit_info.pSignalSemaphores = NULL;
    }
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &barrier_submit_info, VK_NULL_HANDLE));
    VK_CALL(vkQueueWaitIdle(graphics_queue.queue));
    vkFreeCommandBuffers(device, command_pool, 1, &barrier_command_buffer);

    // --------------------------------------------------------------
    // TODO: not here!
    tg_graphics_vulkan_buffer_create(sizeof(tg_uniform_buffer_object), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &geometry_pass.ubo.buffer, &geometry_pass.ubo.device_memory);

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    void* data;
    tg_renderer_3d_geometry_pass_vertex vertices[4] = { 0 };
    {
        vertices[0].position.x = -1.5f;
        vertices[0].position.y = -1.5f;
        vertices[0].position.z =  0.0f;
        vertices[0].normal.x   =  0.0f;
        vertices[0].normal.y   =  0.0f;
        vertices[0].normal.z   =  1.0f;
        vertices[0].uv.x       =  0.0f;
        vertices[0].uv.y       =  0.0f;
        
        vertices[1].position.x =  1.5f;
        vertices[1].position.y = -1.5f;
        vertices[1].position.z =  0.0f;
        vertices[1].normal.x   =  0.0f;
        vertices[1].normal.y   =  0.0f;
        vertices[1].normal.z   =  1.0f;
        vertices[1].uv.x       =  1.0f;
        vertices[1].uv.y       =  0.0f;
        
        vertices[2].position.x =  1.5f;
        vertices[2].position.y =  1.5f;
        vertices[2].position.z =  0.0f;
        vertices[2].normal.x   =  0.0f;
        vertices[2].normal.y   =  0.0f;
        vertices[2].normal.z   =  1.0f;
        vertices[2].uv.x       =  1.0f;
        vertices[2].uv.y       =  1.0f;
        
        vertices[3].position.x = -1.5f;
        vertices[3].position.y =  1.5f;
        vertices[3].position.z =  0.0f;
        vertices[3].normal.x   =  0.0f;
        vertices[3].normal.y   =  0.0f;
        vertices[3].normal.z   =  1.0f;
        vertices[3].uv.x       =  0.0f;
        vertices[3].uv.y       =  1.0f;

    }
    tg_graphics_vulkan_buffer_create(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, sizeof(vertices), 0, &data));
    {
        memcpy(data, vertices, sizeof(vertices));
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.vbo.buffer, &geometry_pass.vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(vertices), &staging_buffer, &geometry_pass.vbo.buffer);
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
    tg_graphics_vulkan_buffer_create(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometry_pass.ibo.buffer, &geometry_pass.ibo.device_memory);
    tg_graphics_vulkan_buffer_copy(sizeof(indices), &staging_buffer, &geometry_pass.ibo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);
    // TODO: not here!
    // --------------------------------------------------------------

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
        attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_descriptions[1].flags = 0;
        attachment_descriptions[1].format = depth_format;
        attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
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
    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    {
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = NULL;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(*attachment_descriptions);
        render_pass_create_info.pAttachments = attachment_descriptions;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass_description;
        render_pass_create_info.dependencyCount = 1;
        render_pass_create_info.pDependencies = &subpass_dependency;
    }
    VK_CALL(vkCreateRenderPass(device, &render_pass_create_info, NULL, &geometry_pass.render_pass));

    const VkImageView framebuffer_attachments[] = {
        geometry_pass.g_buffer.image_view,
        geometry_pass.z_buffer.image_view
    };
    VkFramebufferCreateInfo framebuffer_create_info = { 0 };
    {
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = geometry_pass.render_pass;
        framebuffer_create_info.attachmentCount = sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments);
        framebuffer_create_info.pAttachments = framebuffer_attachments;
        framebuffer_create_info.width = swapchain_extent.width;
        framebuffer_create_info.height = swapchain_extent.height;
        framebuffer_create_info.layers = 1;
    }
    VK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &geometry_pass.framebuffer));

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_pool_size.descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = NULL;
        descriptor_pool_create_info.flags = 0;
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.poolSizeCount = 1;
        descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &geometry_pass.descriptor_pool));

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = 1;
        descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &geometry_pass.descriptor_set_layout));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = geometry_pass.descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &geometry_pass.descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &geometry_pass.descriptor_set));
    
    tg_graphics_vulkan_shader_module_create("shaders/geometry_vert.spv", &geometry_pass.vertex_shader);
    tg_graphics_vulkan_shader_module_create("shaders/geometry_frag.spv", &geometry_pass.fragment_shader);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &geometry_pass.descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = NULL;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &geometry_pass.pipeline_layout));

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[2] = { 0 };
    {
        pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[0].pNext = NULL;
        pipeline_shader_stage_create_infos[0].flags = 0;
        pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_shader_stage_create_infos[0].module = geometry_pass.vertex_shader;
        pipeline_shader_stage_create_infos[0].pName = "main";
        pipeline_shader_stage_create_infos[0].pSpecializationInfo = NULL;
        pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[1].pNext = NULL;
        pipeline_shader_stage_create_infos[1].flags = 0;
        pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_shader_stage_create_infos[1].module = geometry_pass.fragment_shader;
        pipeline_shader_stage_create_infos[1].pName = "main";
        pipeline_shader_stage_create_infos[1].pSpecializationInfo = NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_renderer_3d_geometry_pass_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3] = { 0 };
    {
        vertex_input_attribute_descriptions[0].binding = 0;
        vertex_input_attribute_descriptions[0].location = 0;
        vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[0].offset = offsetof(tg_renderer_3d_geometry_pass_vertex, position);
        vertex_input_attribute_descriptions[1].binding = 0;
        vertex_input_attribute_descriptions[1].location = 1;
        vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[1].offset = offsetof(tg_renderer_3d_geometry_pass_vertex, normal);
        vertex_input_attribute_descriptions[2].binding = 0;
        vertex_input_attribute_descriptions[2].location = 2;
        vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attribute_descriptions[2].offset = offsetof(tg_renderer_3d_geometry_pass_vertex, uv);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attribute_descriptions) / sizeof(*vertex_input_attribute_descriptions);
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
        viewport.width = (float)swapchain_extent.width;
        viewport.height = (float)swapchain_extent.height;
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
        pipeline_multisample_state_create_info.pNext = NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;//surface.msaa_sample_count;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;//VK_TRUE;
        pipeline_multisample_state_create_info.minSampleShading = 1.0f;
        pipeline_multisample_state_create_info.pSampleMask = NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = NULL;
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
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = { 0 };
    {
        pipeline_color_blend_attachment_state.blendEnable = VK_TRUE;
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
        graphics_pipeline_create_info.stageCount = sizeof(pipeline_shader_stage_create_infos) / sizeof(*pipeline_shader_stage_create_infos);
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
        graphics_pipeline_create_info.layout = geometry_pass.pipeline_layout;
        graphics_pipeline_create_info.renderPass = geometry_pass.render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &geometry_pass.pipeline));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &geometry_pass.command_buffer));

    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = geometry_pass.ubo.buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(tg_uniform_buffer_object);
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.pNext = NULL;
        write_descriptor_set.dstSet = geometry_pass.descriptor_set;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_set.pImageInfo = NULL;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
        write_descriptor_set.pTexelBufferView = NULL;
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
    VK_CALL(vkBeginCommandBuffer(geometry_pass.command_buffer, &command_buffer_begin_info));
    vkCmdBindPipeline(geometry_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass.pipeline);

    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(geometry_pass.command_buffer, 0, 1, &geometry_pass.vbo.buffer, &vertex_buffer_offset);
    vkCmdBindIndexBuffer(geometry_pass.command_buffer, geometry_pass.ibo.buffer, 0, VK_INDEX_TYPE_UINT16);

    const ui32 dynamic_offset = 0;
    vkCmdBindDescriptorSets(geometry_pass.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pass.pipeline_layout, 0, 1, &geometry_pass.descriptor_set, 1, &dynamic_offset);

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
        render_pass_begin_info.pNext = NULL;
        render_pass_begin_info.renderPass = geometry_pass.render_pass;
        render_pass_begin_info.framebuffer = geometry_pass.framebuffer;
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values; // TODO: NULL?
    }
    vkCmdBeginRenderPass(geometry_pass.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(geometry_pass.command_buffer, 6, 1, 0, 0, 0);
    vkCmdEndRenderPass(geometry_pass.command_buffer);
    VK_CALL(vkEndCommandBuffer(geometry_pass.command_buffer));
}
void tg_graphics_renderer_3d_internal_init_shading_pass()
{

}
void tg_graphics_renderer_3d_internal_init_present_pass()
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    void* data;

    tg_renderer_3d_present_pass_vertex vertices[4] = { 0 };
    {
        vertices[0].position.x = -1.0f;
        vertices[0].position.y =  1.0f;
        vertices[0].uv.x       =  0.0f;
        vertices[0].uv.y       =  0.0f;

        vertices[1].position.x =  1.0f;
        vertices[1].position.y =  1.0f;
        vertices[1].uv.x       =  1.0f;
        vertices[1].uv.y       =  0.0f;

        vertices[2].position.x =  1.0f;
        vertices[2].position.y = -1.0f;
        vertices[2].uv.x       =  1.0f;
        vertices[2].uv.y       =  1.0f;

        vertices[3].position.x = -1.0f;
        vertices[3].position.y = -1.0f;
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
    tg_graphics_vulkan_buffer_copy(sizeof(vertices), &staging_buffer, &present_pass.vbo.buffer);
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
    tg_graphics_vulkan_buffer_copy(sizeof(indices), &staging_buffer, &present_pass.ibo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    VkSemaphoreCreateInfo image_acquired_semaphore_create_info = { 0 };
    {
        image_acquired_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        image_acquired_semaphore_create_info.pNext = NULL;
        image_acquired_semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &image_acquired_semaphore_create_info, NULL, &present_pass.image_acquired_semaphore));

    VkFenceCreateInfo fence_create_info = { 0 };
    {
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VK_CALL(vkCreateFence(device, &fence_create_info, NULL, &present_pass.rendering_finished_fence));

    VkSemaphoreCreateInfo rendering_finished_semaphore_create_info = { 0 };
    {
        rendering_finished_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        rendering_finished_semaphore_create_info.pNext = NULL;
        rendering_finished_semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &rendering_finished_semaphore_create_info, NULL, &present_pass.rendering_finished_semaphore));

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
        attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    {
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = NULL;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = 1;
        render_pass_create_info.pAttachments = &attachment_description;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass_description;
        render_pass_create_info.dependencyCount = 0;
        render_pass_create_info.pDependencies = NULL;
    }
    VK_CALL(vkCreateRenderPass(device, &render_pass_create_info, NULL, &present_pass.render_pass));

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        VkFramebufferCreateInfo framebuffer_create_info = { 0 };
        {
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = NULL;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = present_pass.render_pass;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments = &swapchain_image_views[i];
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = 1;
        }
        VK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &present_pass.framebuffers[i]));
    }

    VkDescriptorPoolSize descriptor_pool_size = { 0 };
    {
        descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_size.descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = NULL;
        descriptor_pool_create_info.flags = 0;
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.poolSizeCount = 1;
        descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &present_pass.descriptor_pool));

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = 1;
        descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &present_pass.descriptor_set_layout));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = present_pass.descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &present_pass.descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &present_pass.descriptor_set));

    tg_graphics_vulkan_shader_module_create("shaders/present_vert.spv", &present_pass.vertex_shader);
    tg_graphics_vulkan_shader_module_create("shaders/present_frag.spv", &present_pass.fragment_shader);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &present_pass.descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = NULL;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &present_pass.pipeline_layout));

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
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &present_pass.pipeline));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = SURFACE_IMAGE_COUNT;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, present_pass.command_buffers));

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
    VkImageMemoryBarrier preparation_memory_barrier = { 0 };
    {
        preparation_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preparation_memory_barrier.pNext = NULL;
        preparation_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        preparation_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        preparation_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        preparation_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        preparation_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        preparation_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        preparation_memory_barrier.image = geometry_pass.g_buffer.image;
        preparation_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        preparation_memory_barrier.subresourceRange.baseMipLevel = 0;
        preparation_memory_barrier.subresourceRange.levelCount = 1;
        preparation_memory_barrier.subresourceRange.baseArrayLayer = 0;
        preparation_memory_barrier.subresourceRange.layerCount = 1;
    }
    VkImageMemoryBarrier reversion_memory_barrier = { 0 };
    {
        reversion_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        reversion_memory_barrier.pNext = NULL;
        reversion_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        reversion_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        reversion_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        reversion_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        reversion_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        reversion_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        reversion_memory_barrier.image = geometry_pass.g_buffer.image;
        reversion_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        reversion_memory_barrier.subresourceRange.baseMipLevel = 0;
        reversion_memory_barrier.subresourceRange.levelCount = 1;
        reversion_memory_barrier.subresourceRange.baseArrayLayer = 0;
        reversion_memory_barrier.subresourceRange.layerCount = 1;
    }
    const VkDeviceSize vertex_buffer_offsets[1] = { 0 };
    VkClearValue clear_value = { 0 };
    {
        clear_value.color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 1.0f };
        clear_value.depthStencil = (VkClearDepthStencilValue){ 0.0f, 0 };
    }
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = geometry_pass.g_buffer.sampler;
        descriptor_image_info.imageView = geometry_pass.g_buffer.image_view;
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

        vkCmdPipelineBarrier(present_pass.command_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &preparation_memory_barrier);
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
            render_pass_begin_info.clearValueCount = 1;
            render_pass_begin_info.pClearValues = &clear_value;
        }
        vkCmdBeginRenderPass(present_pass.command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdDrawIndexed(present_pass.command_buffers[i], 6, 1, 0, 0, 0);
        vkCmdEndRenderPass(present_pass.command_buffers[i]);
        vkCmdPipelineBarrier(present_pass.command_buffers[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &reversion_memory_barrier);
        VK_CALL(vkEndCommandBuffer(present_pass.command_buffers[i]));
    }
}
void tg_graphics_renderer_3d_internal_init_clear_pass()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &clear_pass.command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
    VK_CALL(vkBeginCommandBuffer(clear_pass.command_buffer, &command_buffer_begin_info));

    const VkClearColorValue clear_color_value = { 1.0f, 0.0f, 1.0f, 1.0f };
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

    VkImageMemoryBarrier g_buffer_memory_barrier = { 0 };
    {
        g_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        g_buffer_memory_barrier.pNext = NULL;
        g_buffer_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        g_buffer_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        g_buffer_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        g_buffer_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        g_buffer_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        g_buffer_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        g_buffer_memory_barrier.image = geometry_pass.g_buffer.image;
        g_buffer_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        g_buffer_memory_barrier.subresourceRange.baseMipLevel = 0;
        g_buffer_memory_barrier.subresourceRange.levelCount = 1;
        g_buffer_memory_barrier.subresourceRange.baseArrayLayer = 0;
        g_buffer_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(clear_pass.command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &g_buffer_memory_barrier);
    vkCmdClearColorImage(clear_pass.command_buffer, geometry_pass.g_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &g_buffer_image_subresource_range);
    g_buffer_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    g_buffer_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    g_buffer_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    g_buffer_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    vkCmdPipelineBarrier(clear_pass.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &g_buffer_memory_barrier);

    VkImageMemoryBarrier z_attachment_memory_barrier = { 0 };
    {
        z_attachment_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        z_attachment_memory_barrier.pNext = NULL;
        z_attachment_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        z_attachment_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        z_attachment_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        z_attachment_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        z_attachment_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        z_attachment_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        z_attachment_memory_barrier.image = geometry_pass.z_buffer.image;
        z_attachment_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        z_attachment_memory_barrier.subresourceRange.baseMipLevel = 0;
        z_attachment_memory_barrier.subresourceRange.levelCount = 1;
        z_attachment_memory_barrier.subresourceRange.baseArrayLayer = 0;
        z_attachment_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(clear_pass.command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &z_attachment_memory_barrier);
    vkCmdClearDepthStencilImage(clear_pass.command_buffer, geometry_pass.z_buffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &z_buffer_image_subresource_range);
    z_attachment_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    z_attachment_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    z_attachment_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    z_attachment_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    vkCmdPipelineBarrier(clear_pass.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &z_attachment_memory_barrier);

    // TODO: normal-buffer

    VK_CALL(vkEndCommandBuffer(clear_pass.command_buffer));
}

void tg_graphics_renderer_3d_init()
{
    tg_graphics_renderer_3d_internal_init_geometry_pass();
    tg_graphics_renderer_3d_internal_init_shading_pass();
    tg_graphics_renderer_3d_internal_init_present_pass();
    tg_graphics_renderer_3d_internal_init_clear_pass();
}
void tg_graphics_renderer_3d_draw(const tg_mesh_h mesh_h)
{
    // geometry pass

    tg_uniform_buffer_object* p_uniform_buffer_object = NULL;
    VK_CALL(vkMapMemory(device, geometry_pass.ubo.device_memory, 0, sizeof(*p_uniform_buffer_object), 0, &p_uniform_buffer_object));
    {
        tgm_vec3f from = { -1.0f, 1.0f, 1.0f };
        tgm_vec3f to = { 0.0f, 0.0f, -2.0f };
        tgm_vec3f up = { 0.0f, 1.0f, 0.0f };
        const tgm_vec3f translation_vector = { 0.0f, 0.0f, -2.0f };
        const f32 fov_y = TGM_TO_DEGREES(70.0f);
        const f32 aspect = (f32)swapchain_extent.width / (f32)swapchain_extent.height;
        const f32 n = -0.1f;
        const f32 f = -1000.0f;
        tgm_m4f_translate(&p_uniform_buffer_object->model, &translation_vector);
        tgm_m4f_look_at(&p_uniform_buffer_object->view, &from, &to, &up);
        tgm_m4f_perspective(&p_uniform_buffer_object->projection, fov_y, aspect, n, f);
    }
    vkUnmapMemory(device, geometry_pass.ubo.device_memory);

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &geometry_pass.command_buffer;
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

#endif
