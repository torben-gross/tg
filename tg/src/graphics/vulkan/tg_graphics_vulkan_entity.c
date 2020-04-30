#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "graphics/vulkan/tg_graphics_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_graphics_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"
#include "tg_scene.h"

tg_entity_graphics_data_ptr_h tg_entity_graphics_data_ptr_create(tg_entity* p_entity, tg_scene* p_scene)
{
	TG_ASSERT(p_entity);

	tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h = TG_MEMORY_ALLOC(sizeof(*entity_graphics_data_ptr_h));

    switch (p_entity->material_h->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        entity_graphics_data_ptr_h->renderer_h = p_scene->deferred_renderer_h;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        entity_graphics_data_ptr_h->renderer_h = p_scene->forward_renderer_h;
    } break;
    }

	entity_graphics_data_ptr_h->uniform_buffer = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	*((m4*)entity_graphics_data_ptr_h->uniform_buffer.p_mapped_device_memory) = tgm_m4_identity();

    const b32 has_custom_descriptor_set = p_entity->material_h->descriptor.descriptor_pool != VK_NULL_HANDLE;

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
    entity_graphics_data_ptr_h->descriptor = tg_vulkan_descriptor_create(2, p_descriptor_set_layout_bindings);

    if (has_custom_descriptor_set)
    {
        const VkDescriptorSetLayout p_descriptor_set_layouts[2] = { entity_graphics_data_ptr_h->descriptor.descriptor_set_layout, p_entity->material_h->descriptor.descriptor_set_layout };
        entity_graphics_data_ptr_h->pipeline_layout = tg_vulkan_pipeline_layout_create(2, p_descriptor_set_layouts, 0, TG_NULL);
    }
    else
    {
        entity_graphics_data_ptr_h->pipeline_layout = tg_vulkan_pipeline_layout_create(1, &entity_graphics_data_ptr_h->descriptor.descriptor_set_layout, 0, TG_NULL);
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
        vulkan_graphics_pipeline_create_info.vertex_shader = p_entity->material_h->vertex_shader_h->shader_module;
        vulkan_graphics_pipeline_create_info.fragment_shader = p_entity->material_h->fragment_shader_h->shader_module;
        vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
        vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;

        switch (p_entity->material_h->material_type)
        {
        case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
        {
            vulkan_graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
            vulkan_graphics_pipeline_create_info.depth_write_enable = VK_TRUE;
            vulkan_graphics_pipeline_create_info.attachment_count = TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
            vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        } break;
        case TG_VULKAN_MATERIAL_TYPE_FORWARD:
        {
            vulkan_graphics_pipeline_create_info.depth_test_enable = VK_FALSE;
            vulkan_graphics_pipeline_create_info.depth_write_enable = VK_FALSE;
            vulkan_graphics_pipeline_create_info.attachment_count = TG_FORWARD_RENDERER_COLOR_ATTACHMENT_COUNT;
            vulkan_graphics_pipeline_create_info.blend_enable = VK_TRUE;
        } break;
        default: TG_ASSERT(TG_FALSE);
        }

        vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
        vulkan_graphics_pipeline_create_info.pipeline_layout = entity_graphics_data_ptr_h->pipeline_layout;

        switch (p_entity->material_h->material_type)
        {
        case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
        {
            vulkan_graphics_pipeline_create_info.render_pass = p_scene->deferred_renderer_h->geometry_pass.render_pass;
        } break;
        case TG_VULKAN_MATERIAL_TYPE_FORWARD:
        {
            vulkan_graphics_pipeline_create_info.render_pass = p_scene->forward_renderer_h->shading_pass.render_pass;
        } break;
        default: TG_ASSERT(TG_FALSE);
        }
    }
    entity_graphics_data_ptr_h->graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);

    VkDescriptorBufferInfo model_descriptor_buffer_info = { 0 };
    {
        model_descriptor_buffer_info.buffer = entity_graphics_data_ptr_h->uniform_buffer.buffer;
        model_descriptor_buffer_info.offset = 0;
        model_descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkDescriptorBufferInfo view_projection_descriptor_buffer_info = { 0 };
    {
        switch (p_entity->material_h->material_type)
        {
        case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
        {
            view_projection_descriptor_buffer_info.buffer = p_scene->deferred_renderer_h->geometry_pass.view_projection_ubo.buffer;
        } break;
        case TG_VULKAN_MATERIAL_TYPE_FORWARD:
        {
            view_projection_descriptor_buffer_info.buffer = p_scene->forward_renderer_h->shading_pass.view_projection_ubo.buffer;
        } break;
        default: TG_ASSERT(TG_FALSE);
        }

        view_projection_descriptor_buffer_info.offset = 0;
        view_projection_descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };
    {
        p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[0].pNext = TG_NULL;
        p_write_descriptor_sets[0].dstSet = entity_graphics_data_ptr_h->descriptor.descriptor_set;
        p_write_descriptor_sets[0].dstBinding = 0;
        p_write_descriptor_sets[0].dstArrayElement = 0;
        p_write_descriptor_sets[0].descriptorCount = 1;
        p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_write_descriptor_sets[0].pImageInfo = TG_NULL;
        p_write_descriptor_sets[0].pBufferInfo = &model_descriptor_buffer_info;
        p_write_descriptor_sets[0].pTexelBufferView = TG_NULL;

        p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        p_write_descriptor_sets[1].pNext = TG_NULL;
        p_write_descriptor_sets[1].dstSet = entity_graphics_data_ptr_h->descriptor.descriptor_set;
        p_write_descriptor_sets[1].dstBinding = 1;
        p_write_descriptor_sets[1].dstArrayElement = 0;
        p_write_descriptor_sets[1].descriptorCount = 1;
        p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        p_write_descriptor_sets[1].pImageInfo = TG_NULL;
        p_write_descriptor_sets[1].pBufferInfo = &view_projection_descriptor_buffer_info;
        p_write_descriptor_sets[1].pTexelBufferView = TG_NULL;
    }
    tg_vulkan_descriptor_sets_update(2, p_write_descriptor_sets);

    entity_graphics_data_ptr_h->command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    {
        command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        command_buffer_inheritance_info.pNext = TG_NULL;

        switch (p_entity->material_h->material_type)
        {
        case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
        {
            command_buffer_inheritance_info.renderPass = p_scene->deferred_renderer_h->geometry_pass.render_pass;
        } break;
        case TG_VULKAN_MATERIAL_TYPE_FORWARD:
        {
            command_buffer_inheritance_info.renderPass = p_scene->forward_renderer_h->shading_pass.render_pass;
        } break;
        default: TG_ASSERT(TG_FALSE);
        }

        command_buffer_inheritance_info.subpass = 0;

        switch (p_entity->material_h->material_type)
        {
        case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
        {
            command_buffer_inheritance_info.framebuffer = p_scene->deferred_renderer_h->geometry_pass.framebuffer;
        } break;
        case TG_VULKAN_MATERIAL_TYPE_FORWARD:
        {
            command_buffer_inheritance_info.framebuffer = p_scene->forward_renderer_h->shading_pass.framebuffer;
        } break;
        default: TG_ASSERT(TG_FALSE);
        }

        command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
        command_buffer_inheritance_info.queryFlags = 0;
        command_buffer_inheritance_info.pipelineStatistics = 0;
    }
    tg_vulkan_command_buffer_begin(entity_graphics_data_ptr_h->command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &command_buffer_inheritance_info);
    {
        vkCmdBindPipeline(entity_graphics_data_ptr_h->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_graphics_data_ptr_h->graphics_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(entity_graphics_data_ptr_h->command_buffer, 0, 1, &p_entity->mesh_h->vbo.buffer, &vertex_buffer_offset);
        if (p_entity->mesh_h->ibo.size != 0)
        {
            vkCmdBindIndexBuffer(entity_graphics_data_ptr_h->command_buffer, p_entity->mesh_h->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        }

        const u32 descriptor_set_count = has_custom_descriptor_set ? 2 : 1;
        VkDescriptorSet p_descriptor_sets[2] = { 0 };
        {
            p_descriptor_sets[0] = entity_graphics_data_ptr_h->descriptor.descriptor_set;
            if (has_custom_descriptor_set)
            {
                p_descriptor_sets[1] = p_entity->material_h->descriptor.descriptor_set;
            }
        }
        vkCmdBindDescriptorSets(entity_graphics_data_ptr_h->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entity_graphics_data_ptr_h->pipeline_layout, 0, descriptor_set_count, p_descriptor_sets, 0, TG_NULL);

        if (p_entity->mesh_h->ibo.size != 0)
        {
            vkCmdDrawIndexed(entity_graphics_data_ptr_h->command_buffer, (u32)(p_entity->mesh_h->ibo.size / sizeof(u16)), 1, 0, 0, 0); // TODO: u16
        }
        else
        {
            vkCmdDraw(entity_graphics_data_ptr_h->command_buffer, (u32)(p_entity->mesh_h->vbo.size / sizeof(tg_vertex_3d)), 1, 0, 0);
        }
    }
    VK_CALL(vkEndCommandBuffer(entity_graphics_data_ptr_h->command_buffer));

	return entity_graphics_data_ptr_h;
}

void tg_entity_graphics_data_ptr_destroy(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h)
{
	TG_ASSERT(entity_graphics_data_ptr_h);

    tg_vulkan_command_buffer_free(graphics_command_pool, entity_graphics_data_ptr_h->command_buffer);
    tg_vulkan_graphics_pipeline_destroy(entity_graphics_data_ptr_h->graphics_pipeline);
    tg_vulkan_pipeline_layout_destroy(entity_graphics_data_ptr_h->pipeline_layout);
    tg_vulkan_descriptor_destroy(&entity_graphics_data_ptr_h->descriptor);
    tg_vulkan_buffer_destroy(&entity_graphics_data_ptr_h->uniform_buffer);

	TG_MEMORY_FREE(entity_graphics_data_ptr_h);
}

void tg_entity_graphics_data_ptr_set_model_matrix(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h, const m4* p_model_matrix)
{
    *((m4*)entity_graphics_data_ptr_h->uniform_buffer.p_mapped_device_memory) = *p_model_matrix;
}

#endif
