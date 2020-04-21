#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "tg_common.h"
#include "graphics/tg_graphics.h"
#include "math/tg_math.h"



tg_entity_h    tg_entity_create(tg_renderer_3d_h renderer_3d_h, tg_model_h model_h);
void           tg_entity_destroy(tg_entity_h entity_h);
v3             tg_entity_get_position(tg_entity_h entity_h);
void           tg_entity_set_position(tg_entity_h entity_h, const v3* p_position);

#endif
