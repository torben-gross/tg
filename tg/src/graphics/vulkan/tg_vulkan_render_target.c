#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

u32 tg_render_target_get_width(tg_render_target_h h_render_target)
{
	TG_ASSERT(h_render_target);

	const u32 result = h_render_target->color_attachment.width;
	return result;
}

u32 tg_render_target_get_height(tg_render_target_h h_render_target)
{
	TG_ASSERT(h_render_target);

	const u32 result = h_render_target->color_attachment.height;
	return result;
}

tg_color_image_format tg_render_target_get_color_format(tg_render_target_h h_render_target)
{
	TG_ASSERT(h_render_target);

	const tg_color_image_format result = (tg_color_image_format)h_render_target->color_attachment.format;
	return result;
}

void tg_render_target_get_color_data_copy(tg_render_target_h h_render_target, TG_INOUT u32* p_buffer_size, TG_OUT void* p_buffer)
{
	TG_ASSERT(h_render_target && p_buffer_size);

	const u32 pixel_size = tg_color_image_format_size((tg_color_image_format)h_render_target->color_attachment.format);
	const u32 required_size = h_render_target->color_attachment.width * h_render_target->color_attachment.height * pixel_size;

	if (!*p_buffer_size || !p_buffer)
	{
		*p_buffer_size = required_size;
	}
	else
	{
		TG_ASSERT(p_buffer && *p_buffer_size >= required_size);

		tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
		tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(required_size);
		tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			tgvk_cmd_transition_color_image_layout(
				p_command_buffer,
				&h_render_target->color_attachment_copy,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
			tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, &h_render_target->color_attachment_copy, p_staging_buffer->buffer);
			tgvk_cmd_transition_color_image_layout(
				p_command_buffer,
				&h_render_target->color_attachment_copy,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}
		TGVK_CALL(vkEndCommandBuffer(p_command_buffer->command_buffer));

		tgvk_fence_wait(h_render_target->fence);
		tgvk_fence_reset(h_render_target->fence);

		VkSubmitInfo submit_info = { 0 };
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = TG_NULL;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = TG_NULL;
		submit_info.pWaitDstStageMask = TG_NULL;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &p_command_buffer->command_buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = TG_NULL;

		tgvk_queue_submit(TGVK_QUEUE_TYPE_GRAPHICS, 1, &submit_info, h_render_target->fence);
		tgvk_fence_wait(h_render_target->fence);
		tgvk_fence_reset(h_render_target->fence);
		tgvk_buffer_flush_device_to_host_range(p_staging_buffer, 0, required_size);
		tg_memcpy(required_size, p_staging_buffer->memory.p_mapped_device_memory, p_buffer);
		tgvk_global_staging_buffer_release();
	}
}

#endif
