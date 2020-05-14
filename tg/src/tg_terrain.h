#ifndef TG_TERRAIN_H
#define TG_TERRAIN_H

#include "math/tg_math.h"
#include "tg_common.h"

TG_DECLARE_HANDLE(tg_terrain_chunk_entity);

TG_DECLARE_HANDLE(tg_material);

tg_terrain_chunk_entity_h    tg_terrain_chunk_entity_create(i32 x, i32 y, i32 z, tg_material_h material_h); // TODO: tg_terrain_chunk_entity_h will be removed
void                         tg_terrain_chunk_entity_destroy(tg_terrain_chunk_entity_h terrain_chunk_entity_h);



TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_terrain);

void            tg_terrain_capture(tg_terrain_h terrain_h, tg_camera_h camera_h);
tg_terrain_h    tg_terrain_create(tg_camera_h focal_point);
void            tg_terrain_update(tg_terrain_h terrain_h);
void            tg_terrain_destroy(tg_terrain_h terrain_h);

#endif
