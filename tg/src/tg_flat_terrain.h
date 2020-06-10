#if 0

#ifndef TG_TERRAIN_H
#define TG_TERRAIN_H

#include "tg_common.h"



TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_terrain);



tg_terrain_h    tg_terrain_create(tg_camera_h focal_point_camera_h); // TODO: multiple focal points...
void            tg_terrain_update(tg_terrain_h terrain_h);
void            tg_terrain_destroy(tg_terrain_h terrain_h);

#endif

#endif
