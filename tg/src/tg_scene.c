#include "tg_scene.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_entity.h"



void tg_scene_begin(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_list_clear(&p_scene->deferred_entities);
	tg_list_clear(&p_scene->forward_entities);
}

tg_scene tg_scene_create(tg_camera* p_camera, u32 point_light_count, const tg_point_light* p_point_lights)
{
	TG_ASSERT(p_camera && (point_light_count == 0 || p_point_lights));

	tg_scene scene = { 0 };
	scene.deferred_renderer_h = tg_deferred_renderer_create(p_camera, point_light_count, p_point_lights);
	scene.forward_renderer_h = tg_forward_renderer_create(p_camera, tg_deferred_renderer_get_render_target(scene.deferred_renderer_h));
	scene.deferred_entities = TG_LIST_CREATE(tg_entity*);
	scene.forward_entities = TG_LIST_CREATE(tg_entity*);

	return scene;
}

void tg_scene_destroy(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_list_destroy(&p_scene->forward_entities);
	tg_list_destroy(&p_scene->deferred_entities);
	tg_forward_renderer_destroy(p_scene->forward_renderer_h);
	tg_deferred_renderer_destroy(p_scene->deferred_renderer_h);
}

void tg_scene_end(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_deferred_renderer_begin(p_scene->deferred_renderer_h);
	for (u32 i = 0; i < p_scene->deferred_entities.count; i++)
	{
		tg_deferred_renderer_draw(p_scene->deferred_renderer_h, *(tg_entity**)TG_LIST_POINTER_TO(p_scene->deferred_entities, i));
	}
	tg_deferred_renderer_end(p_scene->deferred_renderer_h);
	tg_forward_renderer_begin(p_scene->forward_renderer_h);
	for (u32 i = 0; i < p_scene->forward_entities.count; i++)
	{
		tg_forward_renderer_draw(p_scene->forward_renderer_h, *(tg_entity**)TG_LIST_POINTER_TO(p_scene->forward_entities, i));
	}
	tg_forward_renderer_end(p_scene->forward_renderer_h);
}

tg_render_target_h tg_scene_get_render_target(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	return TG_NULL;
}

void tg_scene_present(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_forward_renderer_present(p_scene->forward_renderer_h);
}

void tg_scene_submit(tg_scene* p_scene, tg_entity* p_entity)
{
	TG_ASSERT(p_scene && p_entity);

	if (tg_material_is_deferred(p_entity->material_h))
	{
		tg_list_insert(&p_scene->deferred_entities, &p_entity);
	}
	else
	{
		tg_list_insert(&p_scene->forward_entities, &p_entity);
	}
}
