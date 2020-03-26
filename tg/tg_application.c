#include "tg_application.h"

#include "tg/graphics/tg_graphics.h"
#include "tg/platform/tg_platform.h"
#include "tg/tg_input.h"
#include <stdbool.h>
#include <stdlib.h>

bool running = true;

void tg_application_start()
{
    tg_graphics_init();
    tg_graphics_renderer_3d_init();

    const tgm_vec3f positions[4] = {
        { -1.5f, -1.5f, 0.0f },
        {  1.5f, -1.5f, 0.0f },
        {  1.5f,  1.5f, 0.0f },
        { -1.5f,  1.5f, 0.0f }
    };
    const tgm_vec3f normals[4] = {
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f }
    };
    const tgm_vec2f uvs[4] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };
    const ui16 indices[6] = {
        0, 1, 2, 2, 3, 0
    };

    tg_mesh_h mesh_h = NULL;
    tg_vertex_shader_h vertex_shader_h = NULL;
    tg_fragment_shader_h fragment_shader_h = NULL;
    tg_material_h material_h = NULL;
    tg_model_h model_h = NULL;

    tg_graphics_mesh_create(4, positions, normals, uvs, 6, indices, &mesh_h);
    tg_graphics_vertex_shader_create("shaders/geometry_vert.spv", &vertex_shader_h);
    tg_graphics_fragment_shader_create("shaders/geometry_frag.spv", &fragment_shader_h);
    tg_graphics_material_create(vertex_shader_h, fragment_shader_h, &material_h);
    tg_graphics_model_create(mesh_h, material_h, &model_h);

    const tgm_vec3f positions2[3] = {
        { -5.0f, -3.0f, 0.0f },
        { -2.0f, -3.0f, 0.0f },
        { -3.5f,  2.0f, 0.0f }
    };
    const tgm_vec3f normals2[3] = {
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f }
    };
    const tgm_vec2f uvs2[3] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.5f, 1.0f }
    };

    tg_mesh_h mesh2_h = NULL;
    tg_vertex_shader_h vertex_shader2_h = NULL;
    tg_fragment_shader_h fragment_shader2_h = NULL;
    tg_material_h material2_h = NULL;
    tg_model_h model2_h = NULL;

    tg_graphics_mesh_create(3, positions2, normals2, uvs2, 0, NULL, &mesh2_h);
    tg_graphics_vertex_shader_create("shaders/geometry_vert.spv", &vertex_shader2_h);
    tg_graphics_fragment_shader_create("shaders/geometry_frag.spv", &fragment_shader2_h);
    tg_graphics_material_create(vertex_shader2_h, fragment_shader2_h, &material2_h);
    tg_graphics_model_create(mesh2_h, material2_h, &model2_h);

    while (running)
    {
        tg_input_clear();
        tg_platform_handle_events();
        tg_graphics_renderer_3d_draw(model_h);
        tg_graphics_renderer_3d_draw(model2_h);
        tg_graphics_renderer_3d_present();
        TG_DEBUG_PRINT_PERFORMANCE();
        if (tg_input_is_key_pressed(TG_KEY_SPACE, true))
        {
            TG_DEBUG_PRINT("SPACE IS PRESSED!");
        }
        if (tg_input_is_key_pressed(TG_KEY_SPACE, false))
        {
            TG_DEBUG_PRINT("SPACE IS CONSUMED!");
        }
    }

    tg_graphics_model_destroy(model2_h);
    tg_graphics_material_destroy(material2_h);
    tg_graphics_fragment_shader_destroy(fragment_shader2_h);
    tg_graphics_vertex_shader_destroy(vertex_shader2_h);
    tg_graphics_mesh_destroy(mesh2_h);

    tg_graphics_model_destroy(model_h);
    tg_graphics_material_destroy(material_h);
    tg_graphics_fragment_shader_destroy(fragment_shader_h);
    tg_graphics_vertex_shader_destroy(vertex_shader_h);
    tg_graphics_mesh_destroy(mesh_h);

    tg_graphics_renderer_3d_shutdown();
    tg_graphics_shutdown();
}

void tg_application_quit()
{
	running = false;
}
