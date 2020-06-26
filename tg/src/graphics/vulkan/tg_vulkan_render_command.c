#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_vulkan_forward_renderer.h"

tg_render_command_h tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material)
{
	TG_ASSERT(h_mesh && h_material);

	tg_render_command_h h_render_command = TG_MEMORY_ALLOC(sizeof(*h_render_command));
	h_render_command->type = TG_HANDLE_TYPE_RENDER_COMMAND;
	h_render_command->h_mesh = h_mesh;
	h_render_command->h_material = h_material;
	h_render_command->camera_info_count = 0;
	return h_render_command;
}

void tg_render_command_destroy(tg_render_command_h h_render_command)
{
	TG_ASSERT(h_render_command);

    for (u32 i = 0; i < h_render_command->camera_info_count; i++)
    {
        tg_vulkan_command_buffer_free(graphics_command_pool, h_render_command->p_camera_infos[i].command_buffer);
        tg_vulkan_graphics_pipeline_destroy(h_render_command->p_camera_infos[i].graphics_pipeline);
        tg_vulkan_pipeline_layout_destroy(h_render_command->p_camera_infos[i].pipeline_layout);
    }
	TG_MEMORY_FREE(h_render_command);
}

void tg_render_command_register(tg_render_command_h h_render_command, tg_camera_h h_camera)
{
	TG_ASSERT(h_render_command && h_camera);

    tg_vulkan_render_command_camera_info* p_camera_info = &h_render_command->p_camera_infos[h_render_command->camera_info_count++];

	p_camera_info->h_camera = h_camera;

    const b32 has_custom_descriptor_set = h_render_command->h_material->descriptor.descriptor_pool != VK_NULL_HANDLE;

    if (has_custom_descriptor_set)
    {
        const VkDescriptorSetLayout p_descriptor_set_layouts[2] = { h_camera->descriptor.descriptor_set_layout, h_render_command->h_material->descriptor.descriptor_set_layout };
        p_camera_info->pipeline_layout = tg_vulkan_pipeline_layout_create(2, p_descriptor_set_layouts, 0, TG_NULL);
    }
    else
    {
        p_camera_info->pipeline_layout = tg_vulkan_pipeline_layout_create(1, &h_camera->descriptor.descriptor_set_layout, 0, TG_NULL);
    }

    const tg_spirv_layout* p_layout = &h_render_command->h_material->h_vertex_shader->vulkan_shader.layout;

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = p_layout->vertex_stride;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    const u64 vertex_input_attribute_descriptions_size = (u64)p_layout->input_resource_count * sizeof(VkVertexInputAttributeDescription);
    VkVertexInputAttributeDescription* p_vertex_input_attribute_descriptions = TG_MEMORY_STACK_ALLOC(vertex_input_attribute_descriptions_size);
    for (u8 i = 0; i < p_layout->input_resource_count; i++)
    {
        p_vertex_input_attribute_descriptions[i].binding = 0;
        p_vertex_input_attribute_descriptions[i].location = p_layout->p_input_resources[i].location;
        p_vertex_input_attribute_descriptions[i].format = p_layout->p_input_resources[i].format;
        p_vertex_input_attribute_descriptions[i].offset = p_layout->p_input_resources[i].offset;
    }

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = (u32)p_layout->input_resource_count;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.vertex_shader = h_render_command->h_material->h_vertex_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.fragment_shader = h_render_command->h_material->h_fragment_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_TRUE;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        vulkan_graphics_pipeline_create_info.attachment_count = TG_DEFERRED_RENDERER_GEOMETRY_PASS_COLOR_ATTACHMENT_COUNT;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        vulkan_graphics_pipeline_create_info.attachment_count = 1;
        vulkan_graphics_pipeline_create_info.blend_enable = VK_TRUE;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    vulkan_graphics_pipeline_create_info.p_pipeline_vertex_input_state_create_info = &pipeline_vertex_input_state_create_info;
    vulkan_graphics_pipeline_create_info.pipeline_layout = p_camera_info->pipeline_layout;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED: vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass; break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:  vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;   break;
    default: TG_INVALID_CODEPATH(); break;
    }

    p_camera_info->graphics_pipeline = tg_vulkan_graphics_pipeline_create(&vulkan_graphics_pipeline_create_info);
    TG_MEMORY_STACK_FREE(vertex_input_attribute_descriptions_size);

    p_camera_info->command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.pNext = TG_NULL;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED: command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass; break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:  command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;   break;
    default: TG_INVALID_CODEPATH(); break;
    }

    command_buffer_inheritance_info.subpass = 0;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED: command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_deferred_renderer->geometry_pass.framebuffer; break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:  command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_forward_renderer->shading_pass.framebuffer;   break;

    default: TG_INVALID_CODEPATH(); break;
    }

    command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
    command_buffer_inheritance_info.queryFlags = 0;
    command_buffer_inheritance_info.pipelineStatistics = 0;

    tg_vulkan_command_buffer_begin(p_camera_info->command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &command_buffer_inheritance_info);
    {
        vkCmdBindPipeline(p_camera_info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->graphics_pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(p_camera_info->command_buffer, 0, 1, &h_render_command->h_mesh->vbo.buffer, &vertex_buffer_offset);
        if (h_render_command->h_mesh->ibo.size != 0)
        {
            vkCmdBindIndexBuffer(p_camera_info->command_buffer, h_render_command->h_mesh->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        }

        const u32 descriptor_set_count = has_custom_descriptor_set ? 2 : 1;
        VkDescriptorSet p_descriptor_sets[2] = { 0 };
        {
            p_descriptor_sets[0] = h_camera->descriptor.descriptor_set;
            if (has_custom_descriptor_set)
            {
                p_descriptor_sets[1] = h_render_command->h_material->descriptor.descriptor_set;
            }
        }
        vkCmdBindDescriptorSets(p_camera_info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->pipeline_layout, 0, descriptor_set_count, p_descriptor_sets, 0, TG_NULL);

        if (h_render_command->h_mesh->index_count != 0)
        {
            vkCmdDrawIndexed(p_camera_info->command_buffer, (u32)(h_render_command->h_mesh->index_count), 1, 0, 0, 0); // TODO: u16
        }
        else
        {
            vkCmdDraw(p_camera_info->command_buffer, (u32)(h_render_command->h_mesh->vertex_count), 1, 0, 0);
        }
    }
    VK_CALL(vkEndCommandBuffer(p_camera_info->command_buffer));
}

#endif
