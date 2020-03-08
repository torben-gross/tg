#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_vertex_buffer_create(ui32 size, ui32 layout_element_count, const tg_vertex_shader_layout_element* layout, void* vertices, tg_vertex_buffer_h* p_vertex_buffer_h)
{
    *p_vertex_buffer_h = tg_allocator_allocate(sizeof(**p_vertex_buffer_h));

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, vertices, size);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(**p_vertex_buffer_h).buffer,
        &(**p_vertex_buffer_h).device_memory);

    tg_graphics_vulkan_buffer_copy(size, &staging_buffer, &(**p_vertex_buffer_h).buffer);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);

    if (layout_element_count == 0)
    {
        (**p_vertex_buffer_h).layout_element_count = 0;
        (**p_vertex_buffer_h).layout = NULL;
    }
    else
    {
        (**p_vertex_buffer_h).layout_element_count = layout_element_count;
        const ui32 layout_size = layout_element_count * sizeof(*layout);
        (**p_vertex_buffer_h).layout = tg_allocator_allocate(layout_size);
        memcpy((**p_vertex_buffer_h).layout, layout, layout_size);
    }
}

void tg_graphics_vertex_buffer_destroy(tg_vertex_buffer_h vertex_buffer_h)
{
    if (vertex_buffer_h->layout_element_count != 0)
    {
        tg_allocator_free(vertex_buffer_h->layout);
    }
    vkDestroyBuffer(device, vertex_buffer_h->buffer, NULL);
    vkFreeMemory(device, vertex_buffer_h->device_memory, NULL);
    tg_allocator_free(vertex_buffer_h);
}

#endif
