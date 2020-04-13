#include "graphics/vulkan/tg_graphics_vulkan_shader.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"
#include "tg_application.h"
#include "util/tg_file_io.h"



tg_vertex_shader_h tg_graphics_vertex_shader_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_vertex_shader_h vertex_shader_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*vertex_shader_h));
    tg_graphics_vulkan_shader_module_create(p_filename, &vertex_shader_h->shader_module);
    return vertex_shader_h;
}

void tg_graphics_vertex_shader_destroy(tg_vertex_shader_h vertex_shader_h)
{
    TG_ASSERT(vertex_shader_h);

    tg_graphics_vulkan_shader_module_destroy(vertex_shader_h->shader_module);
    TG_MEMORY_ALLOCATOR_FREE(vertex_shader_h);
}

tg_fragment_shader_h tg_graphics_fragment_shader_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_fragment_shader_h fragment_shader_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*fragment_shader_h));
    tg_graphics_vulkan_shader_module_create(p_filename, &fragment_shader_h->shader_module);
    return fragment_shader_h;
}

void tg_graphics_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h)
{
    TG_ASSERT(fragment_shader_h);

    tg_graphics_vulkan_shader_module_destroy(fragment_shader_h->shader_module);
    TG_MEMORY_ALLOCATOR_FREE(fragment_shader_h);
}

#endif
