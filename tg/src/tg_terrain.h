#ifndef TG_TERRAIN_H
#define TG_TERRAIN_H

#include "tg_common.h"

TG_DECLARE_HANDLE(tg_mesh);

tg_mesh_h    tg_terrain_generate_chunk(i32 chunk_index_x, i32 chunk_index_y, i32 chunk_index_z, u8 lod);

#endif
