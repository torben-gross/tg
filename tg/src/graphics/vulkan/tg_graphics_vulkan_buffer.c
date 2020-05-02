#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



tg_compute_buffer_h tg_compute_buffer_create(u64 size, b32 visible)
{
	TG_ASSERT(size > 0);
	
	tg_compute_buffer_h compute_buffer_h = TG_MEMORY_ALLOC(sizeof(*compute_buffer_h));
	compute_buffer_h->type = TG_HANDLE_TYPE_COMPUTE_BUFFER;
	compute_buffer_h->buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, visible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	return compute_buffer_h;
}

u64 tg_compute_buffer_size(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	return compute_buffer_h->buffer.size;
}

void* tg_compute_buffer_data(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	return compute_buffer_h->buffer.p_mapped_device_memory;
}

void tg_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h)
{
	TG_ASSERT(compute_buffer_h);

	tg_vulkan_buffer_destroy(&compute_buffer_h->buffer);
	TG_MEMORY_FREE(compute_buffer_h);
}



tg_uniform_buffer_h tg_uniform_buffer_create(u64 size)
{
	TG_ASSERT(size);

	tg_uniform_buffer_h uniform_buffer_h = TG_MEMORY_ALLOC(sizeof(*uniform_buffer_h));
	uniform_buffer_h->type = TG_HANDLE_TYPE_UNIFORM_BUFFER;
	uniform_buffer_h->buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	return uniform_buffer_h;
}

void* tg_uniform_buffer_data(tg_uniform_buffer_h uniform_buffer_h)
{
	TG_ASSERT(uniform_buffer_h);

	return uniform_buffer_h->buffer.p_mapped_device_memory;
}

void tg_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h)
{
	TG_ASSERT(uniform_buffer_h);

	tg_vulkan_buffer_destroy(&uniform_buffer_h->buffer);
	TG_MEMORY_FREE(uniform_buffer_h);
}

#endif
