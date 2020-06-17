#include "tg_entity.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"

u32 entity_next_id = 0;

tg_entity tg_entity_create(tg_mesh_h h_mesh, tg_material_h h_material)
{
	tg_entity entity = { 0 };
	entity.id = entity_next_id++;
	entity.flags = TG_ENTITY_FLAG_INITIALIZED;
    entity.transform.position = (v3){ 0.0f, 0.0f, 0.0f };
    entity.transform.position_matrix = tgm_m4_identity();
	entity.h_graphics_data_ptr = tg_entity_graphics_data_ptr_create(h_mesh, h_material);

	return entity;
}

void tg_entity_destroy(tg_entity* p_entity)
{
	TG_ASSERT(p_entity);

	tg_entity_graphics_data_ptr_destroy(p_entity->h_graphics_data_ptr);
}

void tg_entity_set_mesh(tg_entity* p_entity, tg_mesh_h h_mesh, u8 lod)
{
	tg_entity_graphics_data_ptr_set_mesh(p_entity->h_graphics_data_ptr, h_mesh, lod);
}

void tg_entity_set_position(tg_entity* p_entity, v3 position)
{
	TG_ASSERT(p_entity);

	p_entity->transform.position = position;
	p_entity->transform.position_matrix = tgm_m4_translate(position);
	p_entity->transform.model_matrix = p_entity->transform.position_matrix; // TODO: rotation

	tg_entity_graphics_data_ptr_set_model_matrix(p_entity->h_graphics_data_ptr, &p_entity->transform.position_matrix);
}
