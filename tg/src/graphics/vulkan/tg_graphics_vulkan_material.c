#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_graphics_vulkan_shader.h"
#include "memory/tg_memory_allocator.h"

tg_material_h tg_graphics_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h)
{
	TG_ASSERT(vertex_shader_h && fragment_shader_h);

	tg_material_h material_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*material_h));
    tg_graphics_vulkan_buffer_create(sizeof(tg_uniform_buffer_object), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &material_h->ubo.buffer, &material_h->ubo.device_memory);
    material_h->vertex_shader = vertex_shader_h;
    material_h->fragment_shader = fragment_shader_h;

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
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, TG_NULL, &material_h->descriptor_pool));

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = { 0 };
    {
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;
    }
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = TG_NULL;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = 1;
        descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, TG_NULL, &material_h->descriptor_set_layout));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = TG_NULL;
        descriptor_set_allocate_info.descriptorPool = material_h->descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &material_h->descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &material_h->descriptor_set));

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = TG_NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = 1; // TODO: ask mesh for additional descriptor sets
        pipeline_layout_create_info.pSetLayouts = &material_h->descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = TG_NULL;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, TG_NULL, &material_h->pipeline_layout));

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[2] = { 0 };
    {
        pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
        pipeline_shader_stage_create_infos[0].flags = 0;
        pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_shader_stage_create_infos[0].module = tg_graphics_vulkan_vertex_shader_get_shader_module(material_h->vertex_shader);
        pipeline_shader_stage_create_infos[0].pName = "main";
        pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

        pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
        pipeline_shader_stage_create_infos[1].flags = 0;
        pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_shader_stage_create_infos[1].module = tg_graphics_vulkan_fragment_shader_get_shader_module(material_h->fragment_shader);
        pipeline_shader_stage_create_infos[1].pName = "main";
        pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
    }
    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    {
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(tg_vertex);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[5] = { 0 };
    {
        vertex_input_attribute_descriptions[0].binding = 0;
        vertex_input_attribute_descriptions[0].location = 0;
        vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[0].offset = offsetof(tg_vertex, position);

        vertex_input_attribute_descriptions[1].binding = 0;
        vertex_input_attribute_descriptions[1].location = 1;
        vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[1].offset = offsetof(tg_vertex, normal);
        
        vertex_input_attribute_descriptions[2].binding = 0;
        vertex_input_attribute_descriptions[2].location = 2;
        vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attribute_descriptions[2].offset = offsetof(tg_vertex, uv);
        
        vertex_input_attribute_descriptions[3].binding = 0;
        vertex_input_attribute_descriptions[3].location = 3;
        vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[3].offset = offsetof(tg_vertex, tangent);
        
        vertex_input_attribute_descriptions[4].binding = 0;
        vertex_input_attribute_descriptions[4].location = 4;
        vertex_input_attribute_descriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_input_attribute_descriptions[4].offset = offsetof(tg_vertex, bitangent);
    }
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    {
        pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipeline_vertex_input_state_create_info.pNext = TG_NULL;
        pipeline_vertex_input_state_create_info.flags = 0;
        pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
        pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attribute_descriptions) / sizeof(*vertex_input_attribute_descriptions);
        pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;
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
        pipeline_multisample_state_create_info.rasterizationSamples = surface.msaa_sample_count;
        pipeline_multisample_state_create_info.sampleShadingEnable = VK_TRUE;
        pipeline_multisample_state_create_info.minSampleShading = 1.0f;
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
    VkPipelineColorBlendAttachmentState p_pipeline_color_blend_attachment_states[2] = { 0 };
    {
        p_pipeline_color_blend_attachment_states[0].blendEnable = VK_TRUE;
        p_pipeline_color_blend_attachment_states[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[0].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[0].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        p_pipeline_color_blend_attachment_states[1].blendEnable = VK_TRUE;
        p_pipeline_color_blend_attachment_states[1].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[1].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[1].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[1].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    {
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipeline_color_blend_state_create_info.pNext = TG_NULL;
        pipeline_color_blend_state_create_info.flags = 0;
        pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        pipeline_color_blend_state_create_info.attachmentCount = 2;
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
        graphics_pipeline_create_info.stageCount = sizeof(pipeline_shader_stage_create_infos) / sizeof(*pipeline_shader_stage_create_infos);
        graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = TG_NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = TG_NULL;
        graphics_pipeline_create_info.layout = material_h->pipeline_layout;
        tg_graphics_vulkan_renderer_3d_get_geometry_render_pass(&graphics_pipeline_create_info.renderPass);
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, TG_NULL, &material_h->pipeline));

    return material_h;
}

void tg_graphics_material_destroy(tg_material_h material_h)
{
    TG_ASSERT(material_h);

    vkDestroyDescriptorPool(device, material_h->descriptor_pool, TG_NULL);
    vkDestroyDescriptorSetLayout(device, material_h->descriptor_set_layout, TG_NULL);
    vkDestroyPipelineLayout(device, material_h->pipeline_layout, TG_NULL);
    vkDestroyPipeline(device, material_h->pipeline, TG_NULL);
	TG_MEMORY_ALLOCATOR_FREE(material_h);
}

#endif
