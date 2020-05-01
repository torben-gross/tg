#include "tg_scene.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_entity.h"



void tg_scene_begin(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_list_clear(&p_scene->deferred_entities);
	tg_list_clear(&p_scene->forward_entities);

	for (u32 i = 0; i < p_scene->camera_count; i++)
	{
		tg_camera_clear(p_scene->p_cameras_h[i]);
	}
}

tg_scene tg_scene_create(u32 camera_count, tg_camera_h* p_cameras_h, u32 point_light_count, const tg_point_light* p_point_lights)
{
	TG_ASSERT(camera_count && p_cameras_h && (point_light_count == 0 || p_point_lights));

	tg_scene scene = { 0 };
	scene.camera_count = camera_count;
	scene.p_cameras_h = TG_MEMORY_ALLOC(camera_count * (sizeof(*scene.p_cameras_h) + sizeof(*scene.p_deferred_renderers_h) + sizeof(*scene.p_forward_renderers_h)));
	scene.p_deferred_renderers_h = (tg_deferred_renderer_h*)&scene.p_cameras_h[camera_count];
	scene.p_forward_renderers_h = (tg_forward_renderer_h*)&scene.p_deferred_renderers_h[camera_count];
	for (u32 i = 0; i < camera_count; i++)
	{
		scene.p_cameras_h[i] = p_cameras_h[i];
		scene.p_deferred_renderers_h[i] = tg_deferred_renderer_create(p_cameras_h[i], point_light_count, p_point_lights);
		scene.p_forward_renderers_h[i] = tg_forward_renderer_create(p_cameras_h[i]);// TODO: lighting
	}
	scene.deferred_entities = TG_LIST_CREATE(tg_entity*);
	scene.forward_entities = TG_LIST_CREATE(tg_entity*);

	return scene;
}

void tg_scene_destroy(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	tg_list_destroy(&p_scene->forward_entities);
	tg_list_destroy(&p_scene->deferred_entities);
	for (u32 i = 0; i < p_scene->camera_count; i++)
	{
		tg_forward_renderer_destroy(p_scene->p_forward_renderers_h[p_scene->camera_count - i - 1]);
		tg_deferred_renderer_destroy(p_scene->p_deferred_renderers_h[p_scene->camera_count - i - 1]);
	}
	TG_MEMORY_FREE(p_scene->p_cameras_h);
}

void tg_scene_end(tg_scene* p_scene)
{
	TG_ASSERT(p_scene);

	for (u32 c = 0; c < p_scene->camera_count; c++)
	{
		tg_deferred_renderer_begin(p_scene->p_deferred_renderers_h[c]);
		for (u32 e = 0; e < p_scene->deferred_entities.count; e++)
		{
			tg_deferred_renderer_draw(p_scene->p_deferred_renderers_h[c], *(tg_entity**)TG_LIST_POINTER_TO(p_scene->deferred_entities, e), c);
		}
		tg_deferred_renderer_end(p_scene->p_deferred_renderers_h[c]);
		tg_forward_renderer_begin(p_scene->p_forward_renderers_h[c]);
		for (u32 j = 0; j < p_scene->forward_entities.count; j++)
		{
			tg_forward_renderer_draw(p_scene->p_forward_renderers_h[c], *(tg_entity**)TG_LIST_POINTER_TO(p_scene->forward_entities, j), c);
		}
		tg_forward_renderer_end(p_scene->p_forward_renderers_h[c]);
	}
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
