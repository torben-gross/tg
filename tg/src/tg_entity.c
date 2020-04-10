#include "tg_entity.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory_allocator.h"
#include "tg_scene.h"



typedef struct tg_entity
{
	u32           id;
	tg_scene_h    scene_h;
	tg_model_h    model_h;
} tg_entity;



u32 entity_next_id = 1;



void tg_entity_create(tg_entity_h* p_entity_h)
{
	TG_ASSERT(p_entity_h);

	*p_entity_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_entity_h));
	(**p_entity_h).id = entity_next_id++;
	(**p_entity_h).scene_h = TG_NULL;
	(**p_entity_h).model_h = TG_NULL;
}

void tg_entity_destroy(tg_entity_h entity_h)
{
	TG_ASSERT(entity_h);

	TG_MEMORY_ALLOCATOR_FREE(entity_h);
}



void tg_entity_set_model(tg_entity_h entity_h, tg_model_h model_h)
{
	TG_ASSERT(entity_h && model_h);

	entity_h->model_h = model_h;
}
