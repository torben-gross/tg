#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

tg_uniform_buffer_h tg_graphics_uniform_buffer_create(u64 size)
{
	TG_ASSERT(size);

	tg_uniform_buffer_h uniform_buffer_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*uniform_buffer_h));

	uniform_buffer_h->size = size;
	tg_graphics_vulkan_buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniform_buffer_h->buffer, &uniform_buffer_h->device_memory);
	VK_CALL(vkMapMemory(device, uniform_buffer_h->device_memory, 0, size, 0, &uniform_buffer_h->p_mapped_memory));

	return uniform_buffer_h;
}

void* tg_graphics_uniform_buffer_data(tg_uniform_buffer_h uniform_buffer_h)
{
	TG_ASSERT(uniform_buffer_h);

	return uniform_buffer_h->p_mapped_memory;
}

void tg_graphics_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h)
{
	TG_ASSERT(uniform_buffer_h);

	vkUnmapMemory(device, uniform_buffer_h->device_memory);
	tg_graphics_vulkan_buffer_destroy(uniform_buffer_h->buffer, uniform_buffer_h->device_memory);
	TG_MEMORY_ALLOCATOR_FREE(uniform_buffer_h);
}

#endif
