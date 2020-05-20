#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



typedef struct tg_normals_compute_shader_uniform_buffer
{
    u32    vertex_float_count;
    u32    offset_floats_position;
    u32    offset_floats_normal;
    u32    offset_floats_uv;
    u32    offset_floats_tangent;
    u32    offset_floats_bitangent;
} tg_normals_compute_shader_uniform_buffer;



void tg_mesh_internal_recalculate_normal(tg_vertex_3d* p_v0, tg_vertex_3d* p_v1, tg_vertex_3d* p_v2)
{
    const v3 v01 = tgm_v3_subtract_v3(&p_v1->position, &p_v0->position);
    const v3 v02 = tgm_v3_subtract_v3(&p_v2->position, &p_v0->position);
    const v3 normal_direction = tgm_v3_cross(&v01, &v02);
    const v3 normal = tgm_v3_normalized(&normal_direction);

    p_v0->normal = normal;
    p_v1->normal = normal;
    p_v2->normal = normal;
}

void tg_mesh_internal_recalculate_normals(u32 vertex_count, u32 index_count, const u16* p_indices, tg_vulkan_buffer* p_staging_buffer)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            // TODO: these will override normals, that have been set before. this should be interpolated (only relevant if indices exits).
            tg_mesh_internal_recalculate_normal(
                &((tg_vertex_3d*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 0]],
                &((tg_vertex_3d*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 1]],
                &((tg_vertex_3d*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 2]]
            );
        }
    }
    else
    {
#if 1
        // TODO: most of this can be created only once instead of per mesh
        VkShaderModule shader_module = VK_NULL_HANDLE;
        tg_vulkan_descriptor descriptor = { 0 };
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        tg_vulkan_buffer uniform_buffer = { 0 };



        shader_module = tg_vulkan_shader_module_create("shaders/normals_vbo.comp");

        VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };

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

        descriptor = tg_vulkan_descriptor_create(2, p_descriptor_set_layout_bindings);
        pipeline_layout = tg_vulkan_pipeline_layout_create(1, &descriptor.descriptor_set_layout, 0, TG_NULL);
        pipeline = tg_vulkan_compute_pipeline_create(shader_module, pipeline_layout);
        command_buffer = tg_vulkan_command_buffer_allocate(compute_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        uniform_buffer = tg_vulkan_buffer_create(sizeof(tg_normals_compute_shader_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->vertex_float_count = sizeof(tg_vertex_3d) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_position = offsetof(tg_vertex_3d, position) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_normal = offsetof(tg_vertex_3d, normal) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_uv = offsetof(tg_vertex_3d, uv) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_tangent = offsetof(tg_vertex_3d, tangent) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_bitangent = offsetof(tg_vertex_3d, bitangent) / sizeof(f32);

        tg_vulkan_descriptor_set_update_storage_buffer(descriptor.descriptor_set, p_staging_buffer->buffer, 0);
        tg_vulkan_descriptor_set_update_uniform_buffer(descriptor.descriptor_set, uniform_buffer.buffer, 1);

        tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor.descriptor_set, 0, TG_NULL);
            vkCmdDispatch(command_buffer, vertex_count, 1, 1);
        }
        tg_vulkan_command_buffer_end_and_submit(command_buffer, &compute_queue);



        tg_vulkan_buffer_destroy(&uniform_buffer);
        tg_vulkan_command_buffer_free(compute_command_pool, command_buffer);
        tg_vulkan_graphics_pipeline_destroy(pipeline);
        tg_vulkan_pipeline_layout_destroy(pipeline_layout);
        tg_vulkan_descriptor_destroy(&descriptor);
#else
        for (u32 i = 0; i < vertex_count; i += 3)
        {
            tg_mesh_internal_recalculate_normal(
                &((tg_vertex_3d*)p_staging_buffer->p_mapped_device_memory)[i + 0],
                &((tg_vertex_3d*)p_staging_buffer->p_mapped_device_memory)[i + 1],
                &((tg_vertex_3d*)p_staging_buffer->p_mapped_device_memory)[i + 2]
            );
        }
#endif
    }
}

void tg_mesh_internal_recalculate_tangent_bitangent(tg_vertex_3d* p_v0, tg_vertex_3d* p_v1, tg_vertex_3d* p_v2)
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

void tg_mesh_internal_recalculate_tangents_bitangents(u32 vertex_count, u32 index_count, const u16* p_indices, tg_vertex_3d* p_vertices)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            tg_mesh_internal_recalculate_tangent_bitangent(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i += 3)
        {
            tg_mesh_internal_recalculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
    }
}

void tg_mesh_internal_recalculate_bitangents(u32 vertex_count, tg_vertex_3d* p_vertices)
{
    for (u32 i = 0; i < vertex_count; i++)
    {
        p_vertices[i].bitangent = tgm_v3_cross(&p_vertices[i].normal, &p_vertices[i].tangent);
        p_vertices[i].bitangent = tgm_v3_normalized(&p_vertices[i].bitangent);
    }
}



tg_mesh_h tg_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices)
{
	TG_ASSERT(vertex_count && p_positions && ((index_count && index_count % 3 == 0) || (vertex_count && vertex_count % 3 == 0)));

	tg_mesh_h mesh_h = TG_MEMORY_ALLOC(sizeof(*mesh_h));
    mesh_h->type = TG_HANDLE_TYPE_MESH;

    tg_vulkan_buffer staging_buffer = { 0 };

    const u32 vbo_size = vertex_count * sizeof(tg_vertex_3d);
    const u32 ibo_size = index_count * sizeof(u16);// TODO: u16

    staging_buffer = tg_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // bounds
    mesh_h->bounds.min = (v3){ p_positions[0].x, p_positions[0].y, p_positions[0].z };
    mesh_h->bounds.max = (v3){ p_positions[0].x, p_positions[0].y, p_positions[0].z };
    for (u32 i = 1; i < vertex_count; i++)
    {
        mesh_h->bounds.min = tgm_v3_min(&mesh_h->bounds.min, &p_positions[i]);
        mesh_h->bounds.max = tgm_v3_max(&mesh_h->bounds.max, &p_positions[i]);
    }
    // positions
    for (u32 i = 0; i < vertex_count; i++)
    {
        ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].position = p_positions[i];
    }
    // uvs
    if (p_uvs)
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].uv = p_uvs[i];
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].uv = (v2){ 0.0f, 0.0f };
        }
    }
    // normals
    if (p_normals)
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].normal = p_normals[i];
        }
    }
    else
    {
        tg_mesh_internal_recalculate_normals(vertex_count, index_count, p_indices, &staging_buffer);
    }
    // tangents, bitangents
    if (p_uvs)
    {
        if (p_tangents)
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].tangent = p_tangents[i];
            }
            tg_mesh_internal_recalculate_bitangents(vertex_count, (tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory);
        }
        else
        {
            tg_mesh_internal_recalculate_tangents_bitangents(vertex_count, index_count, p_indices, (tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory);
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].tangent = (v3){ 0.0f, 0.0f, 0.0f };
            ((tg_vertex_3d*)staging_buffer.memory.p_mapped_device_memory)[i].bitangent = (v3){ 0.0f, 0.0f, 0.0f };
        }
    }

    mesh_h->vbo = tg_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(vbo_size, staging_buffer.buffer, mesh_h->vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    if (index_count > 0)
    {
        staging_buffer = tg_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        memcpy(staging_buffer.memory.p_mapped_device_memory, p_indices, ibo_size);
        mesh_h->ibo = tg_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        tg_vulkan_buffer_copy(ibo_size, staging_buffer.buffer, mesh_h->ibo.buffer);
        tg_vulkan_buffer_destroy(&staging_buffer);
    }
    else
    {
        mesh_h->ibo.size = 0;
        mesh_h->ibo.buffer = VK_NULL_HANDLE;
        mesh_h->ibo.memory = (tg_vulkan_memory_block){ 0 };
    }

    return mesh_h;
}

tg_mesh_h tg_mesh_create2(u32 vertex_count, const tg_vertex_3d* p_vertices)
{
    TG_ASSERT(vertex_count && p_vertices && vertex_count % 3 == 0);

    tg_mesh_h mesh_h = TG_MEMORY_ALLOC(sizeof(*mesh_h));
    mesh_h->type = TG_HANDLE_TYPE_MESH;

    tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(vertex_count * sizeof(*p_vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    memcpy(staging_buffer.memory.p_mapped_device_memory, p_vertices, vertex_count * sizeof(*p_vertices));
    tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);

    mesh_h->vbo = tg_vulkan_buffer_create(vertex_count * sizeof(*p_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(vertex_count * sizeof(*p_vertices), staging_buffer.buffer, mesh_h->vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    mesh_h->ibo.size = 0;
    mesh_h->ibo.buffer = VK_NULL_HANDLE;
    mesh_h->ibo.memory = (tg_vulkan_memory_block){ 0 };

    return mesh_h;
}

void tg_mesh_destroy(tg_mesh_h mesh_h)
{
    TG_ASSERT(mesh_h);

    if (mesh_h->ibo.buffer && mesh_h->ibo.memory.device_memory)
    {
        tg_vulkan_buffer_destroy(&mesh_h->ibo);
    }
    tg_vulkan_buffer_destroy(&mesh_h->vbo);
	TG_MEMORY_FREE(mesh_h);
}

#endif
