#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/memory/tg_allocator.h"
#include "tg/tg_application.h"
#include "tg/util/tg_file_io.h"

void tg_graphics_vertex_shader_create(const char* p_filename, tg_vertex_shader_h* p_vertex_shader_h)
{
    TG_ASSERT(p_filename && p_vertex_shader_h);

    *p_vertex_shader_h = TG_ALLOCATOR_ALLOCATE(sizeof(**p_vertex_shader_h));
    tg_graphics_vulkan_shader_module_create(p_filename, &(**p_vertex_shader_h).shader_module);
}

void tg_graphics_vertex_shader_destroy(tg_vertex_shader_h vertex_shader_h)
{
    TG_ASSERT(vertex_shader_h);

    tg_graphics_vulkan_shader_module_destroy(vertex_shader_h->shader_module);
    TG_ALLOCATOR_FREE(vertex_shader_h);
}

void tg_graphics_fragment_shader_create(const char* p_filename, tg_fragment_shader_h* p_fragment_shader_h)
{
    TG_ASSERT(p_filename && p_fragment_shader_h);

    *p_fragment_shader_h = TG_ALLOCATOR_ALLOCATE(sizeof(**p_fragment_shader_h));
    tg_graphics_vulkan_shader_module_create(p_filename, &(**p_fragment_shader_h).shader_module);
}

void tg_graphics_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h)
{
    TG_ASSERT(fragment_shader_h);

    tg_graphics_vulkan_shader_module_destroy(fragment_shader_h->shader_module);
    TG_ALLOCATOR_FREE(fragment_shader_h);
}

#endif
