#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

tg_compute_buffer_h tgg_compute_buffer_create(u64 size)
{
	TG_ASSERT(size > 0);

	tg_compute_buffer_h compute_buffer_h = TG_MEMORY_ALLOC(sizeof(*compute_buffer_h));
	compute_buffer_h->buffer = tgg_vulkan_buffer_create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	return compute_buffer_h;
}

u64 tgg_compute_buffer_size(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	return compute_buffer_h->buffer.size;
}

void* tgg_compute_buffer_data(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	return compute_buffer_h->buffer.p_mapped_device_memory;
}

void tgg_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	tgg_vulkan_buffer_destroy(&compute_buffer_h->buffer);
	TG_MEMORY_FREE(compute_buffer_h);
}

#endif
