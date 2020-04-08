#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/memory/tg_memory_allocator.h"



typedef struct tg_scene
{
	u32                max_entity_count;
	u32                current_entity_count;
	tg_entity*         p_entities;

	struct render_data
	{
		u32            rendered_entity_count;
		v3*            p_positions;
		tg_model_h*    p_models_h;
	};
} tg_scene;



void tg_graphics_scene_create(tg_camera_h camera_h, tg_scene_h* p_scene_h)
{
	TG_ASSERT(p_scene_h);

	*p_scene_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_scene_h));
}

void tg_graphics_scene_spawn_entities(u32 count, const v3* p_positions, const tg_model_h* p_models, tg_entity_h* p_entities_h)
{
	TG_ASSERT(count > 0 && p_positions && p_models && p_entities_h);
}

void tg_graphics_scene_destroy(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	TG_MEMORY_ALLOCATOR_FREE(scene_h);
}

#endif
