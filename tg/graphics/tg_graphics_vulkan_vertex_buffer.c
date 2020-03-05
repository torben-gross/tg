#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_vertex_buffer_create(ui32 size, void* vertices, tg_vertex_buffer_h* p_vertex_buffer_h)
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
}

void tg_graphics_vertex_buffer_destroy(tg_vertex_buffer_h vertex_buffer_h)
{
    vkDestroyBuffer(device, vertex_buffer_h->buffer, NULL);
    vkFreeMemory(device, vertex_buffer_h->device_memory, NULL);
    tg_allocator_free(vertex_buffer_h);
}

#endif
