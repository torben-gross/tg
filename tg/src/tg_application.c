#include "tg_application.h"

#define ASSET_PATH "assets"



#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"
#include "tg_entity.h"
#include "tg_entity_internal.h"
#include "tg_input.h"
#include "util/tg_string.h"
#include "util/tg_timer.h"



typedef struct tg_camera_info
{
    tg_camera_h    camera_h;
    v3             position;
    f32            pitch;
    f32            yaw;
    f32            roll;
    f32            fov_y_in_radians;
    f32            aspect;
    f32            near;
    f32            far;
    u32            last_mouse_x;
    u32            last_mouse_y;
} tg_camera_info;

#ifdef TG_DEBUG
typedef struct tg_debug_info
{
    f32     ms_sum;
    u32     fps;
    char    buffer[256];
} tg_debug_info;
#endif



b32 running = TG_TRUE;
const char* asset_path = ASSET_PATH; // TODO: determine this some other way



void tg_application_start()
{
    tg_graphics_init();

    tg_camera_info camera_info = { 0 };
    {
        camera_info.position = (v3){ 0.0f, 0.0f, 0.0f };
        camera_info.pitch = 0.0f;
        camera_info.yaw = 0.0f;
        camera_info.roll = 0.0f;
        camera_info.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
        camera_info.aspect = tg_platform_get_window_aspect_ratio(&camera_info.aspect);
        camera_info.near = -0.1f;
        camera_info.far = -1000.0f;
        tg_input_get_mouse_position(&camera_info.last_mouse_x, &camera_info.last_mouse_y);
    }
    camera_info.camera_h = tg_graphics_camera_create(&camera_info.position, camera_info.pitch, camera_info.yaw, camera_info.roll, camera_info.fov_y_in_radians, camera_info.near, camera_info.far);

    tg_renderer_3d_h renderer_3d_h = tg_graphics_renderer_3d_create(camera_info.camera_h);

    const v3 p_quad_positions[4] = {
        { -1.5f, -1.5f, 0.0f },
        {  1.5f, -1.5f, 0.0f },
        {  1.5f,  1.5f, 0.0f },
        { -1.5f,  1.5f, 0.0f }
    };
    const v3 p_ground_positions[4] = {
        { -1000.0f, -2.0f,  1000.0f },
        {  1000.0f, -2.0f,  1000.0f },
        {  1000.0f, -2.0f, -1000.0f },
        { -1000.0f, -2.0f, -1000.0f }
    };
    const v2 p_uvs[4] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };
    const u16 p_indices[6] = {
        0, 1, 2, 2, 3, 0
    };

    tg_mesh_h mesh_h = tg_graphics_mesh_create(4, p_quad_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    tg_vertex_shader_h vertex_shader_h = tg_graphics_vertex_shader_create("shaders/geometry.vert.spv");
    tg_fragment_shader_h fragment_shader_h = tg_graphics_fragment_shader_create("shaders/geometry.frag.spv");
    tg_material_h material_h = tg_graphics_material_create(vertex_shader_h, fragment_shader_h);
    tg_model_h model_h = tg_graphics_model_create(mesh_h, material_h);
    tg_entity_h quad_entity_h = tg_entity_create(renderer_3d_h, model_h);

    tg_mesh_h ground_mesh_h = tg_graphics_mesh_create(4, p_ground_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    tg_material_h ground_material_h = tg_graphics_material_create(vertex_shader_h, fragment_shader_h);
    tg_model_h ground_model_h = tg_graphics_model_create(ground_mesh_h, ground_material_h);
    tg_entity_h ground_entity_h = tg_entity_create(renderer_3d_h, ground_model_h);

#ifdef TG_DEBUG
    tg_debug_info debug_info = { 0 };
#endif

    tg_timer_h timer_h = tg_timer_create();
    tg_timer_start(timer_h);



    /*--------------------------------------------------------+
    | Start main loop                                         |
    +--------------------------------------------------------*/

    f32 quad_offset = -9.0f; // TODO: temporary testing variable
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

        if (tg_input_is_key_down(TG_KEY_K))
        {
            quad_offset += 0.01f * delta_ms;
        }
        if (tg_input_is_key_down(TG_KEY_L))
        {
            quad_offset -= 0.01f * delta_ms;
        }
        v3 quad_offset_translation_z = { 0.0f, 0.0f, quad_offset };
        *quad_entity_h->p_model_matrix = tgm_m4_translate(&quad_offset_translation_z);

        u32 mouse_x;
        u32 mouse_y;
        tg_input_get_mouse_position(&mouse_x, &mouse_y);
        if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
        {
            camera_info.yaw += 0.064f * (f32)((i32)camera_info.last_mouse_x - (i32)mouse_x);
            camera_info.pitch += 0.064f * (f32)((i32)camera_info.last_mouse_y - (i32)mouse_y);
        }
        const m4 camera_rotation = tgm_m4_euler(TGM_TO_RADIANS(camera_info.pitch), TGM_TO_RADIANS(camera_info.yaw), TGM_TO_RADIANS(camera_info.roll));
        
        const v4 right = { camera_rotation.m00, camera_rotation.m10, camera_rotation.m20, camera_rotation.m30 };
        const v4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
        const v4 forward = { -camera_rotation.m02, -camera_rotation.m12, -camera_rotation.m22, -camera_rotation.m32 };

        v3 temp;
        v3 velocity = { 0 };
        if (tg_input_is_key_down(TG_KEY_W))
        {
            temp = tgm_v4_to_v3(&forward);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_A))
        {
            temp = tgm_v4_to_v3(&right);
            temp = tgm_v3_multiply_f(&temp, -1.0f);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_S))
        {
            temp = tgm_v4_to_v3(&forward);
            temp = tgm_v3_multiply_f(&temp, -1.0f);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_D))
        {
            temp = tgm_v4_to_v3(&right);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_SPACE))
        {
            temp = tgm_v4_to_v3(&up);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }
        if (tg_input_is_key_down(TG_KEY_CONTROL))
        {
            temp = tgm_v4_to_v3(&up);
            temp = tgm_v3_multiply_f(&temp, -1.0f);
            velocity = tgm_v3_add_v3(&velocity, &temp);
        }

        if (tgm_v3_magnitude_squared(&velocity) != 0.0f)
        {
            const f32 camera_base_speed = tg_input_is_key_down(TG_KEY_SHIFT) ? 0.02f : 0.01f;
            const f32 camera_speed = camera_base_speed * delta_ms;
            velocity = tgm_v3_normalized(&velocity);
            velocity = tgm_v3_multiply_f(&velocity, camera_speed);
            camera_info.position = tgm_v3_add_v3(&camera_info.position, &velocity);
        }

        tg_graphics_camera_set_view(&camera_info.position, TGM_TO_RADIANS(camera_info.pitch), TGM_TO_RADIANS(camera_info.yaw), TGM_TO_RADIANS(camera_info.roll), camera_info.camera_h);
        camera_info.last_mouse_x = mouse_x;
        camera_info.last_mouse_y = mouse_y;

        if (tg_input_get_mouse_wheel_detents(TG_FALSE))
        {
            camera_info.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
            tg_graphics_camera_set_projection(camera_info.fov_y_in_radians, camera_info.near, camera_info.far, camera_info.camera_h);
        }

        tg_graphics_renderer_3d_draw(renderer_3d_h);
        tg_graphics_renderer_3d_present(renderer_3d_h);
    }

    /*--------------------------------------------------------+
    | End main loop                                           |
    +--------------------------------------------------------*/



    tg_timer_destroy(timer_h);

    tg_entity_destroy(quad_entity_h);
    tg_entity_destroy(ground_entity_h);

    tg_graphics_model_destroy(ground_model_h);
    tg_graphics_material_destroy(ground_material_h);
    tg_graphics_mesh_destroy(ground_mesh_h);

    tg_graphics_model_destroy(model_h);
    tg_graphics_material_destroy(material_h);
    tg_graphics_fragment_shader_destroy(fragment_shader_h);
    tg_graphics_vertex_shader_destroy(vertex_shader_h);
    tg_graphics_mesh_destroy(mesh_h);

    tg_graphics_camera_destroy(camera_info.camera_h);
    tg_graphics_renderer_3d_shutdown(renderer_3d_h);
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
