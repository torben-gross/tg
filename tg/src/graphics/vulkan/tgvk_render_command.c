#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

static void tg__register(tg_render_command_h h_render_command, tg_renderer_h h_renderer, u32 global_resource_count, tg_handle* p_global_resources)
{
    TG_ASSERT(h_render_command && h_renderer && (global_resource_count == 0 || p_global_resources));

    tgvk_render_command_renderer_info* p_renderer_info = &h_render_command->p_renderer_infos[h_render_command->renderer_info_count++];
    p_renderer_info->h_renderer = h_renderer;

    p_renderer_info->descriptor_set = tgvk_descriptor_set_create(&h_render_command->h_material->pipeline);
    tgvk_descriptor_set_update_uniform_buffer(p_renderer_info->descriptor_set.descriptor_set, &h_render_command->model_ubo, 0);
    tgvk_descriptor_set_update_uniform_buffer(p_renderer_info->descriptor_set.descriptor_set, &p_renderer_info->h_renderer->view_projection_ubo, 1);
    for (u32 i = 0; i < global_resource_count; i++)
    {
        const u32 binding = i + TG_SHADER_RESERVED_BINDINGS;
        tgvk_descriptor_set_update(p_renderer_info->descriptor_set.descriptor_set, p_global_resources[i], binding);
    }

    p_renderer_info->command_buffer = tgvk_command_buffer_create(TGVK_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkRenderPass render_pass = VK_NULL_HANDLE;
    tgvk_framebuffer* p_framebuffer = TG_NULL;
    if (h_render_command->h_material->material_type == TGVK_MATERIAL_TYPE_DEFERRED)
    {
        render_pass = shared_render_resources.geometry_render_pass;
        p_framebuffer = &h_renderer->geometry_pass.framebuffer;
    }
    else
    {
        TG_ASSERT(h_render_command->h_material->material_type == TGVK_MATERIAL_TYPE_FORWARD);
        render_pass = shared_render_resources.forward_render_pass;
        p_framebuffer = &h_renderer->forward_pass.framebuffer;
    }
    tgvk_command_buffer_begin_secondary(&p_renderer_info->command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, render_pass, p_framebuffer);
    {
        vkCmdBindPipeline(p_renderer_info->command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_render_command->h_material->pipeline.pipeline);

        if (h_render_command->h_mesh->index_buffer.buffer)
        {
            vkCmdBindIndexBuffer(p_renderer_info->command_buffer.command_buffer, h_render_command->h_mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
        }
        const VkDeviceSize vertex_buffer_offset = 0;
        TG_ASSERT(h_render_command->h_mesh->position_buffer.buffer);
        vkCmdBindVertexBuffers(
            p_renderer_info->command_buffer.command_buffer, 0, 1,
            &h_render_command->h_mesh->position_buffer.buffer,
            &vertex_buffer_offset
        );
        TG_ASSERT(h_render_command->h_mesh->normal_buffer.buffer);
        vkCmdBindVertexBuffers(
            p_renderer_info->command_buffer.command_buffer, 1, 1,
            &h_render_command->h_mesh->normal_buffer.buffer,
            &vertex_buffer_offset
        );
        if (h_render_command->h_mesh->uv_buffer.buffer)
        {
            vkCmdBindVertexBuffers(
                p_renderer_info->command_buffer.command_buffer, 2, 1,
                &h_render_command->h_mesh->uv_buffer.buffer,
                &vertex_buffer_offset
            );
        }
        if (h_render_command->h_mesh->tangent_buffer.buffer)
        {
            vkCmdBindVertexBuffers(
                p_renderer_info->command_buffer.command_buffer, 3, 1,
                &h_render_command->h_mesh->tangent_buffer.buffer,
                &vertex_buffer_offset
            );
        }
        if (h_render_command->h_mesh->bitangent_buffer.buffer)
        {
            vkCmdBindVertexBuffers(
                p_renderer_info->command_buffer.command_buffer, 4, 1,
                &h_render_command->h_mesh->bitangent_buffer.buffer,
                &vertex_buffer_offset
            );
        }

        vkCmdBindDescriptorSets(p_renderer_info->command_buffer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, h_render_command->h_material->pipeline.layout.pipeline_layout, 0, 1, &p_renderer_info->descriptor_set.descriptor_set, 0, TG_NULL);

        if (h_render_command->h_mesh->index_count != 0)
        {
            vkCmdDrawIndexed(p_renderer_info->command_buffer.command_buffer, (u32)h_render_command->h_mesh->index_count, 1, 0, 0, 0); // TODO: u16
        }
        else
        {
            vkCmdDraw(p_renderer_info->command_buffer.command_buffer, (u32)h_render_command->h_mesh->vertex_count, 1, 0, 0);
        }
    }
    TGVK_CALL(vkEndCommandBuffer(p_renderer_info->command_buffer.command_buffer));
}



tg_render_command_h tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material, v3 position, u32 global_resource_count, tg_handle* p_global_resources)
{
	TG_ASSERT(h_mesh && h_material);

    tg_render_command_h h_render_command = tgvk_handle_take(TG_STRUCTURE_TYPE_RENDER_COMMAND);
	h_render_command->h_mesh = h_mesh;
	h_render_command->h_material = h_material;
    h_render_command->model_ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(m4));
    *((m4*)h_render_command->model_ubo.memory.p_mapped_device_memory) = tgm_m4_translate(position);

	h_render_command->renderer_info_count = 0;
    tg_renderer* p_renderers = tgvk_handle_array(TG_STRUCTURE_TYPE_RENDERER);
    for (u32 i = 0; i < TG_MAX_RENDERERS; i++)
    {
        if (p_renderers[i].type != TG_STRUCTURE_TYPE_INVALID)
        {
            tg__register(h_render_command, &p_renderers[i], global_resource_count, p_global_resources);
        }
    }

	return h_render_command;
}

void tg_render_command_destroy(tg_render_command_h h_render_command)
{
	TG_ASSERT(h_render_command);

    for (u32 i = 0; i < h_render_command->renderer_info_count; i++)
    {
        // TODO: lock fence
        //tgvk_fence_wait(h_render_command->p_renderer_infos[i].h_renderer->render_target.fence);
        tgvk_command_buffer_destroy(&h_render_command->p_renderer_infos[i].command_buffer);
        tgvk_descriptor_set_destroy(&h_render_command->p_renderer_infos[i].descriptor_set);
    }
    tgvk_buffer_destroy(&h_render_command->model_ubo);
    tgvk_handle_release(h_render_command);
}

tg_mesh_h tg_render_command_get_mesh(tg_render_command_h h_render_command)
{
    TG_ASSERT(h_render_command);

    return h_render_command->h_mesh;
}

void tg_render_command_set_position(tg_render_command_h h_render_command, v3 position)
{
    TG_ASSERT(h_render_command);

    *((m4*)h_render_command->model_ubo.memory.p_mapped_device_memory) = tgm_m4_translate(position);
}

#endif
