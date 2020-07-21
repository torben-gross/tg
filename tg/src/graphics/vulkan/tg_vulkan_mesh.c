#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#define TG_MESH_VERTEX_CAPACITY(h_mesh)    ((u32)((h_mesh)->vbo.size / sizeof(tg_vertex)))
#define TG_MESH_INDEX_CAPACITY(h_mesh)     ((u32)((h_mesh)->ibo.size / sizeof(u16)))

#include "memory/tg_memory.h"
#include "tg_assets.h"
#include "util/tg_string.h"



typedef struct tg_normals_compute_shader_uniform_buffer
{
    u32    vertex_float_count;
    u32    offset_floats_position;
    u32    offset_floats_normal;
    u32    offset_floats_uv;
    u32    offset_floats_tangent;
    u32    offset_floats_bitangent;
} tg_normals_compute_shader_uniform_buffer;



static void tg__recalculate_normal(tg_vertex* p_v0, tg_vertex* p_v1, tg_vertex* p_v2)
{
    const v3 v01 = tgm_v3_sub(p_v1->position, p_v0->position);
    const v3 v02 = tgm_v3_sub(p_v2->position, p_v0->position);
    const v3 normal = tgm_v3_normalized(tgm_v3_cross(v01, v02));

    p_v0->normal = normal;
    p_v1->normal = normal;
    p_v2->normal = normal;
}

static void tg__recalculate_normals(u32 vertex_count, u32 index_count, const u16* p_indices, tg_vulkan_buffer* p_staging_buffer)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            // TODO: these will override normals, that have been set before. this should be interpolated (only relevant if indices exits).
            tg__recalculate_normal(
                &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 0]],
                &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 1]],
                &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[p_indices[i + 2]]
            );
        }
    }
    else
    {
#if 1
        // TODO: most of this can be created only once instead of per mesh
        tg_vulkan_shader compute_shader = { 0 };
        tg_vulkan_pipeline compute_pipeline = { 0 };
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        tg_vulkan_buffer uniform_buffer = { 0 };



        compute_shader = ((tg_compute_shader_h)tg_assets_get_asset("shaders/normals_vbo.comp"))->vulkan_shader;

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

        compute_pipeline = tg_vulkan_pipeline_create_compute(&compute_shader);
        command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_COMPUTE, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        uniform_buffer = tg_vulkan_buffer_create(sizeof(tg_normals_compute_shader_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->vertex_float_count = sizeof(tg_vertex) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_position = offsetof(tg_vertex, position) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_normal = offsetof(tg_vertex, normal) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_uv = offsetof(tg_vertex, uv) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_tangent = offsetof(tg_vertex, tangent) / sizeof(f32);
        ((tg_normals_compute_shader_uniform_buffer*)uniform_buffer.memory.p_mapped_device_memory)->offset_floats_bitangent = offsetof(tg_vertex, bitangent) / sizeof(f32);

        tg_vulkan_descriptor_set_update_storage_buffer(compute_pipeline.descriptor_set, p_staging_buffer->buffer, 0);
        tg_vulkan_descriptor_set_update_uniform_buffer(compute_pipeline.descriptor_set, uniform_buffer.buffer, 1);

    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline_layout, 0, 1, &compute_pipeline.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(command_buffer, vertex_count, 1, 1);
    }
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_COMPUTE);



    tg_vulkan_buffer_destroy(&uniform_buffer);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_COMPUTE, command_buffer);
    tg_vulkan_pipeline_destroy(&compute_pipeline);
#else
    for (u32 i = 0; i < vertex_count; i += 3)
    {
        tg__recalculate_normal(
            &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[i + 0],
            &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[i + 1],
            &((tg_vertex*)p_staging_buffer->memory.p_mapped_device_memory)[i + 2]
        );
    }
#endif
    }
}

static void tg__recalculate_tangent_bitangent(tg_vertex* p_v0, tg_vertex* p_v1, tg_vertex* p_v2)
{
    const v3 delta_p_01 = tgm_v3_sub(p_v1->position, p_v0->position);
    const v3 delta_p_02 = tgm_v3_sub(p_v2->position, p_v0->position);

    const v2 delta_uv_01 = tgm_v2_sub(p_v1->uv, p_v0->uv);
    const v2 delta_uv_02 = tgm_v2_sub(p_v2->uv, p_v0->uv);

    const f32 f = 1.0f / (delta_uv_01.x * delta_uv_02.y - delta_uv_01.y * delta_uv_02.x);

    const v3 tangent_l_part = tgm_v3_mulf(delta_p_01, delta_uv_02.y);
    const v3 tangent_r_part = tgm_v3_mulf(delta_p_02, delta_uv_01.y);
    const v3 tlp_minus_trp = tgm_v3_sub(tangent_l_part, tangent_r_part);
    const v3 tangent = tgm_v3_normalized(tgm_v3_mulf(tlp_minus_trp, f));

    p_v0->tangent = tangent;
    p_v1->tangent = tangent;
    p_v2->tangent = tangent;

    const v3 bitangent_l_part = tgm_v3_mulf(delta_p_02, delta_uv_01.x);
    const v3 bitangent_r_part = tgm_v3_mulf(delta_p_01, delta_uv_02.x);
    const v3 blp_minus_brp = tgm_v3_sub(bitangent_l_part, bitangent_r_part);
    const v3 bitangent = tgm_v3_normalized(tgm_v3_mulf(blp_minus_brp, f));

    p_v0->bitangent = bitangent;
    p_v1->bitangent = bitangent;
    p_v2->bitangent = bitangent;
}

static void tg__recalculate_tangents_bitangents(u32 vertex_count, u32 index_count, const u16* p_indices, tg_vertex* p_vertices)
{
    if (index_count != 0)
    {
        for (u32 i = 0; i < index_count; i += 3)
        {
            tg__recalculate_tangent_bitangent(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i += 3)
        {
            tg__recalculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
    }
}

static void tg__recalculate_bitangents(u32 vertex_count, tg_vertex* p_vertices)
{
    for (u32 i = 0; i < vertex_count; i++)
    {
        p_vertices[i].bitangent = tgm_v3_normalized(tgm_v3_cross(p_vertices[i].normal, p_vertices[i].tangent));
    }
}

static const char* tg__skip_line(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && *p_iterator != '\n' && *p_iterator != '\r')
    {
        p_iterator++;
    }
    if (*p_iterator == '\r')
    {
        p_iterator++;
    }
    if (*p_iterator == '\n')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_token(const char* p_iterator)
{
    while (*p_iterator != '\n' && *p_iterator != '\r' && *p_iterator != ' ')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_spaces(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && *p_iterator == ' ')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_whitespace(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && (*p_iterator == '\n' || *p_iterator == '\r' || *p_iterator == ' '))
    {
        p_iterator++;
    }

    return p_iterator;
}



tg_mesh_h tg_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices)
{
	TG_ASSERT(vertex_count && p_positions && ((index_count && index_count % 3 == 0) || (vertex_count && vertex_count % 3 == 0)));

    tg_mesh_h h_mesh = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_meshes, h_mesh);
    h_mesh->type = TG_HANDLE_TYPE_MESH;
    
    h_mesh->vertex_input_attribute_count = 5;
    h_mesh->p_vertex_input_attribute_formats[0] = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
    h_mesh->p_vertex_input_attribute_formats[1] = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
    h_mesh->p_vertex_input_attribute_formats[2] = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT;
    h_mesh->p_vertex_input_attribute_formats[3] = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
    h_mesh->p_vertex_input_attribute_formats[4] = TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT;
    h_mesh->vertex_count = vertex_count;
    h_mesh->index_count = index_count;

    tg_vulkan_buffer staging_buffer = { 0 };

    const u32 vbo_size = vertex_count * sizeof(tg_vertex);
    const u32 ibo_size = index_count * sizeof(u16);// TODO: u16

    staging_buffer = tg_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // bounds
    h_mesh->bounds.min = (v3) { p_positions[0].x, p_positions[0].y, p_positions[0].z };
    h_mesh->bounds.max = (v3) { p_positions[0].x, p_positions[0].y, p_positions[0].z };
    for (u32 i = 1; i < vertex_count; i++)
    {
        h_mesh->bounds.min = tgm_v3_min(h_mesh->bounds.min, p_positions[i]);
        h_mesh->bounds.max = tgm_v3_max(h_mesh->bounds.max, p_positions[i]);
    }
    // positions
    for (u32 i = 0; i < vertex_count; i++)
    {
        ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].position = p_positions[i];
    }
    // normals
    if (p_normals)
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].normal = p_normals[i];
        }
    }
    else
    {
        tg__recalculate_normals(vertex_count, index_count, p_indices, &staging_buffer);
    }
    // uvs
    if (p_uvs)
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].uv = p_uvs[i];
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].uv = (v2){ 0.0f, 0.0f };
        }
    }
    // tangents, bitangents
    if (p_uvs)
    {
        if (p_tangents)
        {
            for (u32 i = 0; i < vertex_count; i++)
            {
                ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].tangent = p_tangents[i];
            }
            tg__recalculate_bitangents(vertex_count, (tg_vertex*)staging_buffer.memory.p_mapped_device_memory);
        }
        else
        {
            tg__recalculate_tangents_bitangents(vertex_count, index_count, p_indices, (tg_vertex*)staging_buffer.memory.p_mapped_device_memory);
        }
    }
    else
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].tangent = (v3) { 0.0f, 0.0f, 0.0f };
            ((tg_vertex*)staging_buffer.memory.p_mapped_device_memory)[i].bitangent = (v3) { 0.0f, 0.0f, 0.0f };
        }
    }

    h_mesh->vbo = tg_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    tg_vulkan_buffer_copy(vbo_size, staging_buffer.buffer, h_mesh->vbo.buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    if (index_count > 0)
    {
        staging_buffer = tg_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        tg_memory_copy(ibo_size, p_indices, staging_buffer.memory.p_mapped_device_memory);
        h_mesh->ibo = tg_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        tg_vulkan_buffer_copy(ibo_size, staging_buffer.buffer, h_mesh->ibo.buffer);
        tg_vulkan_buffer_destroy(&staging_buffer);
    }
    else
    {
        h_mesh->ibo.size = 0;
        h_mesh->ibo.buffer = VK_NULL_HANDLE;
        h_mesh->ibo.memory = (tg_vulkan_memory_block) { 0 };
    }

    h_mesh->p_vertex_positions = TG_MEMORY_ALLOC(vertex_count * sizeof(v3));
    for (u32 i = 0; i < vertex_count; i++)
    {
        h_mesh->p_vertex_positions[i] = p_positions[i];
    }

    return h_mesh;
}

tg_mesh_h tg_mesh_create2(u32 vertex_count, u32 vertex_input_attribute_count, const tg_vertex_input_attribute_format* p_vertex_input_attribute_formats, const void* p_vertices, u32 index_count, const u16* p_indices)
{
    TG_ASSERT(vertex_count && vertex_input_attribute_count <= TG_MAX_SHADER_INPUTS && p_vertex_input_attribute_formats && p_vertices && (vertex_count % 3 == 0 || (index_count && index_count % 3 == 0)));

    tg_mesh_h h_mesh = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_meshes, h_mesh);
    h_mesh->type = TG_HANDLE_TYPE_MESH;
    h_mesh->vertex_input_attribute_count = vertex_input_attribute_count;

    u64 vertex_stride = 0;
    for (u32 i = 0; i < vertex_input_attribute_count; i++)
    {
        h_mesh->p_vertex_input_attribute_formats[i] = p_vertex_input_attribute_formats[i];
        vertex_stride += (u64)tg_vertex_input_attribute_format_get_size(p_vertex_input_attribute_formats[i]);
    }

    h_mesh->vertex_count = vertex_count;
    h_mesh->vbo = tg_vulkan_buffer_create(vertex_count * vertex_stride, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (index_count)
    {
        h_mesh->index_count = index_count;
        h_mesh->ibo = tg_vulkan_buffer_create(index_count * sizeof(*p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    else
    {
        h_mesh->index_count = 0;
        h_mesh->ibo = (tg_vulkan_buffer) { 0 };
    }

    const u64 max_size = tgm_u64_max(vertex_count * vertex_stride, index_count * sizeof(u16));
    tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(max_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    
    tg_memory_copy(vertex_count * vertex_stride, p_vertices, staging_buffer.memory.p_mapped_device_memory);
    tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
    tg_vulkan_buffer_copy(vertex_count * vertex_stride, staging_buffer.buffer, h_mesh->vbo.buffer);

    if (index_count)
    {
        tg_memory_copy(index_count * sizeof * (p_indices), p_indices, staging_buffer.memory.p_mapped_device_memory);
        tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
        tg_vulkan_buffer_copy(index_count * sizeof * (p_indices), staging_buffer.buffer, h_mesh->ibo.buffer);
    }
    else
    {
        h_mesh->ibo.size = 0;
        h_mesh->ibo.buffer = VK_NULL_HANDLE;
        h_mesh->ibo.memory = (tg_vulkan_memory_block) { 0 };
    }

    tg_vulkan_buffer_destroy(&staging_buffer);

    h_mesh->p_vertex_positions = TG_MEMORY_ALLOC(vertex_count * sizeof(v3));
    const u8* p_it = (const u8*)p_vertices;
    switch (p_vertex_input_attribute_formats[0])
    {
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT:
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            h_mesh->p_vertex_positions[i] = tgm_v4_to_v3(*(v4*)p_it);
            p_it += vertex_stride;
        }
    }; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            h_mesh->p_vertex_positions[i] = *((v3*)p_it);
            p_it += vertex_stride;
        }
    }; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT:
    {
        for (u32 i = 0; i < vertex_count; i++)
        {
            h_mesh->p_vertex_positions[i].xy = *((v2*)p_it);
            h_mesh->p_vertex_positions[i].z = 0.0f;
            p_it += vertex_stride;
        }
    }; break;
    default: TG_INVALID_CODEPATH(); break;
    }

    return h_mesh;
}

tg_mesh_h tg_mesh_create_empty(u32 vertex_capacity, u32 index_capacity)
{
    tg_mesh_h h_mesh = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_meshes, h_mesh);
    h_mesh->type = TG_HANDLE_TYPE_MESH;

    h_mesh->vertex_count = 0;
    h_mesh->vbo = tg_vulkan_buffer_create(tgm_u64_max(vertex_capacity, 3) * sizeof(tg_vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    h_mesh->index_count = 0;
    if (index_capacity)
    {
        h_mesh->ibo = tg_vulkan_buffer_create(index_capacity * sizeof(u16), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    else
    {
        h_mesh->ibo.size = 0;
        h_mesh->ibo.buffer = VK_NULL_HANDLE;
        h_mesh->ibo.memory = (tg_vulkan_memory_block) { 0 };
    }

    return h_mesh;
}

tg_mesh_h tg_mesh_create_sphere(f32 radius, u32 sector_count, u32 stack_count)
{
    const u32 unique_vertex_count = (sector_count + 1) * (stack_count + 1);
    v3* p_unique_positions = TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(*p_unique_positions));
    v2* p_unique_uvs = TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(*p_unique_uvs));

    const f32 sector_step = 2.0f * TG_PI / sector_count;
    const f32 stack_step = TG_PI / stack_count;

    u32 it = 0;
    for (u32 i = 0; i < stack_count + 1; i++)
    {
        TG_ASSERT(it < unique_vertex_count);

        const f32 stack_angle = TG_PI / 2.0f - (f32)i * stack_step;
        const f32 y = radius * tgm_f32_sin(stack_angle);
        const f32 xz = radius * tgm_f32_cos(stack_angle);

        for (u32 j = 0; j < sector_count + 1; j++)
        {
            const f32 sector_angle = (f32)j * sector_step;
            
            p_unique_positions[it].x = xz * tgm_f32_cos(sector_angle);
            p_unique_positions[it].y = y;
            p_unique_positions[it].z = -xz * tgm_f32_sin(sector_angle);
            p_unique_uvs[it].x = (f32)j / sector_count;
            p_unique_uvs[it].y = (f32)i / stack_count;

            it++;
        }
    }

    const u32 max_vertex_count = 4 * unique_vertex_count;
    const u32 max_index_count = 6 * unique_vertex_count;
    v3* p_positions = TG_MEMORY_STACK_ALLOC((u64)max_vertex_count * sizeof(*p_positions));
    v2* p_uvs = TG_MEMORY_STACK_ALLOC((u64)max_vertex_count * sizeof(*p_uvs));
    u16* p_indices = TG_MEMORY_STACK_ALLOC((u64)max_index_count * sizeof(*p_indices));

    u32 vertex_count = 0;
    u32 index_count = 0;
    for (u32 i = 0; i < stack_count; i++)
    {
        for (u32 j = 0; j < sector_count; j++)
        {
            const u32 i0 = j + (sector_count + 1) * i;
            const u32 i1 = j + (sector_count + 1) * (i + 1);

            // v0 - v3
            // |     |
            // v1 - v2

            const v3 vert0 = p_unique_positions[i0    ];
            const v3 vert1 = p_unique_positions[i1    ];
            const v3 vert2 = p_unique_positions[i1 + 1];
            const v3 vert3 = p_unique_positions[i0 + 1];

            const v2 uv0 = p_unique_uvs[i0    ];
            const v2 uv1 = p_unique_uvs[i1    ];
            const v2 uv2 = p_unique_uvs[i1 + 1];
            const v2 uv3 = p_unique_uvs[i0 + 1];

            if (i == 0)
            {
                p_positions[vertex_count    ] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert2;

                p_uvs[vertex_count    ] = tgm_v2_divf(tgm_v2_add(uv0, uv3), 2.0f);
                p_uvs[vertex_count + 1] = uv1;
                p_uvs[vertex_count + 2] = uv2;

                p_indices[index_count    ] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;

                vertex_count += 3;
                index_count  += 3;
            }
            else if (i == stack_count - 1)
            {
                p_positions[vertex_count    ] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert3;

                p_uvs[vertex_count    ] = uv0;
                p_uvs[vertex_count + 1] = uv1;
                p_uvs[vertex_count + 2] = tgm_v2_divf(tgm_v2_add(uv1, uv2), 2.0f);

                p_indices[index_count    ] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;

                vertex_count += 3;
                index_count  += 3;
            }
            else
            {
                p_positions[vertex_count    ] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert2;
                p_positions[vertex_count + 3] = vert3;

                p_uvs[vertex_count    ] = uv0;
                p_uvs[vertex_count + 1] = uv1;
                p_uvs[vertex_count + 2] = uv2;
                p_uvs[vertex_count + 3] = uv3;

                p_indices[index_count    ] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;
                p_indices[index_count + 3] = vertex_count + 2;
                p_indices[index_count + 4] = vertex_count + 3;
                p_indices[index_count + 5] = vertex_count;

                vertex_count += 4;
                index_count  += 6;
            }
        }
    }

    tg_mesh_h h_mesh = tg_mesh_create(vertex_count, p_positions, TG_NULL, p_uvs, TG_NULL, index_count, p_indices);

    TG_MEMORY_STACK_FREE((u64)max_index_count * sizeof(*p_indices));
    TG_MEMORY_STACK_FREE((u64)max_vertex_count * sizeof(*p_uvs));
    TG_MEMORY_STACK_FREE((u64)max_vertex_count * sizeof(*p_positions));

    TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(*p_unique_uvs));
    TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(*p_unique_positions));

    return h_mesh;
}

tg_mesh_h tg_mesh_load(const char* p_filename, v3 scale)
{
    TG_ASSERT(p_filename);

    tg_mesh_h h_mesh = TG_NULL;

    tg_file_properties file_properties = { 0 };
    tg_platform_file_get_properties(p_filename, &file_properties);
    char* p_data = TG_MEMORY_STACK_ALLOC(file_properties.size);
    tg_platform_file_read(p_filename, file_properties.size, p_data);

    const char* p_extension = tg_string_extract_filename_extension(p_filename);
    if (tg_string_equal(p_extension, "obj"))
    {
        const char* p_it = p_data;
        const char* const p_eof = p_data + file_properties.size;

        u32 unique_vertex_count = 0;
        u32 unique_uv_count = 0;
        u32 unique_normal_count = 0;
        u32 total_triangle_count = 0;

        while (p_it < p_eof)
        {
            p_it = tg__skip_whitespace(p_it, p_eof);

            if (*p_it == 'v')
            {
                if (p_it[1] == 't')
                {
                    unique_uv_count++;
                }
                else if (p_it[1] == 'n')
                {
                    unique_normal_count++;
                }
                else
                {
                    unique_vertex_count++;
                }
            }
            else if (*p_it == 'f')
            {
                total_triangle_count++;
            }
            
            p_it = tg__skip_line(p_it, p_eof);
        }

        v3* p_unique_positions = TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(v3));
        v3* p_unique_normals = TG_MEMORY_STACK_ALLOC(unique_normal_count * sizeof(v3));
        v2* p_unique_uvs = TG_MEMORY_STACK_ALLOC(unique_uv_count * sizeof(v2));

        v3i* p_triangle_positions = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_normals = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_uvs = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));

        unique_vertex_count = 0;
        unique_uv_count = 0;
        unique_normal_count = 0;
        total_triangle_count = 0;

        p_it = p_data;
        while (p_it < p_eof)
        {
            p_it = tg__skip_whitespace(p_it, p_eof);

            if (*p_it == 'v')
            {
                if (p_it[1] == 't')
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_uvs[unique_uv_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_uvs[unique_uv_count].y = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_uv_count++;
                }
                else if (p_it[1] == 'n')
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].y = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].z = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_normal_count++;
                }
                else
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].y = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].z = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_vertex_count++;
                }
            }
            else if (*p_it == 'f')
            {
                p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);

                for (u8 i = 0; i < 3; i++)
                {
                    p_triangle_positions[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                    TG_ASSERT(p_triangle_positions[total_triangle_count].p_data[i] >= 0);
                    while (*p_it++ != '/');
                    p_it = tg__skip_spaces(p_it, p_eof);

                    if (*p_it != '/')
                    {
                        p_triangle_uvs[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                        TG_ASSERT(p_triangle_uvs[total_triangle_count].p_data[i] >= 0);
                        while (*p_it >= '0' && *p_it <= '9')
                        {
                            p_it++;
                        }
                        p_it = tg__skip_spaces(p_it, p_eof);
                    }
                    else
                    {
                        p_triangle_uvs[total_triangle_count].p_data[i] = -1;
                    }

                    if (*p_it == '/')
                    {
                        p_it++;
                        p_it = tg__skip_spaces(p_it, p_eof);
                        p_triangle_normals[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                        TG_ASSERT(p_triangle_normals[total_triangle_count].p_data[i] >= 0);
                        p_it = tg__skip_token(p_it);
                    }
                    else
                    {
                        p_triangle_normals[total_triangle_count].p_data[i] = -1;
                    }
                    p_it = tg__skip_spaces(p_it, p_eof);
                }

#ifdef TG_DEBUG
                p_it = tg__skip_spaces(p_it, p_eof);
                TG_ASSERT(*p_it == '\n' || *p_it == '\r'); // TODO: only triangulated meshes are currently supported!
#endif

                total_triangle_count++;
            }
            else
            {
                p_it = tg__skip_line(p_it, p_eof);
            }
        }

        // TODO: u16 is not enough in some cases
        //TG_ASSERT(3 * total_triangle_count < TG_U16_MAX);

        v3* p_positions = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v3));
        v3* p_normals = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v3));
        v2* p_uvs = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v2));

        for (u32 i = 0; i < total_triangle_count; i++)
        {
            p_positions[3 * i + 0] = p_unique_positions[p_triangle_positions[i].p_data[0] - 1];
            p_positions[3 * i + 1] = p_unique_positions[p_triangle_positions[i].p_data[1] - 1];
            p_positions[3 * i + 2] = p_unique_positions[p_triangle_positions[i].p_data[2] - 1];
            p_normals[3 * i + 0] = p_triangle_normals[i].p_data[0] - 1 == -1 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[0] - 1];
            p_normals[3 * i + 1] = p_triangle_normals[i].p_data[1] - 1 == -1 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[1] - 1];
            p_normals[3 * i + 2] = p_triangle_normals[i].p_data[2] - 1 == -1 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[2] - 1];
            p_uvs[3 * i + 0] = p_triangle_uvs[i].p_data[0] - 1 == -1 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[0] - 1];
            p_uvs[3 * i + 1] = p_triangle_uvs[i].p_data[1] - 1 == -1 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[1] - 1];
            p_uvs[3 * i + 2] = p_triangle_uvs[i].p_data[2] - 1 == -1 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[2] - 1];
        }

        for (u32 i = 0; i < 3 * total_triangle_count; i++) // TODO: this must go
        {
            p_positions[i].x = p_positions[i].x * scale.x;
            p_positions[i].y = p_positions[i].y * scale.y;
            p_positions[i].z = p_positions[i].z * scale.z;
        }

        h_mesh = tg_mesh_create(3 * total_triangle_count, p_positions, TG_NULL, TG_NULL, TG_NULL, 0, TG_NULL); // TODO: i feel like the normals are wrong, if included here! e.g. sponza.obj

        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v2));
        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v3));
        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v3));

        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));
        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));
        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));

        TG_MEMORY_STACK_FREE(unique_uv_count * sizeof(v2));
        TG_MEMORY_STACK_FREE(unique_normal_count * sizeof(v3));
        TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(v3));
    }

    TG_MEMORY_STACK_FREE(file_properties.size);

    return h_mesh;
}

void tg_mesh_destroy(tg_mesh_h h_mesh)
{
    TG_ASSERT(h_mesh);

    if (h_mesh->ibo.buffer && h_mesh->ibo.memory.device_memory)
    {
        tg_vulkan_buffer_destroy(&h_mesh->ibo);
    }
    tg_vulkan_buffer_destroy(&h_mesh->vbo);
    TG_VULKAN_RELEASE_HANDLE(h_mesh);
}

void tg_mesh_update2(tg_mesh_h h_mesh, u32 vertex_count, u32 vertex_stride, const void* p_vertices, u32 index_count, const u16* p_indices)
{
    TG_ASSERT(h_mesh);

    h_mesh->vertex_count = vertex_count;
    const u32 vertex_capacity = TG_MESH_VERTEX_CAPACITY(h_mesh);
    if (vertex_capacity < vertex_count)
    {
        tg_vulkan_buffer_destroy(&h_mesh->vbo);
        h_mesh->vbo = tg_vulkan_buffer_create(vertex_count * (u64)vertex_stride, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    h_mesh->index_count = index_count;
    if (index_count)
    {
        if (h_mesh->ibo.buffer && h_mesh->ibo.memory.device_memory)
        {
            const u32 index_capacity = TG_MESH_INDEX_CAPACITY(h_mesh);
            if (index_capacity < index_count)
            {
                tg_vulkan_buffer_destroy(&h_mesh->ibo);
                h_mesh->ibo = tg_vulkan_buffer_create(index_count * sizeof(u16), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }
        }
        else
        {
            h_mesh->ibo = tg_vulkan_buffer_create(index_count * sizeof(u16), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
    }

    const u64 max_size = tgm_u64_max(vertex_count * (u64)vertex_stride, index_count * sizeof(u16));
    tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(max_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    tg_memory_copy(vertex_count * (u64)vertex_stride, p_vertices, staging_buffer.memory.p_mapped_device_memory);
    tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
    tg_vulkan_buffer_copy(vertex_count * (u64)vertex_stride, staging_buffer.buffer, h_mesh->vbo.buffer);

    if (index_count)
    {
        tg_memory_copy(index_count * sizeof(*p_indices), p_vertices, staging_buffer.memory.p_mapped_device_memory);
        tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
        tg_vulkan_buffer_copy(index_count * sizeof(*p_indices), staging_buffer.buffer, h_mesh->ibo.buffer);
    }
    
    tg_vulkan_buffer_destroy(&staging_buffer);
}

#endif
