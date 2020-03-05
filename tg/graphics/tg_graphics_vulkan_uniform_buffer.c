#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"

void tg_graphics_uniform_buffer_create(ui32 size, tg_uniform_buffer_h* p_uniform_buffer_h)
{
    TG_ASSERT(p_uniform_buffer_h);

    *p_uniform_buffer_h = tg_allocator_allocate(sizeof(**p_uniform_buffer_h));

    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &(**p_uniform_buffer_h).buffer,
        &(**p_uniform_buffer_h).device_memory
    );
}

void tg_graphics_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h)
{
    vkDestroyBuffer(device, uniform_buffer_h->buffer, NULL);
    vkFreeMemory(device, uniform_buffer_h->device_memory, NULL);
    tg_allocator_free(uniform_buffer_h);
}

#endif
