#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/math/tg_math.h"
#include "tg/platform/tg_allocator.h"



#define RENDERER_2D_MAX_QUAD_COUNT   100000
#define RENDERER_2D_MAX_VERTEX_COUNT (4 * RENDERER_2D_MAX_QUAD_COUNT)
#define RENDERER_2D_MAX_VBO_SIZE     (RENDERER_2D_MAX_QUAD_COUNT * sizeof(tg_vertex))
#define RENDERER_2D_MAX_INDEX_COUNT  (6 * RENDERER_2D_MAX_QUAD_COUNT)
#define RENDERER_2D_MAX_IBO_SIZE     (RENDERER_2D_MAX_INDEX_COUNT * sizeof(ui16))



tg_image_h                           image_h; // TODO: <- NO!
VkBuffer                             renderer_2d_vbo;
VkDeviceMemory                       renderer_2d_vbo_memory;
VkBuffer                             renderer_2d_ibo;
VkDeviceMemory                       renderer_2d_ibo_memory;
VkBuffer                             renderer_2d_ubo;
VkDeviceMemory                       renderer_2d_ubo_memory;


VkFence                              renderer_2d_rendering_finished_fence;
VkSemaphore                          renderer_2d_rendering_finished_semaphore;
VkSemaphore                          renderer_2d_image_acquired_semaphore;

VkRenderPass                         renderer_2d_render_pass;
VkImage                              renderer_2d_color_image;
VkDeviceMemory                       renderer_2d_color_image_memory;
VkImageView                          renderer_2d_color_image_view;
VkImage                              renderer_2d_depth_image;
VkDeviceMemory                       renderer_2d_depth_image_memory;
VkImageView                          renderer_2d_depth_image_view;
VkFramebuffer                        renderer_2d_framebuffers[SURFACE_IMAGE_COUNT];

VkDescriptorPool                     renderer_2d_descriptor_pool;
VkDescriptorSetLayout                renderer_2d_descriptor_set_layout;
VkDescriptorSet                      renderer_2d_descriptor_set;

VkShaderModule                       renderer_2d_vertex_shader;
VkShaderModule                       renderer_2d_fragment_shader;
VkPipelineLayout                     renderer_2d_pipeline_layout;
VkPipeline                           renderer_2d_pipeline;

VkCommandBuffer                      renderer_2d_command_buffer;


tg_vertex*                           renderer_2d_mapped_vbo_memory = NULL;
ui32                                 renderer_2d_current_quad_index = 0;



void tg_graphics_renderer_2d_internal_init_index_buffer()
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tg_graphics_vulkan_buffer_create(RENDERER_2D_MAX_VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

    ui16* indices = tg_allocator_allocate(RENDERER_2D_MAX_IBO_SIZE);
    for (ui32 i = 0; i < RENDERER_2D_MAX_QUAD_COUNT; i++)
    {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 2;
        indices[6 * i + 4] = 4 * i + 3;
        indices[6 * i + 5] = 4 * i + 0;
    }

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, RENDERER_2D_MAX_IBO_SIZE, 0, &data);
    memcpy(data, indices, RENDERER_2D_MAX_IBO_SIZE);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_graphics_vulkan_buffer_create(RENDERER_2D_MAX_VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_2d_ibo, &renderer_2d_ibo_memory);
    tg_graphics_vulkan_buffer_copy(RENDERER_2D_MAX_VBO_SIZE, &staging_buffer, &renderer_2d_ibo);

    vkFreeMemory(device, staging_buffer_memory, NULL);
    vkDestroyBuffer(device, staging_buffer, NULL);
    tg_allocator_free(indices);
}
void tg_graphics_renderer_2d_internal_init_synchronization_primitives()
{
    VkFenceCreateInfo fence_create_info = { 0 };
    {
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VK_CALL(vkCreateFence(device, &fence_create_info, NULL, &renderer_2d_rendering_finished_fence));

    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    {
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = NULL;
        semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &renderer_2d_rendering_finished_semaphore));
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &renderer_2d_image_acquired_semaphore));
}
void tg_graphics_renderer_2d_internal_init_render_pass()
{
    VkAttachmentReference color_attachment_reference = { 0 };
    {
        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference depth_attachment_reference = { 0 };
    {
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference color_resolve_attachment_reference = { 0 };
    {
        color_resolve_attachment_reference.attachment = 2;
        color_resolve_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass_description = { 0 };
    {
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = NULL;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_attachment_reference;
        subpass_description.pResolveAttachments = &color_resolve_attachment_reference;
        subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = NULL;
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
    VkFormat depth_format;
    tg_graphics_vulkan_depth_format_acquire(&depth_format);
    VkAttachmentDescription attachment_descriptions[3] = { 0 };
    {
        attachment_descriptions[0].flags = 0;
        attachment_descriptions[0].format = surface.format.format;
        attachment_descriptions[0].samples = surface.msaa_sample_count;
        attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_descriptions[1].flags = 0;
        attachment_descriptions[1].format = depth_format;
        attachment_descriptions[1].samples = surface.msaa_sample_count;
        attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment_descriptions[2].flags = 0;
        attachment_descriptions[2].format = surface.format.format;
        attachment_descriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachment_descriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_descriptions[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
    VK_CALL(vkCreateRenderPass(device, &render_pass_create_info, NULL, &renderer_2d_render_pass));

    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, surface.format.format, surface.msaa_sample_count, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_2d_color_image, &renderer_2d_color_image_memory);
    tg_graphics_vulkan_image_view_create(renderer_2d_color_image, surface.format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &renderer_2d_color_image_view);

    tg_graphics_vulkan_image_create(swapchain_extent.width, swapchain_extent.height, 1, depth_format, surface.msaa_sample_count, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer_2d_depth_image, &renderer_2d_depth_image_memory);
    tg_graphics_vulkan_image_view_create(renderer_2d_depth_image, depth_format, 1, VK_IMAGE_ASPECT_DEPTH_BIT, &renderer_2d_depth_image_view);

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        const VkImageView image_view_attachments[] = {
            renderer_2d_color_image_view,
            renderer_2d_depth_image_view,
            swapchain_image_views[i]
        };
        VkFramebufferCreateInfo framebuffer_create_info = { 0 };
        {
            framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext = NULL;
            framebuffer_create_info.flags = 0;
            framebuffer_create_info.renderPass = renderer_2d_render_pass;
            framebuffer_create_info.attachmentCount = sizeof(image_view_attachments) / sizeof(*image_view_attachments);
            framebuffer_create_info.pAttachments = image_view_attachments;
            framebuffer_create_info.width = swapchain_extent.width;
            framebuffer_create_info.height = swapchain_extent.height;
            framebuffer_create_info.layers = 1;
        }
        VK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &renderer_2d_framebuffers[i]));
    }
}
void tg_graphics_renderer_2d_internal_init_descriptors()
{
    VkDescriptorPoolSize descriptor_pool_sizes[2] = { 0 };
    {
        descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_pool_sizes[0].descriptorCount = 1;
        descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_sizes[1].descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = NULL;
        descriptor_pool_create_info.flags = 0;
        descriptor_pool_create_info.maxSets = 1;
        descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_sizes) / sizeof(*descriptor_pool_sizes);
        descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &renderer_2d_descriptor_pool));



    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[2] = { 0 };
    {
        descriptor_set_layout_bindings[0].binding = 0;
        descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_layout_bindings[0].descriptorCount = 1;
        descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_layout_bindings[0].pImmutableSamplers = NULL;
        descriptor_set_layout_bindings[1].binding = 1;
        descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_set_layout_bindings[1].descriptorCount = 1;
        descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptor_set_layout_bindings[1].pImmutableSamplers = NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = sizeof(descriptor_set_layout_bindings) / sizeof(*descriptor_set_layout_bindings);
        descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &renderer_2d_descriptor_set_layout));



    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = NULL;
        descriptor_set_allocate_info.descriptorPool = renderer_2d_descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &renderer_2d_descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &renderer_2d_descriptor_set));



    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = renderer_2d_ubo;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(tg_uniform_buffer_object);
    }
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = image_h->sampler;
        descriptor_image_info.imageView = image_h->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_sets[2] = { 0 };
    {
        write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[0].pNext = NULL;
        write_descriptor_sets[0].dstSet = renderer_2d_descriptor_set;
        write_descriptor_sets[0].dstBinding = 0;
        write_descriptor_sets[0].dstArrayElement = 0;
        write_descriptor_sets[0].descriptorCount = 1;
        write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write_descriptor_sets[0].pImageInfo = NULL;
        write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;
        write_descriptor_sets[0].pTexelBufferView = NULL;
        write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[1].pNext = NULL;
        write_descriptor_sets[1].dstSet = renderer_2d_descriptor_set;
        write_descriptor_sets[1].dstBinding = 1;
        write_descriptor_sets[1].dstArrayElement = 0;
        write_descriptor_sets[1].descriptorCount = 1;
        write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_sets[1].pImageInfo = &descriptor_image_info;
        write_descriptor_sets[1].pBufferInfo = NULL;
        write_descriptor_sets[1].pTexelBufferView = NULL;
    }
    vkUpdateDescriptorSets(device, sizeof(write_descriptor_sets) / sizeof(*write_descriptor_sets), write_descriptor_sets, 0, NULL);
}
void tg_graphics_renderer_2d_internal_init_pipeline()
{
    tg_graphics_vulkan_shader_module_create("shaders/vert.spv", &renderer_2d_vertex_shader);
    tg_graphics_vulkan_shader_module_create("shaders/frag.spv", &renderer_2d_fragment_shader);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &renderer_2d_descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = NULL;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &renderer_2d_pipeline_layout));

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[2] = { 0 };
    {
        pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[0].pNext = NULL;
        pipeline_shader_stage_create_infos[0].flags = 0;
        pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_shader_stage_create_infos[0].module = renderer_2d_vertex_shader;
        pipeline_shader_stage_create_infos[0].pName = "main";
        pipeline_shader_stage_create_infos[0].pSpecializationInfo = NULL;
        pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[1].pNext = NULL;
        pipeline_shader_stage_create_infos[1].flags = 0;
        pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_shader_stage_create_infos[1].module = renderer_2d_fragment_shader;
        pipeline_shader_stage_create_infos[1].pName = "main";
        pipeline_shader_stage_create_infos[1].pSpecializationInfo = NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {   
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3] = { 0 };
    {
        vertex_input_attribute_descriptions[0].binding = 0;
        vertex_input_attribute_descriptions[0].location = 0;
        vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[0].offset = offsetof(tg_vertex, position);
        vertex_input_attribute_descriptions[1].binding = 0;
        vertex_input_attribute_descriptions[1].location = 1;
        vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[1].offset = offsetof(tg_vertex, color);
        vertex_input_attribute_descriptions[2].binding = 0;
        vertex_input_attribute_descriptions[2].location = 2;
        vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attribute_descriptions[2].offset = offsetof(tg_vertex, uv);
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
    VkRect2D rect = { 0 };
    {
        rect.offset = (VkOffset2D){ 0, 0 };
        rect.extent = swapchain_extent;
    }
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    {
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.pNext = NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &rect;
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
        pipeline_multisample_state_create_info.rasterizationSamples = surface.msaa_sample_count;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_TRUE;
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
        graphics_pipeline_create_info.layout = renderer_2d_pipeline_layout;
        graphics_pipeline_create_info.renderPass = renderer_2d_render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &renderer_2d_pipeline));
}
void tg_graphics_renderer_2d_internal_init_command_buffer()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &renderer_2d_command_buffer));
}

void tg_graphics_renderer_2d_internal_shutdown_pipeline()
{
    vkDestroyPipeline(device, renderer_2d_pipeline, NULL);
    vkDestroyPipelineLayout(device, renderer_2d_pipeline_layout, NULL);

    vkDestroyShaderModule(device, renderer_2d_fragment_shader, NULL);
    vkDestroyShaderModule(device, renderer_2d_vertex_shader, NULL);
}
void tg_graphics_renderer_2d_internal_shutdown_descriptors()
{
    vkDestroyDescriptorSetLayout(device, renderer_2d_descriptor_set_layout, NULL);
    vkDestroyDescriptorPool(device, renderer_2d_descriptor_pool, NULL);
}
void tg_graphics_renderer_2d_internal_shutdown_render_pass()
{
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyFramebuffer(device, renderer_2d_framebuffers[i], NULL);
    }
    vkDestroyImageView(device, renderer_2d_depth_image_view, NULL);
    vkFreeMemory(device, renderer_2d_depth_image_memory, NULL);
    vkDestroyImage(device, renderer_2d_depth_image, NULL);
    vkDestroyImageView(device, renderer_2d_color_image_view, NULL);
    vkFreeMemory(device, renderer_2d_color_image_memory, NULL);
    vkDestroyImage(device, renderer_2d_color_image, NULL);
    vkDestroyRenderPass(device, renderer_2d_render_pass, NULL);
}
void tg_graphics_renderer_2d_internal_shutdown_synchronization_primitives()
{
    vkDestroySemaphore(device, renderer_2d_image_acquired_semaphore, NULL);
    vkDestroySemaphore(device, renderer_2d_rendering_finished_semaphore, NULL);
    vkDestroyFence(device, renderer_2d_rendering_finished_fence, NULL);
}
void tg_graphics_renderer_2d_internal_shutdown_index_buffer()
{
    vkFreeMemory(device, renderer_2d_ibo_memory, NULL);
    vkDestroyBuffer(device, renderer_2d_ibo, NULL);
}

void tg_graphics_renderer_2d_init()
{
    // TODO: assert is vulkan initialized?

    tg_graphics_image_create("test_icon.bmp", &image_h);
    tg_graphics_vulkan_buffer_create(sizeof(tg_uniform_buffer_object), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer_2d_ubo, &renderer_2d_ubo_memory);
    tg_graphics_vulkan_buffer_create(RENDERER_2D_MAX_VBO_SIZE, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &renderer_2d_vbo, &renderer_2d_vbo_memory);
    tg_graphics_renderer_2d_internal_init_index_buffer();

    tg_graphics_renderer_2d_internal_init_synchronization_primitives();
    tg_graphics_renderer_2d_internal_init_render_pass();
    tg_graphics_renderer_2d_internal_init_descriptors();
    tg_graphics_renderer_2d_internal_init_pipeline();
    tg_graphics_renderer_2d_internal_init_command_buffer();
}
void tg_graphics_renderer_2d_begin()
{
    renderer_2d_current_quad_index = 0;
    VK_CALL(vkMapMemory(device, renderer_2d_vbo_memory, 0, RENDERER_2D_MAX_VBO_SIZE, 0, &renderer_2d_mapped_vbo_memory));
}
void tg_graphics_renderer_2d_draw_sprite(f32 x, f32 y, f32 z, f32 w, f32 h, tg_image_h image)
{
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 0].position.x = x - w / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 0].position.y = y - h / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 0].position.z = z;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 0].uv.x = 0.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 0].uv.y = 0.0f;

    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 1].position.x = x + w / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 1].position.y = y - h / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 1].position.z = z;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 1].uv.x = 1.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 1].uv.y = 0.0f;

    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 2].position.x = x + w / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 2].position.y = y + h / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 2].position.z = z;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 2].uv.x = 1.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 2].uv.y = 1.0f;

    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 3].position.x = x - w / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 3].position.y = y + h / 2.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 3].position.z = z;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 3].uv.x = 0.0f;
    renderer_2d_mapped_vbo_memory[4 * renderer_2d_current_quad_index + 3].uv.y = 1.0f;

    renderer_2d_current_quad_index++;
}
void tg_graphics_renderer_2d_end()
{
    vkUnmapMemory(device, renderer_2d_vbo_memory);

    ui32 current_image;
    VK_CALL(vkWaitForFences(device, 1, &renderer_2d_rendering_finished_fence, VK_TRUE, UINT64_MAX));
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, renderer_2d_image_acquired_semaphore, VK_NULL_HANDLE, &current_image));

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;
    }
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
        render_pass_begin_info.renderPass = renderer_2d_render_pass;
        render_pass_begin_info.framebuffer = renderer_2d_framebuffers[current_image];
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values;
    }
    const VkDeviceSize vertex_buffer_offsets[1] = { 0 };
    const ui32 dynamic_offsets[1] = { 0 };

    VK_CALL(vkBeginCommandBuffer(renderer_2d_command_buffer, &command_buffer_begin_info));
    vkCmdBindPipeline(renderer_2d_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_2d_pipeline);
    vkCmdBindVertexBuffers(renderer_2d_command_buffer, 0, sizeof(vertex_buffer_offsets) / sizeof(*vertex_buffer_offsets), &renderer_2d_vbo, vertex_buffer_offsets);
    vkCmdBindIndexBuffer(renderer_2d_command_buffer, renderer_2d_ibo, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(renderer_2d_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer_2d_pipeline_layout, 0, 1, &renderer_2d_descriptor_set, 1, dynamic_offsets);
    vkCmdBeginRenderPass(renderer_2d_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(renderer_2d_command_buffer, 6 * renderer_2d_current_quad_index, 1, 0, 0, 0);
    vkCmdEndRenderPass(renderer_2d_command_buffer);
    VK_CALL(vkEndCommandBuffer(renderer_2d_command_buffer));





    tgm_vec3f from = { -1.0f, 1.0f, 1.0f };
    tgm_vec3f to = { 0.0f, 0.0f, -2.0f };
    tgm_vec3f up = { 0.0f, 1.0f, 0.0f };
    const tgm_vec3f translation_vector = { 0.0f, 0.0f, -2.0f };
    const f32 fov_y = TGM_TO_DEGREES(70.0f);
    const f32 aspect = (f32)swapchain_extent.width / (f32)swapchain_extent.height;
    const f32 n = -0.1f;
    const f32 f = -1000.0f;
    tg_uniform_buffer_object uniform_buffer_object = { 0 };
    tgm_m4f_translate(&uniform_buffer_object.model, &translation_vector);
    tgm_m4f_look_at(&uniform_buffer_object.view, &from, &to, &up);
    tgm_m4f_perspective(&uniform_buffer_object.projection, fov_y, aspect, n, f);

    void* ubo_data;
    VK_CALL(vkMapMemory(device, renderer_2d_ubo_memory, 0, sizeof(tg_uniform_buffer_object), 0, &ubo_data));
    memcpy(ubo_data, &uniform_buffer_object, sizeof(tg_uniform_buffer_object));
    vkUnmapMemory(device, renderer_2d_ubo_memory);





    const VkPipelineStageFlags pipeline_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &renderer_2d_image_acquired_semaphore;
        submit_info.pWaitDstStageMask = pipeline_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &renderer_2d_command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &renderer_2d_rendering_finished_semaphore;
    }
    VK_CALL(vkResetFences(device, 1, &renderer_2d_rendering_finished_fence));
    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, renderer_2d_rendering_finished_fence));

    VkPresentInfoKHR present_info = { 0 };
    {
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &renderer_2d_rendering_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &current_image;
        present_info.pResults = NULL;
    }
    VK_CALL(vkQueuePresentKHR(present_queue.queue, &present_info));
}
void tg_graphics_renderer_2d_shutdown()
{
    vkDeviceWaitIdle(device);

    tg_graphics_image_destroy(image_h);

    tg_graphics_renderer_2d_internal_shutdown_pipeline();
    tg_graphics_renderer_2d_internal_shutdown_descriptors();
    tg_graphics_renderer_2d_internal_shutdown_render_pass();
    tg_graphics_renderer_2d_internal_shutdown_synchronization_primitives();

    vkFreeMemory(device, renderer_2d_ibo_memory, NULL); // TODO: all of these need functions
    vkDestroyBuffer(device, renderer_2d_ibo, NULL);
    vkFreeMemory(device, renderer_2d_vbo_memory, NULL);
    vkDestroyBuffer(device, renderer_2d_vbo, NULL);
    vkFreeMemory(device, renderer_2d_ubo_memory, NULL);
    vkDestroyBuffer(device, renderer_2d_ubo, NULL);
}
void tg_graphics_renderer_2d_on_window_resize(ui32 w, ui32 h)
{
    tg_graphics_renderer_2d_internal_shutdown_pipeline();
    tg_graphics_renderer_2d_internal_shutdown_render_pass();

    tg_graphics_renderer_2d_internal_init_render_pass();
    tg_graphics_renderer_2d_internal_init_pipeline();
}

#endif
