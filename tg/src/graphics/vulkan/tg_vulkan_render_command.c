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
        vulkan_graphics_pipeline_create_info.blend_enable = VK_FALSE;
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_deferred_renderer->geometry_pass.render_pass;
    } break;
    case TG_VULKAN_MATERIAL_TYPE_FORWARD:
    {
        vulkan_graphics_pipeline_create_info.blend_enable = VK_TRUE;
        vulkan_graphics_pipeline_create_info.render_pass = h_camera->capture_pass.h_forward_renderer->shading_pass.render_pass;
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

    tg_vulkan_descriptor_set_update_uniform_buffer(p_camera_info->graphics_pipeline.descriptor_set, p_camera_info->h_camera->view_projection_ubo.buffer, 0);
    for (u32 i = 0; i < global_resource_count; i++)
    {
        const u32 binding = i + 1; // NOTE(torben): binding = 0 is reserved for internal usage
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






    // TODO: shadows
    //if (h_render_command->h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED) // TODO: forward
    //{
    //    VkCommandBufferInheritanceInfo shadow_command_buffer_inheritance_info = { 0 };
    //    shadow_command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    //    shadow_command_buffer_inheritance_info.pNext = TG_NULL;
    //    shadow_command_buffer_inheritance_info.renderPass = h_camera->capture_pass.h_deferred_renderer->shadow_pass.render_pass;
    //    shadow_command_buffer_inheritance_info.subpass = 0;
    //    shadow_command_buffer_inheritance_info.framebuffer = h_camera->capture_pass.h_deferred_renderer->shadow_pass.framebuffer.framebuffer;
    //    shadow_command_buffer_inheritance_info.occlusionQueryEnable = VK_FALSE;
    //    shadow_command_buffer_inheritance_info.queryFlags = 0;
    //    shadow_command_buffer_inheritance_info.pipelineStatistics = 0;
    //
    //    p_camera_info->shadow_command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    //    tg_vulkan_command_buffer_begin(p_camera_info->shadow_command_buffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &shadow_command_buffer_inheritance_info);
    //    {
    //        vkCmdBindPipeline(p_camera_info->shadow_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->graphics_pipeline);
    //
    //        const VkDeviceSize vertex_buffer_offset = 0;
    //        vkCmdBindVertexBuffers(p_camera_info->shadow_command_buffer, 0, 1, &h_render_command->h_mesh->vbo.buffer, &vertex_buffer_offset);
    //        if (h_render_command->h_mesh->ibo.size != 0)
    //        {
    //            vkCmdBindIndexBuffer(p_camera_info->shadow_command_buffer, h_render_command->h_mesh->ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    //        }
    //
    //        VkDescriptorSet p_descriptor_sets[2] = { 0 };
    //        p_descriptor_sets[0] = h_camera->capture_pass.h_deferred_renderer->shadow_pass.descriptor.descriptor_set;
    //        p_descriptor_sets[1] = h_render_command->h_material->descriptor.descriptor_set;
    //        
    //        vkCmdBindDescriptorSets(p_camera_info->shadow_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_camera_info->pipeline_layout, 0, 2, p_descriptor_sets, 0, TG_NULL);
    //
    //        if (h_render_command->h_mesh->index_count != 0)
    //        {
    //            vkCmdDrawIndexed(p_camera_info->shadow_command_buffer, (u32)(h_render_command->h_mesh->index_count), 1, 0, 0, 0); // TODO: u16
    //        }
    //        else
    //        {
    //            vkCmdDraw(p_camera_info->shadow_command_buffer, (u32)(h_render_command->h_mesh->vertex_count), 1, 0, 0);
    //        }
    //    }
    //    VK_CALL(vkEndCommandBuffer(p_camera_info->shadow_command_buffer));
    //}
}



tg_render_command_h tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material, u32 global_resource_count, tg_handle* p_global_resources)
{
	TG_ASSERT(h_mesh && h_material);

    tg_render_command_h h_render_command = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_render_commands, h_render_command);

	h_render_command->type = TG_HANDLE_TYPE_RENDER_COMMAND;
	h_render_command->h_mesh = h_mesh;
	h_render_command->h_material = h_material;
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
        tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_render_command->p_camera_infos[i].shadow_command_buffer);
        tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, h_render_command->p_camera_infos[i].command_buffer);
        tg_vulkan_pipeline_destroy(&h_render_command->p_camera_infos[i].graphics_pipeline);
    }
    TG_VULKAN_RELEASE_HANDLE(h_render_command);
}

#endif
