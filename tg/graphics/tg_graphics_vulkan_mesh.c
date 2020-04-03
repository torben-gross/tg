#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_mesh_recalculate_normal(tg_vertex* p_v0, tg_vertex* p_v1, tg_vertex* p_v2)
{
    tgm_vec3f v01 = *tgm_v3f_subtract_v3f(&v01, &p_v1->position, &p_v0->position);
    tgm_vec3f v02 = *tgm_v3f_subtract_v3f(&v02, &p_v2->position, &p_v0->position);
    tgm_vec3f normal = *tgm_v3f_cross(&normal, &v01, &v02);

    p_v0->normal = normal;
    p_v1->normal = normal;
    p_v2->normal = normal;
}
void tg_graphics_mesh_recalculate_normals(ui32 vertex_count, ui32 index_count, const ui16* p_indices, tg_vertex* p_vertices)
{
    if (index_count != 0)
    {
        for (ui32 i = 0; i < index_count; i += 3)
        {
            tg_graphics_mesh_recalculate_normal(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
        for (ui32 i = 0; i < vertex_count; i += 3)
        {
            tg_graphics_mesh_recalculate_normal(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
    }
}
void tg_graphics_mesh_recalculate_tangent_bitangent(tg_vertex* p_v0, tg_vertex* p_v1, tg_vertex* p_v2)
{
    tgm_vec3f delta_p_01 = *tgm_v3f_subtract_v3f(&delta_p_01, &p_v1->position, &p_v0->position);
    tgm_vec3f delta_p_02 = *tgm_v3f_subtract_v3f(&delta_p_02, &p_v2->position, &p_v0->position);

    tgm_vec2f delta_uv_01 = *tgm_v2f_subtract_v2f(&delta_uv_01, &p_v1->uv, &p_v0->uv);
    tgm_vec2f delta_uv_02 = *tgm_v2f_subtract_v2f(&delta_uv_02, &p_v2->uv, &p_v0->uv);

    const float f = 1.0f / (delta_uv_01.x * delta_uv_02.y - delta_uv_01.y * delta_uv_02.x);

    tgm_vec3f tangent_l_part = *tgm_v3f_multiply_f(&tangent_l_part, &delta_p_01, delta_uv_02.y);
    tgm_vec3f tangent_r_part = *tgm_v3f_multiply_f(&tangent_r_part, &delta_p_02, delta_uv_01.y);
    tgm_vec3f tangent = *tgm_v3f_multiply_f(&tangent, tgm_v3f_subtract_v3f(&tangent, &tangent_l_part, &tangent_r_part), f);

    p_v0->tangent = tangent;
    p_v1->tangent = tangent;
    p_v2->tangent = tangent;

    tgm_vec3f bitangent_l_part = *tgm_v3f_multiply_f(&bitangent_l_part, &delta_p_02, delta_uv_01.x);
    tgm_vec3f bitangent_r_part = *tgm_v3f_multiply_f(&bitangent_r_part, &delta_p_01, delta_uv_02.x);
    tgm_vec3f bitangent = *tgm_v3f_multiply_f(&bitangent, tgm_v3f_subtract_v3f(&bitangent, &bitangent_l_part, &bitangent_r_part), f);

    p_v0->bitangent = bitangent;
    p_v1->bitangent = bitangent;
    p_v2->bitangent = bitangent;
}
void tg_graphics_mesh_recalculate_tangents_bitangents(ui32 vertex_count, ui32 index_count, const ui16* p_indices, tg_vertex* p_vertices)
{
    if (index_count != 0)
    {
        for (ui32 i = 0; i < index_count; i += 3)
        {
            tg_graphics_mesh_recalculate_tangent_bitangent(&p_vertices[p_indices[i + 0]], &p_vertices[p_indices[i + 1]], &p_vertices[p_indices[i + 2]]);
        }
    }
    else
    {
        for (ui32 i = 0; i < vertex_count; i += 3)
        {
            tg_graphics_mesh_recalculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
        }
    }
}
void tg_graphics_mesh_recalculate_bitangents(ui32 vertex_count, tg_vertex* p_vertices)
{
    for (ui32 i = 0; i < vertex_count; i++)
    {
        tgm_v3f_cross(&p_vertices[i].bitangent, &p_vertices[i].normal, &p_vertices[i].tangent);
    }
}

void tg_graphics_mesh_create(ui32 vertex_count, const tgm_vec3f* p_positions, const tgm_vec3f* p_normals, const tgm_vec2f* p_uvs, const tgm_vec3f* p_tangents, ui32 index_count, const ui16* p_indices, tg_mesh_h* p_mesh_h)
{
	TG_ASSERT(vertex_count && p_positions && p_uvs && p_mesh_h && ((index_count && index_count % 3 == 0) || (vertex_count && vertex_count % 3 == 0)));

	*p_mesh_h = TG_ALLOCATOR_ALLOCATE(sizeof(**p_mesh_h));
    (**p_mesh_h).vbo.vertex_count = vertex_count;
    (**p_mesh_h).ibo.index_count = index_count;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    tg_vertex* vbo_data;
    const ui32 vbo_size = vertex_count * sizeof(*vbo_data);
    ui16* ibo_data;
    const ui32 ibo_size = index_count * sizeof(*ibo_data);

    tg_graphics_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, vbo_size, 0, &vbo_data));
    {
        // positions, uvs
        for (ui32 i = 0; i < vertex_count; i++)
        {
            vbo_data[i].position = p_positions[i];
            vbo_data[i].uv = p_uvs[i];
        }

        // normals
        if (p_normals)
        {
            for (ui32 i = 0; i < vertex_count; i++)
            {
                vbo_data[i].normal = p_normals[i];
            }
        }
        else
        {
            tg_graphics_mesh_recalculate_normals(vertex_count, index_count, p_indices, vbo_data);
        }

        // tangents, bitangents
        if (p_tangents)
        {
            for (ui32 i = 0; i < vertex_count; i++)
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
    vkUnmapMemory(device, staging_buffer_memory);
    tg_graphics_vulkan_buffer_create(vbo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(**p_mesh_h).vbo.buffer, &(**p_mesh_h).vbo.device_memory);
    tg_graphics_vulkan_buffer_copy(vbo_size, staging_buffer, (**p_mesh_h).vbo.buffer);
    tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);

    if (index_count > 0)
    {
        tg_graphics_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
        VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, ibo_size, 0, &ibo_data));
        {
            memcpy(ibo_data, p_indices, ibo_size);
        }
        vkUnmapMemory(device, staging_buffer_memory);
        tg_graphics_vulkan_buffer_create(ibo_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(**p_mesh_h).ibo.buffer, &(**p_mesh_h).ibo.device_memory);
        tg_graphics_vulkan_buffer_copy(ibo_size, staging_buffer, (**p_mesh_h).ibo.buffer);
        tg_graphics_vulkan_buffer_destroy(staging_buffer, staging_buffer_memory);
    }
    else
    {
        (**p_mesh_h).ibo.buffer = VK_NULL_HANDLE;
        (**p_mesh_h).ibo.device_memory = VK_NULL_HANDLE;
    }
}
void tg_graphics_mesh_destroy(tg_mesh_h mesh_h)
{
    TG_ASSERT(mesh_h);

    if (mesh_h->ibo.buffer && mesh_h->ibo.device_memory)
    {
        tg_graphics_vulkan_buffer_destroy(mesh_h->ibo.buffer, mesh_h->ibo.device_memory);
    }
    tg_graphics_vulkan_buffer_destroy(mesh_h->vbo.buffer, mesh_h->vbo.device_memory);
	TG_ALLOCATOR_FREE(mesh_h);
}

#endif
