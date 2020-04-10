#ifndef TG_SCENE_H
#define TG_SCENE_H

#include "tg_common.h"



TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_entity);
TG_DECLARE_HANDLE(tg_scene);



void    tg_scene_create(tg_camera_h camera_h, tg_scene_h* p_scene_h);
void    tg_scene_destroy(tg_scene_h scene_h);

void    tg_scene_deregister_entity(tg_scene_h scene_h, tg_entity_h entity_h);
void    tg_scene_register_entity(tg_scene_h scene_h, tg_entity_h entity_h);

#endif
