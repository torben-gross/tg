#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

tg_color_image tg_color_image_create(u32 width, u32 height, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_color_image color_image = { 0 };
	color_image.type = TG_STRUCTURE_TYPE_COLOR_IMAGE;
	color_image.vulkan_image = tg_vulkan_color_image_create(width, height, (VkFormat)format, p_sampler_create_info);
	return color_image;
}

tg_color_image tg_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(p_filename);

	tg_color_image color_image = { 0 };
	color_image.type = TG_STRUCTURE_TYPE_COLOR_IMAGE;
	color_image.vulkan_image = tg_vulkan_color_image_create2(p_filename, p_sampler_create_info);
	return color_image;
}

void tg_color_image_destroy(tg_color_image* p_color_image)
{
	TG_ASSERT(p_color_image);

	tg_vulkan_color_image_destroy(&p_color_image->vulkan_image);
}



tg_color_image_3d tg_color_image_3d_create(u32 width, u32 height, u32 depth, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height && depth);

	tg_color_image_3d color_image_3d = { 0 };
	color_image_3d.type = TG_STRUCTURE_TYPE_COLOR_IMAGE_3D;
	color_image_3d.vulkan_image_3d = tg_vulkan_color_image_3d_create(width, height, depth, (VkFormat)format, p_sampler_create_info);
	return color_image_3d;
}

void tg_color_image_3d_destroy(tg_color_image_3d* p_color_image_3d)
{
	TG_ASSERT(p_color_image_3d);

	tg_vulkan_color_image_3d_destroy(&p_color_image_3d->vulkan_image_3d);
}



tg_depth_image tg_depth_image_create(u32 width, u32 height, tg_depth_image_format format, const tg_sampler_create_info* p_sampler_create_info)
{
	TG_ASSERT(width && height);

	tg_depth_image depth_image = { 0 };
	depth_image.type = TG_STRUCTURE_TYPE_DEPTH_IMAGE;
	depth_image.vulkan_image = tg_vulkan_depth_image_create(width, height, (VkFormat)format, p_sampler_create_info);
	return depth_image;
}

void tg_depth_image_destroy(tg_depth_image* p_depth_image)
{
	TG_ASSERT(p_depth_image);

	tg_vulkan_depth_image_destroy(&p_depth_image->vulkan_image);
}

#endif
