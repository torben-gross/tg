#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

tg_color_image_h tg_color_image_create(u32 width, u32 height, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_color_image_h h_color_image = tgvk_handle_take(TG_STRUCTURE_TYPE_COLOR_IMAGE);
	h_color_image->image = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR, width, height, (VkFormat)format, p_sampler_create_info);
	return h_color_image;
}

tg_color_image_h tg_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(p_filename);

	tg_color_image_h h_color_image = tgvk_handle_take(TG_STRUCTURE_TYPE_COLOR_IMAGE);
	h_color_image->image = tgvk_image_create2(TGVK_IMAGE_TYPE_COLOR, p_filename, p_sampler_create_info);
	return h_color_image;
}

void tg_color_image_destroy(tg_color_image_h h_color_image)
{
	TG_ASSERT(h_color_image);

	tgvk_image_destroy(&h_color_image->image);
}



u32 tg_color_image_format_channels(tg_color_image_format format)
{
	u32 result = 0;

	switch (format)
	{
	case TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM:         result = 4; break;
	case TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM:         result = 4; break;
	case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT:    result = 4; break;
	case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT:    result = 4; break;
	case TG_COLOR_IMAGE_FORMAT_R32_UINT:               result = 1; break;
	case TG_COLOR_IMAGE_FORMAT_R8_UINT:                result = 1; break;
	case TG_COLOR_IMAGE_FORMAT_R8_UNORM:               result = 1; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8_UNORM:             result = 2; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM:           result = 3; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM:         result = 4; break;

	default: TG_INVALID_CODEPATH(); break;
	}

	return result;
}

tg_size tg_color_image_format_size(tg_color_image_format format)
{
	tg_size result = 0;

	switch (format)
	{
	case TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM:         result =  4; break;
	case TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM:         result =  4; break;
	case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT:    result =  8; break;
	case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT:    result = 16; break;
	case TG_COLOR_IMAGE_FORMAT_R32_UINT:               result =  4; break;
	case TG_COLOR_IMAGE_FORMAT_R8_UINT:                result =  1; break;
	case TG_COLOR_IMAGE_FORMAT_R8_UNORM:               result =  1; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8_UNORM:             result =  2; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM:           result =  3; break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM:         result =  4; break;

	default: TG_INVALID_CODEPATH(); break;
	}

	return result;
}



tg_storage_image_3d_h tg_storage_image_3d_create(u32 width, u32 height, u32 depth, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height && depth);

	tg_storage_image_3d_h h_storage_image_3d = tgvk_handle_take(TG_STRUCTURE_TYPE_STORAGE_IMAGE_3D);
	h_storage_image_3d->image_3d = tgvk_image_3d_create(TGVK_IMAGE_TYPE_STORAGE, width, height, depth, (VkFormat)format, p_sampler_create_info);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	tgvk_cmd_transition_image_3d_layout(
		p_command_buffer,
		&h_storage_image_3d->image_3d,
		0,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	return h_storage_image_3d;
}

void tg_storage_image_3d_destroy(tg_storage_image_3d_h h_storage_image_3d)
{
	TG_ASSERT(h_storage_image_3d);

	tgvk_image_3d_destroy(&h_storage_image_3d->image_3d);
	tgvk_handle_release(h_storage_image_3d);
}

void tg_storage_image_3d_set_data(tg_storage_image_3d_h h_storage_image_3d, void* p_data)
{
	TG_ASSERT(h_storage_image_3d && p_data);

	const tg_size size = (tg_size)h_storage_image_3d->image_3d.width * (tg_size)h_storage_image_3d->image_3d.height * (tg_size)h_storage_image_3d->image_3d.depth * tg_color_image_format_size((tg_color_image_format)h_storage_image_3d->image_3d.format);
	tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(size);
	tg_memcpy(size, p_data, p_staging_buffer->memory.p_mapped_device_memory);
	tgvk_buffer_flush_host_to_device(p_staging_buffer);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	tgvk_cmd_transition_image_3d_layout(
		p_command_buffer,
		&h_storage_image_3d->image_3d,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);
	tgvk_cmd_copy_buffer_to_image_3d(p_command_buffer, p_staging_buffer, &h_storage_image_3d->image_3d);
	tgvk_cmd_transition_image_3d_layout(
		p_command_buffer,
		&h_storage_image_3d->image_3d,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	tgvk_global_staging_buffer_release();
}



tg_cube_map_h tg_cube_map_create(u32 dimension, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(dimension);

	tg_cube_map_h h_cube_map = tgvk_handle_take(TG_STRUCTURE_TYPE_CUBE_MAP);

	h_cube_map->cube_map = tgvk_cube_map_create(dimension, (VkFormat)format, p_sampler_create_info);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	tgvk_cmd_transition_cube_map_layout(
		p_command_buffer,
		&h_cube_map->cube_map,
		0,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	return h_cube_map;
}

void tg_cube_map_destroy(tg_cube_map_h h_cube_map)
{
	TG_ASSERT(h_cube_map);

	tgvk_cube_map_destroy(&h_cube_map->cube_map);
}

void tg_cube_map_set_data(tg_cube_map_h h_cube_map, void* p_data)
{
	TG_ASSERT(h_cube_map && p_data);

	const VkDeviceSize size = 6LL * (VkDeviceSize)h_cube_map->cube_map.dimension * (VkDeviceSize)h_cube_map->cube_map.dimension * tg_color_image_format_size((tg_color_image_format)h_cube_map->cube_map.format);
	tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(size);
	tg_memcpy(size, p_data, p_staging_buffer->memory.p_mapped_device_memory);
	tgvk_buffer_flush_host_to_device(p_staging_buffer);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	tgvk_cmd_transition_cube_map_layout(
		p_command_buffer,
		&h_cube_map->cube_map,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT
	);
	tgvk_cmd_copy_buffer_to_cube_map(p_command_buffer, p_staging_buffer, &h_cube_map->cube_map);
	tgvk_cmd_transition_cube_map_layout(
		p_command_buffer,
		&h_cube_map->cube_map,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	tgvk_global_staging_buffer_release();
}



tg_depth_image_h tg_depth_image_create(u32 width, u32 height, tg_depth_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_depth_image_h h_depth_image = tgvk_handle_take(TG_STRUCTURE_TYPE_DEPTH_IMAGE);
	h_depth_image->image = tgvk_image_create(TGVK_IMAGE_TYPE_DEPTH, width, height, (VkFormat)format, p_sampler_create_info);
	return h_depth_image;
}

void tg_depth_image_destroy(tg_depth_image_h h_depth_image)
{
	TG_ASSERT(h_depth_image);

	tgvk_image_destroy(&h_depth_image->image);
}

#endif
