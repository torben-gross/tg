#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_mesh_create(ui32 vertex_count, const tgm_vec3f* p_positions, const tgm_vec3f* p_normals, const tgm_vec2f* p_uvs, ui32 index_count, const ui16* p_indices, tg_mesh_h* p_mesh_h)
{
	TG_ASSERT(vertex_count && p_positions && p_normals && p_uvs && p_mesh_h && index_count % 3 == 0);

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
        for (ui16 i = 0; i < vertex_count; i++) // TODO: ui16 only if indices stay with ui16
        {
            vbo_data[i].position = p_positions[i];
            vbo_data[i].normal = p_normals[i];
            vbo_data[i].uv = p_uvs[i];
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
