#include "tg_application.h"

#include "tg/graphics/tg_graphics.h"
#include "tg/platform/tg_platform.h"
#include "tg/tg_input.h"
#include "tg/util/tg_timer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool running = true;

typedef struct tg_camera_info
{
    tg_camera    camera;
    tgm_vec3f    position;
    f32          pitch;
    f32          yaw;
    f32          roll;
    f32          fov_y_in_radians;
    f32          aspect;
    f32          near;
    f32          far;
    ui32         last_mouse_x;
    ui32         last_mouse_y;
} tg_camera_info;

#ifdef TG_DEBUG
typedef struct tg_debug_info
{
    f32     ms_sum;
    ui32    fps;
    char    buffer[256];
} tg_debug_info;
#endif

void tg_application_start()
{
    tg_graphics_init();

    tg_camera_info camera_info = { 0 };
    tg_input_get_mouse_position(&camera_info.last_mouse_x, &camera_info.last_mouse_y);
    {
        camera_info.position = (tgm_vec3f){ 0.0f, 0.0f, 0.0f };
        camera_info.pitch = 0.0f;
        camera_info.yaw = 0.0f;
        camera_info.roll = 0.0f;
        tgm_mat4f camera_rotation = *tgm_m4f_euler(&camera_rotation, TGM_TO_RADIANS(-camera_info.pitch), TGM_TO_RADIANS(-camera_info.yaw), TGM_TO_RADIANS(-camera_info.roll));
        tgm_vec3f negated_camera_position = *tgm_v3f_negate(&negated_camera_position, &camera_info.position);
        tgm_mat4f camera_translation = *tgm_m4f_translate(&camera_translation, &negated_camera_position);
        tgm_m4f_multiply_m4f(&camera_info.camera.view, &camera_rotation, &camera_translation);

        camera_info.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
        camera_info.aspect = tg_platform_get_window_aspect_ratio(&camera_info.aspect);
        camera_info.near = -0.1f;
        camera_info.far = -1000.0f;
        tgm_m4f_perspective(&camera_info.camera.projection, camera_info.fov_y_in_radians, camera_info.aspect, camera_info.near, camera_info.far);
    }

    tg_graphics_renderer_3d_init(&camera_info.camera);

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

#ifdef TG_DEBUG
    tg_debug_info debug_info = { 0 };
#endif

    tg_timer_h timer_h = NULL;
    tg_timer_create(&timer_h);
    tg_timer_start(timer_h);
    while (running)
    {
        tg_timer_stop(timer_h);
        const f32 delta_ms = tg_timer_elapsed_milliseconds(timer_h);
        tg_timer_reset(timer_h);
        tg_timer_start(timer_h);

#ifdef TG_DEBUG
        debug_info.ms_sum += delta_ms;
        debug_info.fps++;
        if (debug_info.ms_sum > 1000.0f)
        {
            snprintf(debug_info.buffer, sizeof(debug_info.buffer), "%f ms", debug_info.ms_sum / debug_info.fps);
            TG_DEBUG_PRINT(debug_info.buffer);

            snprintf(debug_info.buffer, sizeof(debug_info.buffer), "%lu fps", debug_info.fps);
            TG_DEBUG_PRINT(debug_info.buffer);

            debug_info.ms_sum = 0.0f;
            debug_info.fps = 0;
        }
#endif

        tg_input_clear();
        tg_platform_handle_events();
        tg_graphics_renderer_3d_draw(model_h);
        tg_graphics_renderer_3d_draw(model2_h);
        tg_graphics_renderer_3d_present();

        ui32 mouse_x;
        ui32 mouse_y;
        tg_input_get_mouse_position(&mouse_x, &mouse_y);
        if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
        {
            camera_info.yaw += 0.064f * (f32)((i32)camera_info.last_mouse_x - (i32)mouse_x);
            camera_info.pitch += 0.064f * (f32)((i32)camera_info.last_mouse_y - (i32)mouse_y);
        }
        tgm_mat4f camera_rotation = *tgm_m4f_euler(&camera_rotation, TGM_TO_RADIANS(camera_info.pitch), TGM_TO_RADIANS(camera_info.yaw), TGM_TO_RADIANS(camera_info.roll));

        tgm_vec4f right = { 1.0f, 0.0f, 0.0f, 0.0f };
        tgm_vec4f up = { 0.0f, 1.0f, 0.0f, 0.0f };
        tgm_vec4f forward = { 0.0f, 0.0f, -1.0f, 0.0f };

        tgm_m4f_multiply_v4f(&right, &camera_rotation, &right);
        tgm_m4f_multiply_v4f(&forward, &camera_rotation, &forward);

        tgm_vec3f temp;
        tgm_vec3f velocity = { 0 };
        const f32 camera_speed = 0.01f * delta_ms;
        if (tg_input_is_key_down(TG_KEY_W))
        {
            tgm_v4f_to_v3f(&temp, &forward);
            tgm_v3f_multiply_f(&temp, &temp, camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_A))
        {
            tgm_v4f_to_v3f(&temp, &right);
            tgm_v3f_multiply_f(&temp, &temp, -camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_S))
        {
            tgm_v4f_to_v3f(&temp, &forward);
            tgm_v3f_multiply_f(&temp, &temp, -camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_D))
        {
            tgm_v4f_to_v3f(&temp, &right);
            tgm_v3f_multiply_f(&temp, &temp, camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_SPACE))
        {
            tgm_v4f_to_v3f(&temp, &up);
            tgm_v3f_multiply_f(&temp, &temp, camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_CONTROL))
        {
            tgm_v4f_to_v3f(&temp, &up);
            tgm_v3f_multiply_f(&temp, &temp, -camera_speed);
            tgm_v3f_add_v3f(&velocity, &velocity, &temp);
        }
        tgm_v3f_add_v3f(&camera_info.position, &camera_info.position, &velocity);

        tgm_vec3f negated_camera_position = *tgm_v3f_negate(&negated_camera_position, &camera_info.position);
        tgm_mat4f camera_translation = *tgm_m4f_translate(&camera_translation, &negated_camera_position);
        tgm_mat4f inverse_camera_rotation = *tgm_m4f_euler(&inverse_camera_rotation, TGM_TO_RADIANS(-camera_info.pitch), TGM_TO_RADIANS(-camera_info.yaw), TGM_TO_RADIANS(-camera_info.roll));
        tgm_m4f_multiply_m4f(&camera_info.camera.view, &inverse_camera_rotation, &camera_translation);
        camera_info.last_mouse_x = mouse_x;
        camera_info.last_mouse_y = mouse_y;
    }
    tg_timer_destroy(timer_h);

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
