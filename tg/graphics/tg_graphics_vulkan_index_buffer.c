#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_index_buffer_create(ui16 count, ui16* indices, tg_index_buffer_h* p_index_buffer_h)
{
    TG_ASSERT(count && indices && p_index_buffer_h);

    *p_index_buffer_h = tg_allocator_allocate(sizeof(**p_index_buffer_h));
    const VkDeviceSize size = count * sizeof(*indices);

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, indices, (size_t)size);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_index_buffer ibo = { 0 };
    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(**p_index_buffer_h).buffer,
        &(**p_index_buffer_h).device_memory
    );

    tg_graphics_vulkan_buffer_copy(size, &staging_buffer, &(**p_index_buffer_h).buffer);
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

void tg_graphics_index_buffer_destroy(tg_index_buffer_h index_buffer_h)
{
    vkDestroyBuffer(device, index_buffer_h->buffer, NULL);
    vkFreeMemory(device, index_buffer_h->device_memory, NULL);
    tg_allocator_free(index_buffer_h);
}

#endif
