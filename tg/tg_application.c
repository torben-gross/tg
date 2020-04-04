#include "tg/tg_application.h"

#define ASSET_PATH "assets"

#include "tg/graphics/tg_graphics.h"
#include "tg/memory/tg_allocator.h"
#include "tg/platform/tg_platform.h"
#include "tg/tg_input.h"
#include "tg/util/tg_string.h"
#include "tg/util/tg_timer.h"

b32 running = TG_TRUE;
const char* asset_path = ASSET_PATH; // TODO: determine this some other way

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

    tg_image_h img = NULL;
    tg_graphics_image_create("test_icon.bmp", &img);

    tg_camera_info camera_info = { 0 };
    tg_input_get_mouse_position(&camera_info.last_mouse_x, &camera_info.last_mouse_y);
    {
        camera_info.position = (tgm_vec3f){ 0.0f, 0.0f, 0.0f };
        camera_info.pitch = 0.0f;
        camera_info.yaw = 0.0f;
        camera_info.roll = 0.0f;
        const tgm_mat4f camera_rotation = tgm_m4f_euler(TGM_TO_RADIANS(-camera_info.pitch), TGM_TO_RADIANS(-camera_info.yaw), TGM_TO_RADIANS(-camera_info.roll));
        const tgm_vec3f negated_camera_position = tgm_v3f_negated(&camera_info.position);
        const tgm_mat4f camera_translation = tgm_m4f_translate(&negated_camera_position);
        camera_info.camera.view = tgm_m4f_multiply_m4f(&camera_rotation, &camera_translation);

        camera_info.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
        camera_info.aspect = tg_platform_get_window_aspect_ratio(&camera_info.aspect);
        camera_info.near = -0.1f;
        camera_info.far = -1000.0f;
        camera_info.camera.projection = tgm_m4f_perspective(camera_info.fov_y_in_radians, camera_info.aspect, camera_info.near, camera_info.far);
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

    tg_graphics_mesh_create(4, positions, normals, uvs, NULL, 6, indices, &mesh_h);
    tg_graphics_vertex_shader_create("shaders/geometry.vert.spv", &vertex_shader_h);
    tg_graphics_fragment_shader_create("shaders/geometry.frag.spv", &fragment_shader_h);
    tg_graphics_material_create(vertex_shader_h, fragment_shader_h, &material_h);
    tg_graphics_model_create(mesh_h, material_h, &model_h);

    const tgm_vec3f positions2[3] = {
        { -5.0f, -3.0f, 0.0f },
        { -2.0f, -3.0f, 0.0f },
        { -3.5f,  2.0f, 0.0f }
    };
    const tgm_vec2f uvs2[3] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.5f, 1.0f }
    };
    const tgm_vec3f tangents[3] = {
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    };

    tg_mesh_h mesh2_h = NULL;
    tg_material_h material2_h = NULL;
    tg_model_h model2_h = NULL;

    tg_graphics_mesh_create(3, positions2, NULL, uvs2, tangents, 0, NULL, &mesh2_h);
    tg_graphics_material_create(vertex_shader_h, fragment_shader_h, &material2_h);
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
            if (debug_info.fps < 60)
            {
                TG_DEBUG_PRINT("Low framerate!");
            }

            tg_string_format(sizeof(debug_info.buffer), debug_info.buffer, "%d ms", debug_info.ms_sum / debug_info.fps);
            TG_DEBUG_PRINT(debug_info.buffer);

            tg_string_format(sizeof(debug_info.buffer), debug_info.buffer, "%u fps", debug_info.fps);
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
        const tgm_mat4f camera_rotation = tgm_m4f_euler(TGM_TO_RADIANS(camera_info.pitch), TGM_TO_RADIANS(camera_info.yaw), TGM_TO_RADIANS(camera_info.roll));

        tgm_vec4f right = { 1.0f, 0.0f, 0.0f, 0.0f };
        tgm_vec4f up = { 0.0f, 1.0f, 0.0f, 0.0f };
        tgm_vec4f forward = { 0.0f, 0.0f, -1.0f, 0.0f };

        right = tgm_m4f_multiply_v4f(&camera_rotation, &right);
        forward = tgm_m4f_multiply_v4f(&camera_rotation, &forward);

        tgm_vec3f temp;
        tgm_vec3f velocity = { 0 };
        if (tg_input_is_key_down(TG_KEY_W))
        {
            temp = tgm_v4f_to_v3f(&forward);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_A))
        {
            temp = tgm_v4f_to_v3f(&right);
            temp = tgm_v3f_multiply_f(&temp, -1.0f);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_S))
        {
            temp = tgm_v4f_to_v3f(&forward);
            temp = tgm_v3f_multiply_f(&temp, -1.0f);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_D))
        {
            temp = tgm_v4f_to_v3f(&right);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_SPACE))
        {
            temp = tgm_v4f_to_v3f(&up);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_CONTROL))
        {
            temp = tgm_v4f_to_v3f(&up);
            temp = tgm_v3f_multiply_f(&temp, -1.0f);
            velocity = tgm_v3f_add_v3f(&velocity, &temp);
        }

        if (tgm_v3f_magnitude_squared(&velocity) != 0.0f)
        {
            const f32 camera_base_speed = tg_input_is_key_down(TG_KEY_SHIFT) ? 0.02f : 0.01f;
            const f32 camera_speed = camera_base_speed * delta_ms;
            velocity = tgm_v3f_normalized(&velocity);
            velocity = tgm_v3f_multiply_f(&velocity, camera_speed);
            camera_info.position = tgm_v3f_add_v3f(&camera_info.position, &velocity);
        }

        const tgm_vec3f negated_camera_position = tgm_v3f_negated(&camera_info.position);
        const tgm_mat4f camera_translation = tgm_m4f_translate(&negated_camera_position);
        const tgm_mat4f inverse_camera_rotation = tgm_m4f_euler(TGM_TO_RADIANS(-camera_info.pitch), TGM_TO_RADIANS(-camera_info.yaw), TGM_TO_RADIANS(-camera_info.roll));
        camera_info.camera.view = tgm_m4f_multiply_m4f(&inverse_camera_rotation, &camera_translation);
        camera_info.last_mouse_x = mouse_x;
        camera_info.last_mouse_y = mouse_y;

        if (tg_input_get_mouse_wheel_detents(TG_FALSE))
        {
            camera_info.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
            camera_info.camera.projection = tgm_m4f_perspective(camera_info.fov_y_in_radians, camera_info.aspect, camera_info.near, camera_info.far);
        }
    }
    tg_timer_destroy(timer_h);

    tg_graphics_model_destroy(model2_h);
    tg_graphics_material_destroy(material2_h);
    tg_graphics_mesh_destroy(mesh2_h);

    tg_graphics_model_destroy(model_h);
    tg_graphics_material_destroy(material_h);
    tg_graphics_fragment_shader_destroy(fragment_shader_h);
    tg_graphics_vertex_shader_destroy(vertex_shader_h);
    tg_graphics_mesh_destroy(mesh_h);

    tg_graphics_image_destroy(img);

    tg_graphics_renderer_3d_shutdown();
    tg_graphics_shutdown();
}

void tg_application_quit()
{
	running = TG_FALSE;
}

const char* tg_application_get_asset_path()
{
    return asset_path;
}
