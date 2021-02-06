#ifndef TG_RTVX_TERRAIN_H
#define TG_RTVX_TERRAIN_H

#include "tg_common.h"

#define TG_RTVX_TERRAIN_CELL_STRIDE    64
#define TG_RTVX_TERRAIN_VX_STRIDE      (TG_RTVX_TERRAIN_CELL_STRIDE + 1)

TG_DECLARE_HANDLE(tg_rtvx_terrain);

tg_rtvx_terrain_h    tg_rtvx_terrain_create(void);
void                 tg_rtvx_terrain_destroy(tg_rtvx_terrain_h h_terrain);

#endif
