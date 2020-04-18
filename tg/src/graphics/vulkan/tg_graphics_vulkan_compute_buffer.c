#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

tg_compute_buffer_h tg_graphics_compute_buffer_create(u64 size)
{
	TG_ASSERT(size > 0);

	tg_compute_buffer_h compute_buffer_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*compute_buffer_h));
	tg_graphics_vulkan_buffer_create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &compute_buffer_h->buffer, &compute_buffer_h->device_memory);
	compute_buffer_h->size = size;
	VK_CALL(vkMapMemory(device, compute_buffer_h->device_memory, 0, size, 0, &compute_buffer_h->p_mapped_memory)); // TODO: abstract

	return compute_buffer_h;
}

u64 tg_graphics_compute_buffer_size(tg_compute_buffer_h compute_buffer_h)
{
	return compute_buffer_h->size;
}

void* tg_graphics_compute_buffer_data(tg_compute_buffer_h compute_buffer_h)
{
	return compute_buffer_h->p_mapped_memory;
}

void tg_graphics_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	vkUnmapMemory(device, compute_buffer_h->device_memory);
	tg_graphics_vulkan_buffer_destroy(compute_buffer_h->buffer, compute_buffer_h->device_memory);
	TG_MEMORY_ALLOCATOR_FREE(compute_buffer_h);
}

#endif
