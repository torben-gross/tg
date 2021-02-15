#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN



tg_storage_buffer_h tg_storage_buffer_create(tg_size size, b32 visible)
{
	TG_ASSERT(size > 0);
	
	tg_storage_buffer_h h_storage_buffer = tgvk_handle_take(TG_STRUCTURE_TYPE_STORAGE_BUFFER);
	h_storage_buffer->buffer = TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, visible ? TGVK_MEMORY_HOST : TGVK_MEMORY_DEVICE);
	return h_storage_buffer;
}

void* tg_storage_buffer_data(tg_storage_buffer_h h_storage_buffer)
{
	TG_ASSERT(h_storage_buffer);

	return h_storage_buffer->buffer.memory.p_mapped_device_memory;
}

void tg_storage_buffer_destroy(tg_storage_buffer_h h_storage_buffer)
{
	TG_ASSERT(h_storage_buffer);

	tgvk_buffer_destroy(&h_storage_buffer->buffer);
	tgvk_handle_release(h_storage_buffer);
}

tg_size tg_storage_buffer_size(tg_storage_buffer_h h_storage_buffer)
{
	TG_ASSERT(h_storage_buffer);

	tg_size result = (tg_size)h_storage_buffer->buffer.memory.size;
	return result;
}



tg_uniform_buffer_h tg_uniform_buffer_create(tg_size size)
{
	TG_ASSERT(size);

	tg_uniform_buffer_h h_uniform_buffer = tgvk_handle_take(TG_STRUCTURE_TYPE_UNIFORM_BUFFER);
	h_uniform_buffer->buffer = TGVK_UNIFORM_BUFFER_CREATE(size);
	return h_uniform_buffer;
}

void* tg_uniform_buffer_data(tg_uniform_buffer_h h_uniform_buffer)
{
	TG_ASSERT(h_uniform_buffer);

	return h_uniform_buffer->buffer.memory.p_mapped_device_memory;
}

void tg_uniform_buffer_destroy(tg_uniform_buffer_h h_uniform_buffer)
{
	TG_ASSERT(h_uniform_buffer);

	tgvk_buffer_destroy(&h_uniform_buffer->buffer);
	tgvk_handle_release(h_uniform_buffer);
}

tg_size tg_uniform_buffer_size(tg_uniform_buffer_h h_uniform_buffer)
{
	TG_ASSERT(h_uniform_buffer);

	tg_size result = (tg_size)h_uniform_buffer->buffer.memory.size;
	return result;
}

#endif
