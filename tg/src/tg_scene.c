#include "tg_scene.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"



void tg_scene_begin(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_deferred_renderer_begin(p_scene->deferred_renderer_h);
}

tg_scene tg_scene_create(tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
	TG_ASSERT(camera_h && (point_light_count == 0 || p_point_lights));

	tg_scene scene = { 0 };
	scene.deferred_renderer_h = tg_deferred_renderer_create(camera_h, point_light_count, p_point_lights);

	return scene;
}

void tg_scene_destroy(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_deferred_renderer_destroy(p_scene->deferred_renderer_h);
}

void tg_scene_draw(tg_scene* p_scene, tg_entity* p_entity)
{
	TG_ASSERT(p_scene && p_entity);

	tg_deferred_renderer_draw(p_scene->deferred_renderer_h, p_entity);
}

void tg_scene_end(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_deferred_renderer_end(p_scene->deferred_renderer_h);
}

tg_render_target_h tg_scene_get_render_target(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	return TG_NULL;
}

void tg_scene_present(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_deferred_renderer_present(p_scene->deferred_renderer_h);
}
