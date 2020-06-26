#ifndef TG_PLAYER_H
#define TG_PLAYER_H

#include "tg_entity.h"

typedef struct tg_entity_player_data
{
	tg_entity              entity;
	v3                     position;
	m4                     model_matrix;
	tg_uniform_buffer_h    h_model_uniform_buffer;
} tg_entity_player_data;

#endif
