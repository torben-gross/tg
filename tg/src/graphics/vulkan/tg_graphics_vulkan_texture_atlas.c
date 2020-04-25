#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "util/tg_rectangle_packer.h"

tg_texture_atlas_h tgg_texture_atlas_create_from_images(u32 image_count, tg_image_h* p_images_h)
{
	TG_ASSERT(image_count && p_images_h);

	tg_texture_atlas_h texture_atlas_h = TG_MEMORY_ALLOC(sizeof(*texture_atlas_h));
	texture_atlas_h->p_extents = TG_MEMORY_ALLOC(image_count * sizeof(*texture_atlas_h->p_extents));

	tg_rect* p_rects = TG_MEMORY_ALLOC(image_count * sizeof(*p_rects));
	for (u32 i = 0; i < image_count; i++)
	{
		p_rects[i].id = i;
		p_rects[i].left = 0;
		p_rects[i].bottom = 0;
		p_rects[i].width = p_images_h[i]->width;
		p_rects[i].height = p_images_h[i]->height;
	}
	u32 total_width = 0;
	u32 total_height = 0;
	tg_rectangle_packer_pack(image_count, p_rects, &total_width, &total_height);

	tg_vulkan_image_create_info vulkan_image_create_info = { 0 };
	{
		vulkan_image_create_info.width = total_width;
		vulkan_image_create_info.height = total_height;
		vulkan_image_create_info.mip_levels = 1;
		vulkan_image_create_info.format = TG_VULKAN_IMAGE_FORMAT;
		vulkan_image_create_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
		vulkan_image_create_info.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vulkan_image_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
		vulkan_image_create_info.image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		vulkan_image_create_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		vulkan_image_create_info.min_filter = VK_FILTER_NEAREST;
		vulkan_image_create_info.mag_filter = VK_FILTER_NEAREST;
		vulkan_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vulkan_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		vulkan_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
	texture_atlas_h->texture_atlas = tgg_vulkan_image_create(&vulkan_image_create_info);

	VkCommandBuffer command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	tgg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
	for (u32 i = 0; i < image_count; i++)
	{
		VkImageCopy image_copy = { 0 };
		{
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
		}
		/*
		
		 0  =>  128 - 32
		96  =>    0

		*/
		const VkImageLayout original_layout = p_images_h[p_rects[i].id]->layout;
		if (original_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			tgg_vulkan_command_buffer_cmd_transition_image_layout(command_buffer, p_images_h[p_rects[i].id], 0, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		}
		vkCmdCopyImage(command_buffer, p_images_h[p_rects[i].id]->image, p_images_h[p_rects[i].id]->layout, texture_atlas_h->texture_atlas.image, texture_atlas_h->texture_atlas.layout, 1, &image_copy);
		if (original_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			tgg_vulkan_command_buffer_cmd_transition_image_layout(command_buffer, p_images_h[p_rects[i].id], 0, 0, original_layout, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		}
	}
	tgg_vulkan_command_buffer_cmd_transition_image_layout(command_buffer, &texture_atlas_h->texture_atlas, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	tgg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);

	for (u32 i = 0; i < image_count; i++)
	{
		texture_atlas_h->p_extents[p_rects[i].id].left = (f32)p_rects[i].left / (f32)total_width;
		texture_atlas_h->p_extents[p_rects[i].id].bottom = (f32)p_rects[i].bottom / (f32)total_height;
		texture_atlas_h->p_extents[p_rects[i].id].right = (f32)(p_rects[i].left + p_rects[i].width) / (f32)total_width;
		texture_atlas_h->p_extents[p_rects[i].id].top = (f32)(p_rects[i].bottom + p_rects[i].height) / (f32)total_height;
	}

	TG_MEMORY_FREE(p_rects);

	return texture_atlas_h;
}

void tgg_texture_atlas_destroy(tg_texture_atlas_h texture_atlas_h)
{
	TG_ASSERT(texture_atlas_h);

	tgg_vulkan_image_destroy(&texture_atlas_h->texture_atlas);
	TG_MEMORY_FREE(texture_atlas_h->p_extents);
	TG_MEMORY_FREE(texture_atlas_h);
}

#endif
