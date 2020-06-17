#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "util/tg_rectangle_packer.h"

tg_texture_atlas_h tg_texture_atlas_create_from_images(u32 image_count, tg_color_image_h* ph_color_images)
{
	TG_ASSERT(image_count && ph_color_images);

	tg_texture_atlas_h h_texture_atlas = TG_MEMORY_ALLOC(sizeof(*h_texture_atlas));
	h_texture_atlas->type = TG_HANDLE_TYPE_TEXTURE_ATLAS;
	h_texture_atlas->p_extents = TG_MEMORY_ALLOC(image_count * sizeof(*h_texture_atlas->p_extents));

	tg_rectangle_packer_rect* p_rects = TG_MEMORY_ALLOC(image_count * sizeof(*p_rects));
	for (u32 i = 0; i < image_count; i++)
	{
		p_rects[i].id = i;
		p_rects[i].left = 0;
		p_rects[i].bottom = 0;
		p_rects[i].width = ph_color_images[i]->width;
		p_rects[i].height = ph_color_images[i]->height;
	}
	u32 total_width = 0;
	u32 total_height = 0;
	tg_rectangle_packer_pack(image_count, p_rects, &total_width, &total_height);

#ifdef TG_DEBUG
	const VkPhysicalDeviceProperties physical_device_properties = tg_vulkan_physical_device_get_properties();
	TG_ASSERT(total_width <= physical_device_properties.limits.maxImageDimension2D);
	TG_ASSERT(total_height <= physical_device_properties.limits.maxImageDimension2D);
#endif

	tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
	vulkan_color_image_create_info.width = total_width;
	vulkan_color_image_create_info.height = total_height;
	vulkan_color_image_create_info.mip_levels = 1;
	vulkan_color_image_create_info.format = TG_VULKAN_COLOR_IMAGE_FORMAT;
	vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

	h_texture_atlas->color_image = tg_vulkan_color_image_create(&vulkan_color_image_create_info);
	VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
	{
		tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_texture_atlas->color_image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		for (u32 i = 0; i < image_count; i++)
		{
			VkImageCopy image_copy = { 0 };
			image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy.srcSubresource.mipLevel = 0;
			image_copy.srcSubresource.baseArrayLayer = 0;
			image_copy.srcSubresource.layerCount = 1;
			image_copy.srcOffset.x = 0;
			image_copy.srcOffset.y = 0;
			image_copy.srcOffset.z = 0;
			image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy.dstSubresource.mipLevel = 0;
			image_copy.dstSubresource.baseArrayLayer = 0;
			image_copy.dstSubresource.layerCount = 1;
			image_copy.dstOffset.x = p_rects[i].left;
			image_copy.dstOffset.y = p_rects[i].bottom;
			image_copy.dstOffset.z = 0;
			image_copy.extent.width = p_rects[i].width;
			image_copy.extent.height = p_rects[i].height;
			image_copy.extent.depth = 1;

			tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, ph_color_images[p_rects[i].id], 0, 0, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			vkCmdCopyImage(command_buffer, ph_color_images[p_rects[i].id]->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, h_texture_atlas->color_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
			tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, ph_color_images[p_rects[i].id], 0, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		}
		tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, &h_texture_atlas->color_image, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	}
	tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

	for (u32 i = 0; i < image_count; i++)
	{
		h_texture_atlas->p_extents[p_rects[i].id].left = (f32)p_rects[i].left / (f32)total_width;
		h_texture_atlas->p_extents[p_rects[i].id].bottom = (f32)p_rects[i].bottom / (f32)total_height;
		h_texture_atlas->p_extents[p_rects[i].id].right = (f32)(p_rects[i].left + p_rects[i].width) / (f32)total_width;
		h_texture_atlas->p_extents[p_rects[i].id].top = (f32)(p_rects[i].bottom + p_rects[i].height) / (f32)total_height;
	}

	TG_MEMORY_FREE(p_rects);

	return h_texture_atlas;
}

void tg_texture_atlas_destroy(tg_texture_atlas_h h_texture_atlas)
{
	TG_ASSERT(h_texture_atlas);

	tg_vulkan_color_image_destroy(&h_texture_atlas->color_image);
	TG_MEMORY_FREE(h_texture_atlas->p_extents);
	TG_MEMORY_FREE(h_texture_atlas);
}

#endif
