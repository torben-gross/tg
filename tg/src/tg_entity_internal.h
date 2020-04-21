#ifndef TG_ENTITY_INTERNAL_H
#define TG_ENTITY_INTERNAL_H

#include "tg_entity.h"



typedef struct tg_entity_uniform_buffer
{
	m4    model_matrix;
} tg_entity_uniform_buffer;

typedef struct tg_entity
{
	u32                              id;
	tg_model_h                       model_h;
	struct
	{
		v3                           position;
		m4                           position_matrix;

		tg_uniform_buffer_h          uniform_buffer_h;
		tg_entity_uniform_buffer*    p_uniform_buffer_data;
	} transform;
} tg_entity;



#endif