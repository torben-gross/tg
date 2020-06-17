#ifndef TG_TRANSVOXEL_TERRAIN_H
#define TG_TRANSVOXEL_TERRAIN_H

#include "tg_common.h"
#include "graphics/tg_graphics.h"

TG_DECLARE_HANDLE(tg_transvoxel_terrain);

tg_transvoxel_terrain_h tg_transvoxel_terrain_create(tg_camera_h h_camera);
void tg_transvoxel_terrain_destroy(tg_transvoxel_terrain_h h_terrain);
void tg_transvoxel_terrain_update(tg_transvoxel_terrain_h h_terrain);

#endif
