#include "tg_scene.h"

#include "memory/tg_memory_allocator.h"
#include "tg_entity.h"
#include "util/tg_list.h"



typedef struct tg_scene
{
	tg_list_h    entities;
	u32          next_entity_id;
} tg_scene;



/*------------------------------------------------------------+
| Internals                                                   |
+------------------------------------------------------------*/

// TODO: there are better hash functions
u32 tg_graphics_scene_internal_u32_hash(const u32* p_v) // Knuth's Multiplicative Method
{
	TG_ASSERT(p_v);

	const u32 result = *p_v * 2654435761;
	return result;
}

b32 tg_graphics_scene_internal_u32_equals(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const u32* p_v0_u32 = (u32*)p_v0;
	const u32* p_v1_u32 = (u32*)p_v1;
	return *p_v0_u32 == *p_v1_u32;
}



/*------------------------------------------------------------+
| Main                                                        |
+------------------------------------------------------------*/

void tg_scene_create(tg_camera_h camera_h, tg_scene_h* p_scene_h)
{
	TG_ASSERT(p_scene_h);

	*p_scene_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_scene_h));

	(**p_scene_h).next_entity_id = 0;
	(**p_scene_h).entities = tg_list_create_capacity(tg_entity_h, 256);
}

void tg_scene_destroy(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_list_destroy(scene_h->entities);
	TG_MEMORY_ALLOCATOR_FREE(scene_h);
}



void tg_scene_deregister_entity(tg_scene_h scene_h, tg_entity_h entity_h)
{
	TG_ASSERT(scene_h && entity_h && tg_list_contains(scene_h->entities, &entity_h));

	tg_list_remove(scene_h->entities, &entity_h);
}

void tg_scene_register_entity(tg_scene_h scene_h, tg_entity_h entity_h)
{
	TG_ASSERT(scene_h && entity_h && !tg_list_contains(scene_h->entities, &entity_h));

	tg_list_insert(scene_h->entities, &entity_h);
}
