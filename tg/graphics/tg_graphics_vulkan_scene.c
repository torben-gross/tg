#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/memory/tg_memory_allocator.h"
#include "tg/util/tg_hashmap.h"
#include "tg/util/tg_list.h"



typedef struct tg_scene
{
	tg_hashmap_h     entity_id_to_index_hashmap;

	struct render_data
	{
		u32          next_instance_id;
		tg_list_h    positions;
		tg_list_h    models;
	};
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

void tg_graphics_scene_create(tg_camera_h camera_h, tg_scene_h* p_scene_h)
{
	TG_ASSERT(p_scene_h);

	*p_scene_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_scene_h));

	(**p_scene_h).next_instance_id = 0;
	tg_hashmap_create_count_capacity(u32, u32, 256, 4, tg_graphics_scene_internal_u32_hash, tg_graphics_scene_internal_u32_equals, &(**p_scene_h).entity_id_to_index_hashmap);
	tg_list_create_capacity(v3, 256, &(**p_scene_h).positions);
	tg_list_create_capacity(tg_model_h, 256, &(**p_scene_h).models);
}

void tg_graphics_scene_spawn_entity(tg_scene_h scene_h, const v3* p_position, const tg_model_h model_h, tg_entity_h* p_entity_h)
{
	TG_ASSERT(scene_h && p_position && model_h && p_entity_h);

	const u32 instance_id = scene_h->next_instance_id++;
	**(u32**)p_entity_h = instance_id;

	const u32 key = instance_id;
	const u32 value = tg_list_count(scene_h->positions);
	tg_hashmap_insert(scene_h->entity_id_to_index_hashmap, &key, &value);
	tg_list_insert(scene_h->positions, p_position);
	tg_list_insert(scene_h->models, &model_h);
}

void tg_graphics_scene_destroy(tg_scene_h scene_h)
{
	TG_ASSERT(scene_h);

	tg_list_destroy(scene_h->models);
	tg_list_destroy(scene_h->positions);
	tg_hashmap_destroy(scene_h->entity_id_to_index_hashmap);

	TG_MEMORY_ALLOCATOR_FREE(scene_h);
}

#endif
