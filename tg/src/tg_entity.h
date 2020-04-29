#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "math/tg_math.h"
#include "tg_common.h"



TG_DECLARE_HANDLE(tg_entity);
TG_DECLARE_HANDLE(tg_entity_graphics_data_ptr);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_scene);



typedef struct tg_entity
{
	u32                              id;
	u32                              flags;
	tg_mesh_h                        mesh_h;
	tg_material_h                    material_h;
	struct
	{
		v3                           position;
		m4                           position_matrix;
		m4                           model_matrix;
	} transform;
	tg_entity_graphics_data_ptr_h    graphics_data_ptr_h;
} tg_entity;



tg_entity_h    tg_entity_create(tg_scene_h scene_h, tg_mesh_h mesh_h, tg_material_h material_h);
void           tg_entity_destroy(tg_entity_h entity_h);
void           tg_entity_set_position(tg_entity_h entity_h, const v3* p_position);

#endif
