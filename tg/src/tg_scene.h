#ifndef TG_SCENE_H
#define TG_SCENE_H

#include "tg_common.h"
#include "util/tg_list.h"



TG_DECLARE_TYPE(tg_camera);
TG_DECLARE_TYPE(tg_entity);
TG_DECLARE_TYPE(tg_point_light);

TG_DECLARE_HANDLE(tg_deferred_renderer);
TG_DECLARE_HANDLE(tg_forward_renderer);
TG_DECLARE_HANDLE(tg_render_target);



typedef struct tg_scene
{
	tg_deferred_renderer_h    deferred_renderer_h;
	tg_forward_renderer_h     forward_renderer_h;

	tg_list                   deferred_entities;
	tg_list                   forward_entities;
} tg_scene;



void                  tg_scene_begin(tg_scene* p_scene);
tg_scene              tg_scene_create(tg_camera* p_camera, u32 point_light_count, const tg_point_light* p_point_lights);
void                  tg_scene_destroy(tg_scene* p_scene);
void                  tg_scene_end(tg_scene* p_scene);
tg_render_target_h    tg_scene_get_render_target(tg_scene* p_scene);
void                  tg_scene_present(tg_scene* p_scene);
void                  tg_scene_submit(tg_scene* p_scene, tg_entity* p_entity);

#endif
