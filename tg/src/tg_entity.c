#include "tg_entity.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"

u32 entity_next_id = 0;

tg_entity tg_entity_create(tg_mesh_h mesh_h, tg_material_h material_h)
{
	TG_ASSERT(mesh_h && material_h);

	tg_entity entity = { 0 };
	entity.id = entity_next_id++;
	entity.flags = 0;
    entity.transform.position = (v3){ 0.0f, 0.0f, 0.0f };
    entity.transform.position_matrix = tgm_m4_identity();
	entity.graphics_data_ptr_h = tg_entity_graphics_data_ptr_create(mesh_h, material_h);

	return entity;
}

void tg_entity_destroy(tg_entity* p_entity)
{
	TG_ASSERT(p_entity);

	tg_entity_graphics_data_ptr_destroy(p_entity->graphics_data_ptr_h);
}

void tg_entity_set_position(tg_entity* p_entity, const v3* p_position)
{
	TG_ASSERT(p_entity && p_position);

	p_entity->transform.position = *p_position;
	p_entity->transform.position_matrix = tgm_m4_translate(p_position);
	p_entity->transform.model_matrix = p_entity->transform.position_matrix; // TODO: rotation

	tg_entity_graphics_data_ptr_set_model_matrix(p_entity->graphics_data_ptr_h, &p_entity->transform.position_matrix);
}
