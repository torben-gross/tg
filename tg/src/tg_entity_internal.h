#ifndef TG_ENTITY_INTERNAL_H
#define TG_ENTITY_INTERNAL_H

#include "tg_entity.h"



typedef struct tg_entity
{
	u32                 id;
	tg_renderer_3d_h    renderer_3d_h;
	tg_model_h          model_h;
	m4*                 p_model_matrix;
} tg_entity;



#endif