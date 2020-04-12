#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_graphics_vulkan_shader.h"
#include "memory/tg_memory_allocator.h"

tg_material_h tg_graphics_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h)
{
	TG_ASSERT(vertex_shader_h && fragment_shader_h);

	tg_material_h material_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*material_h));

    material_h->vertex_shader = vertex_shader_h;
    material_h->fragment_shader = fragment_shader_h;

    return material_h;
}

void tg_graphics_material_destroy(tg_material_h material_h)
{
    TG_ASSERT(material_h);

	TG_MEMORY_ALLOCATOR_FREE(material_h);
}

#endif
