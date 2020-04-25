#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

m4 identity;

tg_model_h tgg_model_create(tg_mesh_h mesh_h, tg_material_h material_h)
{
	TG_ASSERT(mesh_h && material_h);

	tg_model_h model_h = TG_MEMORY_ALLOC(sizeof(*model_h));

    model_h->mesh_h = mesh_h;
    model_h->material_h = material_h;

    return model_h;
}

void tgg_model_destroy(tg_model_h model_h)
{
	TG_ASSERT(model_h);

	TG_MEMORY_FREE(model_h);
}

#endif
