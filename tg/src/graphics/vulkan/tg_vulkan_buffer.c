#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



tg_storage_buffer tg_storage_buffer_create(u64 size, b32 visible)
{
	TG_ASSERT(size > 0);
	
	tg_storage_buffer storage_buffer = { 0 };
	storage_buffer.type = TG_STRUCTURE_TYPE_STORAGE_BUFFER;
	storage_buffer.vulkan_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, visible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	return storage_buffer;
}

void* tg_storage_buffer_data(tg_storage_buffer* p_storage_buffer)
{
	TG_ASSERT(p_storage_buffer);

	return p_storage_buffer->vulkan_buffer.memory.p_mapped_device_memory;
}

void tg_storage_buffer_destroy(tg_storage_buffer* p_storage_buffer)
{
	TG_ASSERT(p_storage_buffer);

	tg_vulkan_buffer_destroy(&p_storage_buffer->vulkan_buffer);
}

u64 tg_storage_buffer_size(tg_storage_buffer* p_storage_buffer)
{
	TG_ASSERT(p_storage_buffer);

	return p_storage_buffer->vulkan_buffer.memory.size;
}



tg_uniform_buffer tg_uniform_buffer_create(u64 size)
{
	TG_ASSERT(size);

	tg_uniform_buffer uniform_buffer = { 0 };
	uniform_buffer.type = TG_STRUCTURE_TYPE_UNIFORM_BUFFER;
	uniform_buffer.vulkan_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	return uniform_buffer;
}

void* tg_uniform_buffer_data(tg_uniform_buffer* p_uniform_buffer)
{
	TG_ASSERT(p_uniform_buffer);

	return p_uniform_buffer->vulkan_buffer.memory.p_mapped_device_memory;
}

void tg_uniform_buffer_destroy(tg_uniform_buffer* p_uniform_buffer)
{
	TG_ASSERT(p_uniform_buffer);

	tg_vulkan_buffer_destroy(&p_uniform_buffer->vulkan_buffer);
}

u64 tg_uniform_buffer_size(tg_uniform_buffer* p_uniform_buffer)
{
	TG_ASSERT(p_uniform_buffer);

	return p_uniform_buffer->vulkan_buffer.memory.size;
}

#endif
