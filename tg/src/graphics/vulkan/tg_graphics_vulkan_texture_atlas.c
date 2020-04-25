#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

tg_texture_atlas_h tg_texture_atlas_create_from_images(u32 image_count, tg_image_h* p_images_h)
{
	TG_ASSERT(image_count && p_images_h);

	tg_texture_atlas_h texture_atlas_h = TG_MEMORY_ALLOC(sizeof(*texture_atlas_h));

	return texture_atlas_h;
}

void tg_texture_atlas_destroy(tg_texture_atlas_h texture_atlas_h)
{
	TG_ASSERT(texture_atlas_h);

	TG_MEMORY_FREE(texture_atlas_h);
}

#endif
