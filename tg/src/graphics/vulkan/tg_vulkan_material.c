#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN



static tg_material_h tg__create(tgvk_material_type material_type, tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    tg_material_h h_material = tgvk_handle_take(TG_STRUCTURE_TYPE_MATERIAL);
    h_material->material_type = material_type;
    h_material->h_vertex_shader = h_vertex_shader;
    h_material->h_fragment_shader = h_fragment_shader;

    tgvk_graphics_pipeline_create_info graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.p_vertex_shader = &h_vertex_shader->shader;
    graphics_pipeline_create_info.p_fragment_shader = &h_fragment_shader->shader;
    graphics_pipeline_create_info.cull_mode = VK_CULL_MODE_BACK_BIT;
    graphics_pipeline_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
    graphics_pipeline_create_info.depth_test_enable = VK_TRUE;
    graphics_pipeline_create_info.depth_write_enable = VK_TRUE;

    switch (material_type)
    {
    case TGVK_MATERIAL_TYPE_DEFERRED:
    {
        graphics_pipeline_create_info.blend_enable = VK_FALSE;
        graphics_pipeline_create_info.render_pass = shared_render_resources.geometry_render_pass;
    } break;
    case TGVK_MATERIAL_TYPE_FORWARD:
    {
        graphics_pipeline_create_info.blend_enable = VK_TRUE;
        graphics_pipeline_create_info.render_pass = shared_render_resources.forward_render_pass;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    graphics_pipeline_create_info.viewport_size.x = (f32)swapchain_extent.width;
    graphics_pipeline_create_info.viewport_size.y = (f32)swapchain_extent.height;
    graphics_pipeline_create_info.polygon_mode = VK_POLYGON_MODE_FILL;

    h_material->pipeline = tgvk_pipeline_create_graphics(&graphics_pipeline_create_info);

    return h_material;
}



tg_material_h tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg__create(TGVK_MATERIAL_TYPE_DEFERRED, h_vertex_shader, h_fragment_shader);
    return h_material;
}

tg_material_h tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg__create(TGVK_MATERIAL_TYPE_FORWARD, h_vertex_shader, h_fragment_shader);
    return h_material;
}

void tg_material_destroy(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    tgvk_pipeline_destroy(&h_material->pipeline);
    tgvk_handle_release(h_material);
}

b32 tg_material_is_deferred(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TGVK_MATERIAL_TYPE_DEFERRED;
    return result;
}

b32 tg_material_is_forward(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TGVK_MATERIAL_TYPE_FORWARD;
    return result;
}

#endif
