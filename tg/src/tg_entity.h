#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"



typedef struct tg_entity_uniform_buffer
{
	m4    model_matrix;
} tg_entity_uniform_buffer;

typedef struct tg_entity
{
	u32                              id;
	u32                              flags;
	tg_model_h                       model_h;
	struct
	{
		v3                           position;
		m4                           position_matrix;
		m4                           model_matrix;
	} transform;
	tg_entity_graphics_data_ptr_h    graphics_data_ptr_h;
	//tg_entity_physics_data_ptr_h     physics_data_ptr_h; TODO
} tg_entity;



tg_entity_h    tg_entity_create(tg_renderer_3d_h renderer_3d_h, tg_model_h model_h);
void           tg_entity_destroy(tg_entity_h entity_h);
v3             tg_entity_get_position(tg_entity_h entity_h);
void           tg_entity_set_position(tg_entity_h entity_h, const v3* p_position);

#endif
