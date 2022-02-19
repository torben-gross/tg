#ifndef TG_AMANATIDES_WOO_H
#define TG_AMANATIDES_WOO_H

#include "tg_common.h"
#include "math/tg_math.h"

b32 tg_amanatides_woo(v3 ray_hit_on_grid, v3 ray_direction, v3 extent, const u32* p_voxel_grid, TG_OUT v3i* p_voxel_id);

#endif
