#include "tg_scene.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"



void tg_scene_begin(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_deferred_renderer_begin(scene_h->deferred_renderer_h);
}

tg_scene_h tg_scene_create(tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
	TG_ASSERT(camera_h && (point_light_count == 0 || p_point_lights));

	tg_scene_h scene_h = TG_MEMORY_ALLOC(sizeof(*scene_h));
	scene_h->deferred_renderer_h = tg_deferred_renderer_create(camera_h, point_light_count, p_point_lights);

	return scene_h;
}

void tg_scene_destroy(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_deferred_renderer_destroy(scene_h->deferred_renderer_h);
	TG_MEMORY_FREE(scene_h);
}

void tg_scene_draw(tg_scene_h scene_h, tg_entity_h entity_h)
{
	TG_ASSERT(scene_h && entity_h);

	tg_deferred_renderer_draw(scene_h->deferred_renderer_h, entity_h);
}

void tg_scene_end(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_deferred_renderer_end(scene_h->deferred_renderer_h);
}

tg_render_target_h tg_scene_get_render_target(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	return TG_NULL;
}

void tg_scene_present(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_deferred_renderer_present(scene_h->deferred_renderer_h);
}
