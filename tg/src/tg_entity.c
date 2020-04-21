#include "tg_entity_internal.h"

#include "memory/tg_memory_allocator.h"



u32 entity_next_id = 0;



tg_entity_h tg_entity_create(tg_renderer_3d_h renderer_3d_h, tg_model_h model_h) // TODO: allocate model internally before and set after creation
{
	TG_ASSERT(renderer_3d_h && model_h);

	tg_entity_h entity_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*entity_h));

	entity_h->id = entity_next_id++;
	entity_h->renderer_3d_h = renderer_3d_h;
	entity_h->model_h = model_h; // TODO: this call wil probably go

	tgg_renderer_3d_register(renderer_3d_h, entity_h);

	return entity_h;
}

void tg_entity_destroy(tg_entity_h entity_h)
{
	TG_ASSERT(entity_h);

	TG_MEMORY_ALLOCATOR_FREE(entity_h);
}
