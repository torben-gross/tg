#include "graphics/vulkan/tgvk_render_target.h"

#ifdef TG_VULKAN

u32 tg_render_target_get_width(tg_render_target* p_render_target)
{
	TG_ASSERT(p_render_target);

	const u32 result = p_render_target->color_attachment.width;
	return result;
}

u32 tg_render_target_get_height(tg_render_target* p_render_target)
{
	TG_ASSERT(p_render_target);

	const u32 result = p_render_target->color_attachment.height;
	return result;
}

tg_color_image_format tg_render_target_get_color_format(tg_render_target* p_render_target)
{
	TG_ASSERT(p_render_target);

	const tg_color_image_format result = (tg_color_image_format)p_render_target->color_attachment.format;
	return result;
}

void tg_render_target_get_color_data_copy(tg_render_target* p_render_target, TG_INOUT u32* p_buffer_size, TG_OUT void* p_buffer)
{
	TG_ASSERT(p_render_target && p_buffer_size);

	const u32 pixel_size = (u32)tg_color_image_format_size((tg_color_image_format)p_render_target->color_attachment.format);
	const u32 required_size = p_render_target->color_attachment.width * p_render_target->color_attachment.height * pixel_size;

	if (!*p_buffer_size || !p_buffer)
	{
		*p_buffer_size = required_size;
	}
	else
	{
		TG_ASSERT(p_buffer && *p_buffer_size >= required_size);

		tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(required_size);
		tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
		tgvk_cmd_transition_image_layout(p_command_buffer, &p_render_target->color_attachment_copy, TGVK_LAYOUT_SHADER_READ_CFV, TGVK_LAYOUT_TRANSFER_READ);
		tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, &p_render_target->color_attachment_copy, p_staging_buffer);
		tgvk_cmd_transition_image_layout(p_command_buffer, &p_render_target->color_attachment_copy, TGVK_LAYOUT_TRANSFER_READ, TGVK_LAYOUT_SHADER_READ_CFV);
		TGVK_CALL(vkEndCommandBuffer(p_command_buffer->buffer));

		tgvk_fence_wait(p_render_target->fence);
		tgvk_fence_reset(p_render_target->fence);

		VkSubmitInfo submit_info = { 0 };
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = TG_NULL;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = TG_NULL;
		submit_info.pWaitDstStageMask = TG_NULL;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &p_command_buffer->buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = TG_NULL;

		tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, p_render_target->fence);
		tgvk_fence_wait(p_render_target->fence);
		tgvk_fence_reset(p_render_target->fence);
		tg_memcpy(required_size, p_staging_buffer->memory.p_mapped_device_memory, p_buffer);
		tgvk_global_staging_buffer_release();
	}
}



tg_render_target tgvk_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tgvk_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tgvk_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
	tg_render_target render_target = { 0 };

	render_target.color_attachment = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE, color_width, color_height, color_format, p_color_sampler_create_info TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));
	render_target.color_attachment_copy = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE, color_width, color_height, color_format, p_color_sampler_create_info TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));
	render_target.depth_attachment = tgvk_image_create(TGVK_IMAGE_TYPE_DEPTH, depth_width, depth_height, depth_format, p_depth_sampler_create_info TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));
	render_target.depth_attachment_copy = tgvk_image_create(TGVK_IMAGE_TYPE_DEPTH, depth_width, depth_height, depth_format, p_depth_sampler_create_info TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_cmd_transition_image_layout(p_command_buffer, &render_target.color_attachment, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE);
	tgvk_cmd_transition_image_layout(p_command_buffer, &render_target.color_attachment_copy, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_SHADER_READ_CFV);
	tgvk_cmd_transition_image_layout(p_command_buffer, &render_target.depth_attachment, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE);
	tgvk_cmd_transition_image_layout(p_command_buffer, &render_target.depth_attachment_copy, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_SHADER_READ_CFV);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	render_target.fence = tgvk_fence_create(fence_create_flags);

	return render_target;
}

void tgvk_render_target_destroy(tg_render_target* p_render_target)
{
	tgvk_fence_destroy(p_render_target->fence);
	tgvk_image_destroy(&p_render_target->depth_attachment_copy);
	tgvk_image_destroy(&p_render_target->color_attachment_copy);
	tgvk_image_destroy(&p_render_target->depth_attachment);
	tgvk_image_destroy(&p_render_target->color_attachment);
}



void tgvk_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding)
{
	tgvk_descriptor_set_update_image(descriptor_set, &p_render_target->color_attachment_copy, dst_binding);
	// TODO: select color or depth somewhere
}

#endif
