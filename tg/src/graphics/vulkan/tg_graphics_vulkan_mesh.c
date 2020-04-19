#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

typedef struct tg_normals_compute_shader_uniform_buffer
{
    u32    vertex_float_count;
    u32    offset_floats_position;
    u32    offset_floats_normal;
    u32    offset_floats_uv;
    u32    offset_floats_tangent;
    u32    offset_floats_bitangent;
} tg_normals_compute_shader_uniform_buffer;

void tg_graphics_mesh_recalculate_normal(tg_vertex_3d* p_v0, tg_vertex_3d* p_v1, tg_vertex_3d* p_v2)
{
    const v3 v01 = tgm_v3_subtract_v3(&p_v1->position, &p_v0->position);
    const v3 v02 = tgm_v3_subtract_v3(&p_v2->position, &p_v0->position);
    const v3 normal_direction = tgm_v3_cross(&v01, &v02);
    const v3 normal = tgm_v3_normalized(&normal_direction);

    p_v0->normal = normal;
    p_v1->normal = normal;
    p_v2->normal = normal;
}
void tg_graphics_mesh_recalculate_normals(u32 vertex_count, u32 index_count, const u16* p_indices, VkBuffer staging_buffer, tg_vertex_3d* p_vertices)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            // TODO: these will override normals, that have been set before. this should be interpolated.
            tg_graphics_mesh_recalculate_normal(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
#if 1
        VkShaderModule shader_module = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VkBuffer uniform_buffer = VK_NULL_HANDLE;
        VkDeviceMemory uniform_buffer_memory = VK_NULL_HANDLE;

        tg_graphics_vulkan_shader_module_create("shaders/normals_vbo.comp.spv", &shader_module);

        VkDescriptorPoolSize descriptor_pool_size = { 0 };
        {
            descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptor_pool_size.descriptorCount = 1;
        }
        tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &descriptor_pool);

        VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };
        {
            p_descriptor_set_layout_bindings[0].binding = 0;
            p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            p_descriptor_set_layout_bindings[0].descriptorCount = 1;
            p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

            p_descriptor_set_layout_bindings[1].binding = 1;
            p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            p_descriptor_set_layout_bindings[1].descriptorCount = 1;
            p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
        }
        tg_graphics_vulkan_descriptor_set_layout_create(0, 2, p_descriptor_set_layout_bindings, &descriptor_set_layout);
        tg_graphics_vulkan_descriptor_set_allocate(descriptor_pool, descriptor_set_layout, &descriptor_set);

        tg_graphics_vulkan_pipeline_layout_create(1, &descriptor_set_layout, 0, TG_NULL, &pipeline_layout);

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = { 0 };
        {
            pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            pipeline_shader_stage_create_info.pNext = TG_NULL;
            pipeline_shader_stage_create_info.flags = 0;
            pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            pipeline_shader_stage_create_info.module = shader_module;
            pipeline_shader_stage_create_info.pName = "main";
            pipeline_shader_stage_create_info.pSpecializationInfo = TG_NULL;
        }
        VkComputePipelineCreateInfo compute_pipeline_create_info = { 0 };
        {
            compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            compute_pipeline_create_info.pNext = TG_NULL;
            compute_pipeline_create_info.flags = 0;
            compute_pipeline_create_info.stage = pipeline_shader_stage_create_info;
            compute_pipeline_create_info.layout = pipeline_layout;
            compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
            compute_pipeline_create_info.basePipelineIndex = -1;
        }
        VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, TG_NULL, &pipeline)); // TODO: abstract

        command_pool = tg_graphics_vulkan_command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, compute_queue.index);
        tg_graphics_vulkan_command_buffer_allocate(command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &command_buffer);

        tg_graphics_vulkan_buffer_create(sizeof(tg_normals_compute_shader_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffer, &uniform_buffer_memory);
        tg_normals_compute_shader_uniform_buffer* p_uniform_buffer_mapped_memory = TG_NULL;
        VK_CALL(vkMapMemory(device, uniform_buffer_memory, 0, sizeof(tg_normals_compute_shader_uniform_buffer), 0, &p_uniform_buffer_mapped_memory));
        {
            p_uniform_buffer_mapped_memory->vertex_float_count = sizeof(tg_vertex_3d) / sizeof(f32);
            p_uniform_buffer_mapped_memory->offset_floats_position = offsetof(tg_vertex_3d, position) / sizeof(f32);
            p_uniform_buffer_mapped_memory->offset_floats_normal = offsetof(tg_vertex_3d, normal) / sizeof(f32);
            p_uniform_buffer_mapped_memory->offset_floats_uv = offsetof(tg_vertex_3d, uv) / sizeof(f32);
            p_uniform_buffer_mapped_memory->offset_floats_tangent = offsetof(tg_vertex_3d, tangent) / sizeof(f32);
            p_uniform_buffer_mapped_memory->offset_floats_bitangent = offsetof(tg_vertex_3d, bitangent) / sizeof(f32);
        }
        vkUnmapMemory(device, uniform_buffer_memory);

        VkDescriptorBufferInfo storage_descriptor_buffer_info = { 0 };
        {
            storage_descriptor_buffer_info.buffer = staging_buffer;
            storage_descriptor_buffer_info.offset = 0;
            storage_descriptor_buffer_info.range = VK_WHOLE_SIZE;
        }
        VkDescriptorBufferInfo uniform_descriptor_buffer_info = { 0 };
        {
            uniform_descriptor_buffer_info.buffer = uniform_buffer;
            uniform_descriptor_buffer_info.offset = 0;
            uniform_descriptor_buffer_info.range = VK_WHOLE_SIZE;
        }
        VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };
        {
            p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            p_write_descriptor_sets[0].pNext = TG_NULL;
            p_write_descriptor_sets[0].dstSet = descriptor_set;
            p_write_descriptor_sets[0].dstBinding = 0;
            p_write_descriptor_sets[0].dstArrayElement = 0;
            p_write_descriptor_sets[0].descriptorCount = 1;
            p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            p_write_descriptor_sets[0].pImageInfo = TG_NULL;
            p_write_descriptor_sets[0].pBufferInfo = &storage_descriptor_buffer_info;
            p_write_descriptor_sets[0].pTexelBufferView = TG_NULL;

            p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            p_write_descriptor_sets[1].pNext = TG_NULL;
            p_write_descriptor_sets[1].dstSet = descriptor_set;
            p_write_descriptor_sets[1].dstBinding = 1;
            p_write_descriptor_sets[1].dstArrayElement = 0;
            p_write_descriptor_sets[1].descriptorCount = 1;
            p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            p_write_descriptor_sets[1].pImageInfo = TG_NULL;
            p_write_descriptor_sets[1].pBufferInfo = &uniform_descriptor_buffer_info;
            p_write_descriptor_sets[1].pTexelBufferView = TG_NULL;
        }
        vkUpdateDescriptorSets(device, 2, p_write_descriptor_sets, 0, TG_NULL);

        tg_graphics_vulkan_command_buffer_begin(0, command_buffer);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor_set, 0, TG_NULL);
        vkCmdDispatch(command_buffer, vertex_count, 1, 1);
        VK_CALL(vkEndCommandBuffer(command_buffer));

        VkSubmitInfo submit_info = { 0 };
        {
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = TG_NULL;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = TG_NULL;
            submit_info.pWaitDstStageMask = TG_NULL;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = TG_NULL;
        }
        VK_CALL(vkQueueSubmit(compute_queue.queue, 1, &submit_info, TG_NULL));
        VK_CALL(vkQueueWaitIdle(compute_queue.queue));
#else
        for (u32 i = 0; i < vertex_count; i += 3)
        {
            tg_graphics_mesh_recalculate_normal(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
#endif
    }
}
void tg_graphics_mesh_recalculate_tangent_bitangent(tg_vertex_3d* p_v0, tg_vertex_3d* p_v1, tg_vertex_3d* p_v2)
{
    const v3 delta_p_01 = tgm_v3_subtract_v3(&p_v1->position, &p_v0->position);
    const v3 delta_p_02 = tgm_v3_subtract_v3(&p_v2->position, &p_v0->position);

    const v2 delta_uv_01 = tgm_v2_subtract_v2(&p_v1->uv, &p_v0->uv);
    const v2 delta_uv_02 = tgm_v2_subtract_v2(&p_v2->uv, &p_v0->uv);

    const f32 f = 1.0f / (delta_uv_01.x * delta_uv_02.y - delta_uv_01.y * delta_uv_02.x);

    const v3 tangent_l_part = tgm_v3_multiply_f(&delta_p_01, delta_uv_02.y);
    const v3 tangent_r_part = tgm_v3_multiply_f(&delta_p_02, delta_uv_01.y);
    const v3 tlp_minus_trp = tgm_v3_subtract_v3(&tangent_l_part, &tangent_r_part);
    const v3 tangent = tgm_v3_multiply_f(&tlp_minus_trp, f);
    const v3 normalized_tangent = tgm_v3_normalized(&tangent);

    p_v0->tangent = normalized_tangent;
    p_v1->tangent = normalized_tangent;
    p_v2->tangent = normalized_tangent;

    const v3 bitangent_l_part = tgm_v3_multiply_f(&delta_p_02, delta_uv_01.x);
    const v3 bitangent_r_part = tgm_v3_multiply_f(&delta_p_01, delta_uv_02.x);
    const v3 blp_minus_brp = tgm_v3_subtract_v3(&bitangent_l_part, &bitangent_r_part);
    const v3 bitangent = tgm_v3_multiply_f(&blp_minus_brp, f);
    const v3 normalized_bitangent = tgm_v3_normalized(&bitangent);

    p_v0->bitangent = normalized_bitangent;
    p_v1->bitangent = normalized_bitangent;
    p_v2->bitangent = normalized_bitangent;
}
void tg_graphics_mesh_recalculate_tangents_bitangents(u32 vertex_count, u32 index_count, const u16* p_indices, tg_vertex_3d* p_vertices)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            tg_graphics_mesh_recalculate_tangent_bitangent(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i += 3)
        {
            tg_graphics_mesh_recalculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
    }
}
void tg_graphics_mesh_recalculate_bitangents(u32 vertex_count, tg_vertex_3d* p_vertices)
{
    for (u32 i = 0; i < vertex_count; i++)
    {
        p_vertices[i].bitangent = tgm_v3_cross(&p_vertices[i].normal, &p_vertices[i].tangent);
        p_vertices[i].bitangent = tgm_v3_normalized(&p_vertices[i].bitangent);
    }
}

tg_mesh_h tg_graphics_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices)
{
	TG_ASSERT(vertex_count && p_positions && ((index_count && index_count % 3 == 0) || (vertex_count && vertex_count % 3 == 0)));

	tg_mesh_h mesh_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*mesh_h));
    mesh_h->vbo.vertex_count = vertex_count;
    mesh_h->ibo.index_count = index_count;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    tg_vertex_3d* vbo_data;
    const u32 vbo_size = vertex_count * sizeof(*vbo_data);
    u16* ibo_data;
    const u32 ibo_size = index_count * sizeof(*ibo_data);

    tg_graphics_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, vbo_size, 0, &vbo_data));
    {
        // positions
        for (u32 i = 0; i < vertex_count; i++)
        {
            vbo_data[i].position = p_positions[i];
        }

        // uvs
        if (p_uvs)
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                vbo_data[i].uv = p_uvs[i];
            }
        }
        else
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                vbo_data[i].uv = (v2){ 0.0f, 0.0f };
            }
        }

        // normals
        if (p_normals)
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                vbo_data[i].normal = p_normals[i];
            }
        }
        else
        {
            tg_graphics_mesh_recalculate_normals(vertex_count, index_count, p_indices, staging_buffer, vbo_data);
        }

        // tangents, bitangents
        if (p_uvs)
        {
            if (p_tangents)
            {
                for (u32 i = 0; i < vertex_count; i++)
                {
                    vbo_data[i].tangent = p_tangents[i];
                }
                tg_graphics_mesh_recalculate_bitangents(vertex_count, vbo_data);
            }
            else
            {
                tg_graphics_mesh_recalculate_tangents_bitangents(vertex_count, index_count, p_indices, vbo_data);
            }
        }
        else
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                vbo_data[i].tangent = (v3){ 0.0f, 0.0f, 0.0f };
                vbo_data[i].bitangent = (v3){ 0.0f, 0.0f, 0.0f };
            }
        }
    }
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mesh_h->vbo.buffer, &mesh_h->vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(vbo_size, staging_buffer, mesh_h->vbo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    if (index_count > 0)
    {
        tg_graphics_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
        VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, ibo_size, 0, &ibo_data));
        {
            memcpy(ibo_data, p_indices, ibo_size);
        }
        vkUnmapMemory(device, staging_buffer_memory);
        tg_graphics_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mesh_h->ibo.buffer, &mesh_h->ibo.device_memory);
        tg_graphics_vulkan_buffer_copy(ibo_size, staging_buffer, mesh_h->ibo.buffer);
        tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);
    }
    else
    {
        mesh_h->ibo.buffer = VK_NULL_HANDLE;
        mesh_h->ibo.device_memory = VK_NULL_HANDLE;
    }

    return mesh_h;
}
void tg_graphics_mesh_destroy(tg_mesh_h mesh_h)
{
    TG_ASSERT(mesh_h);

    if (mesh_h->ibo.buffer && mesh_h->ibo.device_memory)
    {
        tg_graphics_vulkan_buffer_destroy(mesh_h->ibo.buffer, mesh_h->ibo.device_memory);
    }
    tg_graphics_vulkan_buffer_destroy(mesh_h->vbo.buffer, mesh_h->vbo.device_memory);
	TG_MEMORY_ALLOCATOR_FREE(mesh_h);
}

#endif
