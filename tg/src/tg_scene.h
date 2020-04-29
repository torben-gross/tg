#ifndef TG_SCENE_H
#define TG_SCENE_H

#include "tg_common.h"



TG_DECLARE_HANDLE(tg_scene); // TODO: do we even need handles for scene, entity and such?


TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_deferred_renderer);
TG_DECLARE_HANDLE(tg_entity);
TG_DECLARE_HANDLE(tg_render_target);

TG_DECLARE_TYPE(tg_point_light);



typedef struct tg_scene
{
	tg_deferred_renderer_h deferred_renderer_h;
} tg_scene;



void                  tg_scene_begin(tg_scene_h scene_h);
tg_scene_h            tg_scene_create(tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights);
void                  tg_scene_destroy(tg_scene_h scene_h);
void                  tg_scene_draw(tg_scene_h scene_h, tg_entity_h entity_h);
void                  tg_scene_end(tg_scene_h scene_h);
tg_render_target_h    tg_scene_get_render_target(tg_scene_h scene_h);
void                  tg_scene_present(tg_scene_h scene_h);

#endif
