#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "math/tg_math.h"
#include "tg_common.h"



TG_DECLARE_TYPE(tg_scene);

TG_DECLARE_HANDLE(tg_entity_graphics_data_ptr);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);



typedef struct tg_entity
{
	u32                              id;
	u32                              flags;
	struct
	{
		v3                           position;
		m4                           position_matrix;
		m4                           model_matrix;
	} transform;
	tg_entity_graphics_data_ptr_h    graphics_data_ptr_h;
} tg_entity;



tg_entity    tg_entity_create(tg_mesh_h mesh_h, tg_material_h material_h);
void         tg_entity_destroy(tg_entity *p_entity);
void         tg_entity_set_position(tg_entity* p_entity, const v3* p_position);

#endif
