#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

tg_color_image_h tg_color_image_create(u32 width, u32 height, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_color_image_h h_color_image = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_color_images, h_color_image);
	h_color_image->type = TG_STRUCTURE_TYPE_COLOR_IMAGE;
	h_color_image->vulkan_image = tg_vulkan_color_image_create(width, height, (VkFormat)format, p_sampler_create_info);
	return h_color_image;
}

tg_color_image_h tg_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(p_filename);

	tg_color_image_h h_color_image = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_color_images, h_color_image);
	h_color_image->type = TG_STRUCTURE_TYPE_COLOR_IMAGE;
	h_color_image->vulkan_image = tg_vulkan_color_image_create2(p_filename, p_sampler_create_info);
	return h_color_image;
}

void tg_color_image_destroy(tg_color_image_h h_color_image)
{
	TG_ASSERT(h_color_image);

	tg_vulkan_color_image_destroy(&h_color_image->vulkan_image);
}



u32 tg_color_image_format_size(tg_color_image_format format)
{
	u32 size = 0;

	switch (format)
	{
	case TG_COLOR_IMAGE_FORMAT_A8B8G8R8:               size =  4; break;
	case TG_COLOR_IMAGE_FORMAT_B8G8R8A8:               size =  4; break;
	case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT:    size =  8; break;
	case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT:    size = 16; break;
	case TG_COLOR_IMAGE_FORMAT_R32:                    size =  4; break;
	case TG_COLOR_IMAGE_FORMAT_R8:                     size =  1; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8:                   size =  2; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8:                 size =  3; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8A8:               size =  4; break;

	default: TG_INVALID_CODEPATH(); break;
	}

	return size;
}



tg_color_image_3d_h tg_color_image_3d_create(u32 width, u32 height, u32 depth, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height && depth);

	tg_color_image_3d_h h_color_image_3d = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_color_images_3d, h_color_image_3d);
	h_color_image_3d->type = TG_STRUCTURE_TYPE_COLOR_IMAGE_3D;
	h_color_image_3d->vulkan_image_3d = tg_vulkan_color_image_3d_create(width, height, depth, (VkFormat)format, p_sampler_create_info);
	return h_color_image_3d;
}

void tg_color_image_3d_destroy(tg_color_image_3d_h h_color_image_3d)
{
	TG_ASSERT(h_color_image_3d);

	tg_vulkan_color_image_3d_destroy(&h_color_image_3d->vulkan_image_3d);
}



tg_cube_map_h tg_cube_map_create(u32 dimension, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(dimension);

	tg_cube_map_h h_cube_map = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_cube_maps, h_cube_map);
	h_cube_map->type = TG_STRUCTURE_TYPE_CUBE_MAP;
	h_cube_map->vulkan_cube_map = tg_vulkan_cube_map_create(dimension, (VkFormat)format, p_sampler_create_info);
	tg_vulkan_command_buffer_begin(global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
	tg_vulkan_command_buffer_cmd_transition_cube_map_layout(
		global_graphics_command_buffer,
		&h_cube_map->vulkan_cube_map,
		0,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tg_vulkan_command_buffer_end_and_submit(global_graphics_command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
	return h_cube_map;
}

void tg_cube_map_destroy(tg_cube_map_h h_cube_map)
{
	TG_ASSERT(h_cube_map);

	tg_vulkan_cube_map_destroy(&h_cube_map->vulkan_cube_map);
}

void tg_cube_map_set_data(tg_cube_map_h h_cube_map, void* p_data)
{
	TG_ASSERT(h_cube_map && p_data);

	const VkDeviceSize size = 6LL * (VkDeviceSize)h_cube_map->vulkan_cube_map.dimension * (VkDeviceSize)h_cube_map->vulkan_cube_map.dimension * tg_color_image_format_size((tg_color_image_format)h_cube_map->vulkan_cube_map.format);
	tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	tg_memory_copy(size, p_data, staging_buffer.memory.p_mapped_device_memory);
	tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);

	tg_vulkan_command_buffer_begin(global_graphics_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
	tg_vulkan_command_buffer_cmd_transition_cube_map_layout(
		global_graphics_command_buffer,
		&h_cube_map->vulkan_cube_map,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);
	tg_vulkan_command_buffer_cmd_copy_buffer_to_cube_map(global_graphics_command_buffer, staging_buffer.buffer, &h_cube_map->vulkan_cube_map);
	tg_vulkan_command_buffer_cmd_transition_cube_map_layout(
		global_graphics_command_buffer,
		&h_cube_map->vulkan_cube_map,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tg_vulkan_command_buffer_end_and_submit(global_graphics_command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);

	tg_vulkan_buffer_destroy(&staging_buffer);
}



tg_depth_image_h tg_depth_image_create(u32 width, u32 height, tg_depth_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_depth_image_h h_depth_image = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_depth_images, h_depth_image);
	h_depth_image->type = TG_STRUCTURE_TYPE_DEPTH_IMAGE;
	h_depth_image->vulkan_image = tg_vulkan_depth_image_create(width, height, (VkFormat)format, p_sampler_create_info);
	return h_depth_image;
}

void tg_depth_image_destroy(tg_depth_image_h h_depth_image)
{
	TG_ASSERT(h_depth_image);

	tg_vulkan_depth_image_destroy(&h_depth_image->vulkan_image);
}

#endif
