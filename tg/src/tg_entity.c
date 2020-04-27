#include "tg_entity.h"

#include "memory/tg_memory.h"



u32 entity_next_id = 0;



tg_entity_h tg_entity_create(tg_renderer_3d_h renderer_3d_h, tg_model_h model_h) // TODO: allocate model internally before and set model after creation?
{
	TG_ASSERT(renderer_3d_h && model_h);

	tg_entity_h entity_h = TG_MEMORY_ALLOC(sizeof(*entity_h));

	entity_h->id = entity_next_id++;
	entity_h->flags = 0;
	entity_h->model_h = model_h; // TODO: this call wil probably go?

    entity_h->transform.position = (v3){ 0.0f, 0.0f, 0.0f };
    entity_h->transform.position_matrix = tgm_m4_identity();

	entity_h->graphics_data_ptr_h = tgg_entity_graphics_data_ptr_create(entity_h, renderer_3d_h);

	return entity_h;
}

void tg_entity_destroy(tg_entity_h entity_h)
{
	TG_ASSERT(entity_h);

	tgg_entity_graphics_data_ptr_destroy(entity_h->graphics_data_ptr_h);
	TG_MEMORY_FREE(entity_h);
}

v3 tg_entity_get_position(tg_entity_h entity_h)
{
	TG_ASSERT(entity_h);

	return entity_h->transform.position;
}

void tg_entity_set_position(tg_entity_h entity_h, const v3* p_position)
{
	TG_ASSERT(entity_h && p_position);

	entity_h->transform.position = *p_position;
	entity_h->transform.position_matrix = tgm_m4_translate(p_position);
	entity_h->transform.model_matrix = entity_h->transform.position_matrix; // TODO: rotation

	tgg_entity_graphics_data_ptr_set_model_matrix(entity_h->graphics_data_ptr_h, &entity_h->transform.position_matrix);
}
