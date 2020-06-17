#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "math/tg_math.h"
#include "tg_common.h"



TG_DECLARE_TYPE(tg_scene);

TG_DECLARE_HANDLE(tg_entity_graphics_data_ptr);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);



typedef enum tg_entity_flags
{
	TG_ENTITY_FLAG_INITIALIZED    = (1 << 0)
} tg_entity_flags;

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
	tg_entity_graphics_data_ptr_h    h_graphics_data_ptr;
} tg_entity;



tg_entity    tg_entity_create(tg_mesh_h h_mesh, tg_material_h h_material);
void         tg_entity_destroy(tg_entity *p_entity);
void         tg_entity_set_mesh(tg_entity* p_entity, tg_mesh_h h_mesh, u8 lod);
void         tg_entity_set_position(tg_entity* p_entity, v3 position);

#endif
