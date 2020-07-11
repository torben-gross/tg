#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_vulkan_forward_renderer.h"



static void tg__register(tg_render_command_h h_render_command, tg_camera_h h_camera, u32 global_resource_count, tg_handle* p_global_resources)
{
    TG_ASSERT(h_render_command && h_camera);

    tg_vulkan_render_command_camera_info* p_camera_info = &h_render_command->p_camera_infos[h_render_command->camera_info_count++];
    p_camera_info->h_camera = h_camera;

    tg_vulkan_graphics_pipeline_create_info vulkan_graphics_pipeline_create_info = { 0 };
    vulkan_graphics_pipeline_create_info.p_vertex_shader = &h_render_command->h_material->h_vertex_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.p_fragment_shader = &h_render_command->h_material->h_fragment_shader->vulkan_shader;
    vulkan_graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    vulkan_graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    vulkan_graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
    vulkan_graphics_pipeline_create_info.depth_write_enable = VK_TRUE;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass;
        vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_camera->capture_pass.h_deferred_renderer->geometry_pass.framebuffer.width;
        vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_camera->capture_pass.h_deferred_renderer->geometry_pass.framebuffer.height;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        vulkan_graphics_pipeline_create_info.blend_enable = VK_TRUE;
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;
        vulkan_graphics_pipeline_create_info.viewport_size.x = (f32)h_camera->capture_pass.h_forward_renderer->shading_pass.framebuffer.width;
        vulkan_graphics_pipeline_create_info.viewport_size.y = (f32)h_camera->capture_pass.h_forward_renderer->shading_pass.framebuffer.height;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    p_camera_info->graphics_pipeline = tg_vulkan_pipeline_create_graphics(&vulkan_graphics_pipeline_create_info);

    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.pNext = TG_NULL;
    command_buffer_inheritance_info.subpass = 0;
    command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
    command_buffer_inheritance_info.queryFlags = 0;
    command_buffer_inheritance_info.pipelineStatistics = 0;

    switch (h_render_command->h_material->material_type)
    {
    case TG_VULKAN_MATERIAL_TYPE_DEFERRED:
    {
        command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass;
        command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_deferred_renderer->geometry_pass.framebuffer.framebuffer;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;
        command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_forward_renderer->shading_pass.framebuffer.framebuffer;
    }break;
    default: TG_INVALID_CODEPATH(); break;
    }

    tg_vulkan_descriptor_set_update_uniform_buffer(p_camera_info->graphics_pipeline.descriptor_set, h_render_command->model_ubo.buffer, 0);
    tg_vulkan_descriptor_set_update_uniform_buffer(p_camera_info->graphics_pipeline.descriptor_set, p_camera_info->h_camera->view_projection_ubo.buffer, 1);
    for (u32 i = 0; i < global_resource_count; i++)
    {
        const u32 binding = i + TG_SHADER_RESERVED_BINDINGS;
        tg_vulkan_descriptor_set_update(p_camera_info->graphics_pipeline.descriptor_set, p_global_resources[i], binding);
    }

    p_camera_info->command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    tg_vulkan_command_buffer_begin(p_camera_info->command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &command_buffer_inheritance_info);
    {
        vkCmdBindPipeline(p_camera_info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->graphics_pipeline.pipeline);

        const VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(p_camera_info->command_buffer, 0, 1, &h_render_command->h_mesh->vbo.buffer, &vertex_buffer_offset);
        if (h_render_command->h_mesh->ibo.size != 0)
        {
            vkCmdBindIndexBuffer(p_camera_info->command_buffer, h_render_command->h_mesh->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
        }

        const u32 descriptor_set_count = p_camera_info->graphics_pipeline.descriptor_set == VK_NULL_HANDLE ? 0 : 1;
        vkCmdBindDescriptorSets(p_camera_info->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->graphics_pipeline.pipeline_layout, 0, descriptor_set_count, &p_camera_info->graphics_pipeline.descriptor_set, 0, TG_NULL);

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





    tg_vulkan_graphics_pipeline_create_info pipeline_create_info = { 0 };
    pipeline_create_info.p_vertex_shader = &tg_vertex_shader_get("shaders/shadow.vert")->vulkan_shader;
    pipeline_create_info.p_fragment_shader = &tg_fragment_shader_get("shaders/shadow.frag")->vulkan_shader;
    pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    pipeline_create_info.sample_count = 1;
    pipeline_create_info.depth_test_enable = TG_TRUE;
    pipeline_create_info.depth_write_enable = TG_TRUE;
    pipeline_create_info.blend_enable = TG_FALSE;
    pipeline_create_info.render_pass = h_camera->shadow_pass.render_pass;
    pipeline_create_info.viewport_size.x = (f32)TG_CASCADED_SHADOW_MAP_SIZE;
    pipeline_create_info.viewport_size.y = (f32)TG_CASCADED_SHADOW_MAP_SIZE;

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = h_render_command->h_material->h_vertex_shader->vulkan_shader.spirv_layout.vertex_stride;
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_description = { 0 };
    vertex_input_attribute_description.location = 0;
    vertex_input_attribute_description.binding = 0;
    vertex_input_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description.offset = 0;

    h_camera->shadow_pass.command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (u32 i = 0; i < TG_CASCADED_SHADOW_MAPS; i++)
    {
        p_camera_info->p_shadow_graphics_pipelines[i] = tg_vulkan_pipeline_create_graphics2(&pipeline_create_info, vertex_input_binding_description, 1, &vertex_input_attribute_description);
        tg_vulkan_descriptor_set_update_uniform_buffer(p_camera_info->p_shadow_graphics_pipelines[i].descriptor_set, h_render_command->model_ubo.buffer, 0);
        tg_vulkan_descriptor_set_update_uniform_buffer(p_camera_info->p_shadow_graphics_pipelines[i].descriptor_set, h_camera->shadow_pass.p_lightspace_ubos[i].buffer, 1);

        VkCommandBufferInheritanceInfo shadow_command_buffer_inheritance_info = { 0 };
        shadow_command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        shadow_command_buffer_inheritance_info.pNext = TG_NULL;
        shadow_command_buffer_inheritance_info.renderPass = h_camera->shadow_pass.render_pass;
        shadow_command_buffer_inheritance_info.subpass = 0;
        shadow_command_buffer_inheritance_info.framebuffer = h_camera->shadow_pass.p_framebuffers[i].framebuffer;
        shadow_command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
        shadow_command_buffer_inheritance_info.queryFlags = 0;
        shadow_command_buffer_inheritance_info.pipelineStatistics = 0;
        
        p_camera_info->p_shadow_command_buffers[i] = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        tg_vulkan_command_buffer_begin(p_camera_info->p_shadow_command_buffers[i], VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &shadow_command_buffer_inheritance_info);
        {
            vkCmdBindPipeline(p_camera_info->p_shadow_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->p_shadow_graphics_pipelines[i].pipeline);

            const VkDeviceSize vertex_buffer_offset = 0;
            vkCmdBindVertexBuffers(p_camera_info->p_shadow_command_buffers[i], 0, 1, &h_render_command->h_mesh->vbo.buffer, &vertex_buffer_offset);
            if (h_render_command->h_mesh->ibo.size != 0)
            {
                vkCmdBindIndexBuffer(p_camera_info->p_shadow_command_buffers[i], h_render_command->h_mesh->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
            }

            vkCmdBindDescriptorSets(p_camera_info->p_shadow_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->p_shadow_graphics_pipelines[i].pipeline_layout, 0, 1, &p_camera_info->p_shadow_graphics_pipelines[i].descriptor_set, 0, TG_NULL);

            if (h_render_command->h_mesh->index_count != 0)
            {
                vkCmdDrawIndexed(p_camera_info->p_shadow_command_buffers[i], (u32)(h_render_command->h_mesh->index_count), 1, 0, 0, 0); // TODO: u16
            }
            else
            {
                vkCmdDraw(p_camera_info->p_shadow_command_buffers[i], (u32)(h_render_command->h_mesh->vertex_count), 1, 0, 0);
            }
        }
        VK_CALL(vkEndCommandBuffer(p_camera_info->p_shadow_command_buffers[i]));
    }
}



tg_render_command_h tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material, v3 position, u32 global_resource_count, tg_handle* p_global_resources)
{
	TG_ASSERT(h_mesh && h_material);

    tg_render_command_h h_render_command = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_render_commands, h_render_command);

	h_render_command->type = TG_HANDLE_TYPE_RENDER_COMMAND;
	h_render_command->h_mesh = h_mesh;
	h_render_command->h_material = h_material;
    h_render_command->model_ubo = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    *((m4*)h_render_command->model_ubo.memory.p_mapped_device_memory) = tgm_m4_translate(position);

	h_render_command->camera_info_count = 0;
    for (u32 i = 0; i < TG_MAX_CAMERAS; i++)
    {
        if (p_cameras[i].type != TG_HANDLE_TYPE_INVALID)
        {
            tg__register(h_render_command, &p_cameras[i], global_resource_count, p_global_resources);
        }
    }

	return h_render_command;
}

void tg_render_command_destroy(tg_render_command_h h_render_command)
{
	TG_ASSERT(h_render_command);

    for (u32 i = 0; i < h_render_command->camera_info_count; i++)
    {
        for (u32 j = 0; j < TG_CASCADED_SHADOW_MAPS; j++)
        {
            tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_render_command->p_camera_infos[i].p_shadow_command_buffers[j]);
        }
        tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_render_command->p_camera_infos[i].command_buffer);
        tg_vulkan_pipeline_destroy(&h_render_command->p_camera_infos[i].graphics_pipeline);
    }
    tg_vulkan_buffer_destroy(&h_render_command->model_ubo);
    TG_VULKAN_RELEASE_HANDLE(h_render_command);
}

void tg_render_command_set_position(tg_render_command_h h_render_command, v3 position)
{
    *((m4*)h_render_command->model_ubo.memory.p_mapped_device_memory) = tgm_m4_translate(position);
}

#endif
