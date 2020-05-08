#ifndef TG_TERRAIN_H
#define TG_TERRAIN_H

#include "math/tg_math.h"
#include "tg_common.h"

TG_DECLARE_HANDLE(tg_terrain_chunk_entity);

TG_DECLARE_HANDLE(tg_material);

tg_terrain_chunk_entity_h    tg_terrain_chunk_entity_create(i32 x, i32 y, i32 z, tg_material_h material_h);
void                         tg_terrain_chunk_entity_destroy(tg_terrain_chunk_entity_h terrain_chunk_entity_h);

#endif
