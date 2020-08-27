#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN



static tg_material_h tg__create(tg_vulkan_material_type vulkan_material_type, tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    tg_material_h h_material = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_materials, h_material);

    h_material->type = TG_STRUCTURE_TYPE_MATERIAL;
    h_material->material_type = vulkan_material_type;
    h_material->h_vertex_shader = h_vertex_shader;
    h_material->h_fragment_shader = h_fragment_shader;

    return h_material;
}



tg_material_h tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg__create(TG_VULKAN_MATERIAL_TYPE_DEFERRED, h_vertex_shader, h_fragment_shader);
    return h_material;
}

tg_material_h tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg__create(TG_VULKAN_MATERIAL_TYPE_FORWARD, h_vertex_shader, h_fragment_shader);
    return h_material;
}

void tg_material_destroy(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    TG_VULKAN_RELEASE_HANDLE(h_material);
}

b32 tg_material_is_deferred(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED;
    return result;
}

b32 tg_material_is_forward(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD;
    return result;
}

#endif
