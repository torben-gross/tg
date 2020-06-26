#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"



typedef struct tg_entity tg_entity;
typedef void tg_entity_update_fn(tg_entity* p_entity, f32 delta_ms);
typedef void tg_entity_render_fn(tg_entity* p_entity, tg_camera_h h_camera);

typedef struct tg_entity
{
	u32                     flags;

	tg_entity_update_fn*    p_update_fn;
	tg_entity_render_fn*    p_render_fn;
	void*                   p_data;
} tg_entity;

#endif
