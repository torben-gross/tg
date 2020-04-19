#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

tg_compute_buffer_h tg_graphics_compute_buffer_create(u64 size)
{
	TG_ASSERT(size > 0);

	tg_compute_buffer_h compute_buffer_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*compute_buffer_h));
	compute_buffer_h->buffer = tg_graphics_vulkan_buffer_create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	return compute_buffer_h;
}

u64 tg_graphics_compute_buffer_size(tg_compute_buffer_h compute_buffer_h)
{
	return compute_buffer_h->buffer.size;
}

void* tg_graphics_compute_buffer_data(tg_compute_buffer_h compute_buffer_h)
{
	return compute_buffer_h->buffer.p_mapped_device_memory;
}

void tg_graphics_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	tg_graphics_vulkan_buffer_destroy(&compute_buffer_h->buffer);
	TG_MEMORY_ALLOCATOR_FREE(compute_buffer_h);
}

#endif
