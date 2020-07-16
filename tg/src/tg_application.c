#include "tg_application.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"
#include "tg_input.h"
#include "tg_marching_cubes.h"
#include "tg_transvoxel_terrain.h"
#include "util/tg_list.h"
#include "util/tg_string.h"



#ifdef TG_DEBUG
typedef struct tg_debug_info
{
    f32     ms_sum;
    u32     fps;
} tg_debug_info;
#endif

typedef struct tg_pbr_sphere
{
    tg_uniform_buffer_h     h_ubo;
    tg_material_h           h_material;
    tg_render_command_h     h_render_command;
} tg_pbr_sphere;

typedef struct tg_pbr_material
{
    v4     albedo;
    f32    metallic;
    f32    roughness;
    f32    ao;
} tg_pbr_material;

typedef struct tg_sample_scene
{
    tg_list                     render_commands;
    tg_camera                   camera;
    tg_renderer_h               h_main_renderer;
    tg_renderer_h               h_secondary_renderer;
    u32                         last_mouse_x;
    u32                         last_mouse_y;
    struct quad
    {
        tg_mesh_h               h_quad_mesh;
        tg_uniform_buffer_h     h_quad_color_ubo;
        tg_material_h           h_quad_material;
        tg_render_command_h     h_quad_render_command;
        f32                     quad_offset_z;
        f32                     quad_delta_time_sum_looped;
    };
    tg_mesh_h                   h_pbr_sphere_mesh;
    tg_pbr_sphere               p_pbr_spheres[49];
    tg_terrain                  terrain;
    f32                         light_timer;
} tg_sample_scene;



b32 running = TG_TRUE;
tg_sample_scene sample_scene = { 0 };



static void tg__game_3d_create()
{
    sample_scene.render_commands = TG_LIST_CREATE(tg_render_command_h);
    sample_scene.camera.type = TG_CAMERA_TYPE_PERSPECTIVE;
    sample_scene.camera.position = (v3) { 128.0f, 150.0f, 128.0f };
    sample_scene.camera.pitch = 0.0f;
    sample_scene.camera.yaw = 0.0f;
    sample_scene.camera.roll = 0.0f;
    sample_scene.camera.perspective.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
    sample_scene.camera.perspective.aspect = tg_platform_get_window_aspect_ratio();
    sample_scene.camera.perspective.near = -0.1f;
    sample_scene.camera.perspective.far = -1000.0f;
    tg_input_get_mouse_position(&sample_scene.last_mouse_x, &sample_scene.last_mouse_y);
    sample_scene.h_main_renderer = tg_renderer_create(&sample_scene.camera);
    sample_scene.h_secondary_renderer = tg_renderer_create(&sample_scene.camera);
    tg_renderer_enable_shadows(sample_scene.h_main_renderer, TG_TRUE);
    tg_renderer_enable_shadows(sample_scene.h_secondary_renderer, TG_FALSE);

    tg_color_image_create_info color_image_create_info = { 0 };
    tg_platform_get_window_size(&color_image_create_info.width, &color_image_create_info.height);
    color_image_create_info.mip_levels = 1;
    color_image_create_info.format = TG_COLOR_IMAGE_FORMAT_B8G8R8A8;
    color_image_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
    color_image_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
    color_image_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_REPEAT;
    color_image_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_REPEAT;
    color_image_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_REPEAT;




    const v3 p_quad_positions[4] = {
        { -3.2f, -1.8f, 0.0f },
        {  3.2f, -1.8f, 0.0f },
        {  3.2f,  1.8f, 0.0f },
        { -3.2f,  1.8f, 0.0f }
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

    sample_scene.h_quad_mesh = tg_mesh_create(4, p_quad_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    sample_scene.h_quad_color_ubo = tg_uniform_buffer_create(sizeof(v3));
    *((v3*)tg_uniform_buffer_data(sample_scene.h_quad_color_ubo)) = (v3) { 1.0f, 0.0f, 0.0f };
    tg_handle p_custom_handles[2] = { sample_scene.h_quad_color_ubo, tg_renderer_get_render_target(sample_scene.h_secondary_renderer) };
    sample_scene.h_quad_material = tg_material_create_forward(tg_vertex_shader_get("shaders/forward.vert"), tg_fragment_shader_get("shaders/forward_custom.frag"));

    sample_scene.h_quad_render_command = tg_render_command_create(sample_scene.h_quad_mesh, sample_scene.h_quad_material, (v3) { 0.0f, 133.0f, 0.0f }, 2, p_custom_handles);
    sample_scene.quad_offset_z = -65.0f;
    tg_list_insert(&sample_scene.render_commands, &sample_scene.h_quad_render_command);



    tg_mesh_h h_sponza_mesh = tg_mesh_load("meshes/sponza.obj");

    tg_uniform_buffer_h h_sponza_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
    ((tg_pbr_material*)tg_uniform_buffer_data(h_sponza_ubo))->albedo = (v4) { 1.0f, 1.0f, 1.0f, 1.0f };
    ((tg_pbr_material*)tg_uniform_buffer_data(h_sponza_ubo))->metallic = 0.1f;
    ((tg_pbr_material*)tg_uniform_buffer_data(h_sponza_ubo))->roughness = 0.4f;
    ((tg_pbr_material*)tg_uniform_buffer_data(h_sponza_ubo))->ao = 1.0f;
    tg_material_h h_sponza_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred_pbr.vert"), tg_fragment_shader_get("shaders/deferred_pbr.frag"));

    tg_render_command_h h_sponza_render_command = tg_render_command_create(h_sponza_mesh, h_sponza_material, (v3) { 128.0f, 140.0f, 128.0f }, 1, & h_sponza_ubo);
    tg_list_insert(&sample_scene.render_commands, &h_sponza_render_command);

    sample_scene.terrain = tg_terrain_create(&sample_scene.camera);



    sample_scene.h_pbr_sphere_mesh = tg_mesh_create_sphere(0.5f, 128, 64);
    for (u32 y = 0; y < 7; y++)
    {
        for (u32 x = 0; x < 7; x++)
        {

            const u32 i = y * 7 + x;
            
            const v3 sphere_translation = { 128.0f - 7.0f + (f32)x * 2.0f, 143.0f + (f32)y * 2.0f, 112.0f };
            sample_scene.p_pbr_spheres[i].h_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
            ((tg_pbr_material*)tg_uniform_buffer_data(sample_scene.p_pbr_spheres[i].h_ubo))->albedo = (v4) { 1.0f, 1.0f, 1.0f, 1.0f };
            ((tg_pbr_material*)tg_uniform_buffer_data(sample_scene.p_pbr_spheres[i].h_ubo))->metallic = (f32)x / 6.0f;
            ((tg_pbr_material*)tg_uniform_buffer_data(sample_scene.p_pbr_spheres[i].h_ubo))->roughness = ((f32)y + 0.1f) / 6.5f;
            ((tg_pbr_material*)tg_uniform_buffer_data(sample_scene.p_pbr_spheres[i].h_ubo))->ao = 1.0f;
            sample_scene.p_pbr_spheres[i].h_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred_pbr.vert"), tg_fragment_shader_get("shaders/deferred_pbr.frag"));
            if (x == 6 && y == 0)
            {
                sample_scene.p_pbr_spheres[i].h_render_command = tg_render_command_create(tg_mesh_create_sphere(3.0f, 128, 64), sample_scene.p_pbr_spheres[i].h_material, sphere_translation, 1, &sample_scene.p_pbr_spheres[i].h_ubo);
            }
            else
            {
                sample_scene.p_pbr_spheres[i].h_render_command = tg_render_command_create(sample_scene.h_pbr_sphere_mesh, sample_scene.p_pbr_spheres[i].h_material, sphere_translation, 1, &sample_scene.p_pbr_spheres[i].h_ubo);
            }
            tg_list_insert(&sample_scene.render_commands, &sample_scene.p_pbr_spheres[i].h_render_command);
        }
    }

    // TODO: implement :)
    tg_renderer_bake_begin(sample_scene.h_main_renderer);
    tg_renderer_bake_push_probe(sample_scene.h_main_renderer, 128.0f, 145.0f, 128.0f);
    tg_renderer_bake_push_static(sample_scene.h_main_renderer, h_sponza_render_command);
    tg_renderer_bake_end(sample_scene.h_main_renderer);
}

static void tg__game_3d_update_and_render(f32 delta_ms)
{
    if (tg_input_is_key_down(TG_KEY_K))
    {
        sample_scene.quad_offset_z += 0.01f * delta_ms;
    }
    if (tg_input_is_key_down(TG_KEY_L))
    {
        sample_scene.quad_offset_z -= 0.01f * delta_ms;
    }
    v3 quad_offset_translation_z = { 0.0f, 133.0f, sample_scene.quad_offset_z };
    tg_render_command_set_position(sample_scene.h_quad_render_command, quad_offset_translation_z);

    sample_scene.quad_delta_time_sum_looped += delta_ms;
    if (sample_scene.quad_delta_time_sum_looped >= 10000.0f)
    {
        sample_scene.quad_delta_time_sum_looped -= 10000.0f;
    }
    const f32 sint = tgm_f32_sin(0.001f * sample_scene.quad_delta_time_sum_looped * 2.0f * (f32)TGM_PI);
    const f32 noise_x = tgm_noise(sint, 0.1f, 0.1f) + 0.5f;
    const f32 noise_y = tgm_noise(0.1f, sint, 0.1f) + 0.5f;
    const f32 noise_z = tgm_noise(0.1f, 0.1f, sint) + 0.5f;
    *((v3*)tg_uniform_buffer_data(sample_scene.h_quad_color_ubo)) = (v3) { noise_x, noise_y, noise_z };



    u32 mouse_x;
    u32 mouse_y;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        sample_scene.camera.yaw += TGM_TO_RADIANS(0.064f * (f32)((i32)sample_scene.last_mouse_x - (i32)mouse_x));
        sample_scene.camera.pitch += TGM_TO_RADIANS(0.064f * (f32)((i32)sample_scene.last_mouse_y - (i32)mouse_y));
    }
    const m4 camera_rotation = tgm_m4_euler(sample_scene.camera.pitch, sample_scene.camera.yaw, sample_scene.camera.roll);

    const v4 right = { camera_rotation.m00, camera_rotation.m10, camera_rotation.m20, camera_rotation.m30 }; // TODO: make these camera functions
    const v4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    const v4 forward = { -camera_rotation.m02, -camera_rotation.m12, -camera_rotation.m22, -camera_rotation.m32 };

    v3 velocity = { 0 };
    if (tg_input_is_key_down(TG_KEY_W))
    {
        velocity = tgm_v3_add(velocity, tgm_v4_to_v3(forward));
    }
    if (tg_input_is_key_down(TG_KEY_A))
    {
        velocity = tgm_v3_add(velocity, tgm_v3_mulf(tgm_v4_to_v3(right), -1.0f));
    }
    if (tg_input_is_key_down(TG_KEY_S))
    {
        velocity = tgm_v3_add(velocity, tgm_v3_mulf(tgm_v4_to_v3(forward), -1.0f));
    }
    if (tg_input_is_key_down(TG_KEY_D))
    {
        velocity = tgm_v3_add(velocity, tgm_v4_to_v3(right));
    }
    if (tg_input_is_key_down(TG_KEY_SPACE))
    {
        velocity = tgm_v3_add(velocity, tgm_v4_to_v3(up));
    }
    if (tg_input_is_key_down(TG_KEY_CONTROL))
    {
        velocity = tgm_v3_add(velocity, tgm_v3_mulf(tgm_v4_to_v3(up), -1.0f));
    }

    if (tgm_v3_magsqr(velocity) != 0.0f)
    {
        const f32 camera_base_speed = tg_input_is_key_down(TG_KEY_SHIFT) ? 0.1f : 0.01f;
        const f32 camera_speed = camera_base_speed * delta_ms;
        velocity = tgm_v3_mulf(tgm_v3_normalized(velocity), camera_speed);
        sample_scene.camera.position = tgm_v3_add(sample_scene.camera.position, velocity);
    }

    sample_scene.last_mouse_x = mouse_x;
    sample_scene.last_mouse_y = mouse_y;

    if (tg_input_get_mouse_wheel_detents(TG_FALSE))
    {
        sample_scene.camera.perspective.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
    }

    tg_terrain_update(&sample_scene.terrain, delta_ms);


    sample_scene.light_timer += delta_ms;
    while (sample_scene.light_timer > 32000.0f)
    {
        sample_scene.light_timer -= 32000.0f;
    }
    const f32 lx0 = tgm_f32_sin(sample_scene.light_timer / 32000.0f * 2.0f * (f32)TGM_PI);
    const f32 ly0 = tgm_f32_sin(sample_scene.light_timer / 32000.0f * 2.0f * (f32)TGM_PI) * 0.5f - 0.5f;
    const f32 lz0 = tgm_f32_cos(sample_scene.light_timer / 32000.0f * 2.0f * (f32)TGM_PI);
    const f32 lx1 = 127.0f + 5.0f * tgm_f32_cos(sample_scene.light_timer / 4000.0f * 2.0f * (f32)TGM_PI);
    const f32 ly1 = 149.0f + 5.0f * tgm_f32_sin(sample_scene.light_timer / 4000.0f * 2.0f * (f32)TGM_PI);
    const f32 lz1 = 112.0f + 2.0f;


    //const v3 d0 = tgm_v3_normalized((v3) { lx0, ly0, lz0 });
    const v3 d0 = tgm_v3_normalized((v3) { 0.3f, -1.0f, -0.2f });
    const v3 c0d = tgm_v3_mulf((v3) { 0.529f, 0.808f, 0.922f }, 2.0f);
    const v3 c0n = tgm_v3_mulf((v3) { 0.992f, 0.369f, 0.325f }, 2.0f);
    //const v3 c0 = tgm_v3_lerp(c0n, c0d, -d0.y);
    const v3 c0 = { 4.0f, 4.0f, 4.0f };

    tg_renderer_begin(sample_scene.h_secondary_renderer);
    tg_renderer_push_directional_light(sample_scene.h_secondary_renderer, d0, (v3) { 4.0f, 4.0f, 10.0f });
    tg_terrain_render(&sample_scene.terrain, sample_scene.h_secondary_renderer);
    tg_render_command_h* ph_render_commands = TG_LIST_POINTER_TO(sample_scene.render_commands, 0);
    for (u32 i = 0; i < sample_scene.render_commands.count; i++)
    {
        tg_renderer_execute(sample_scene.h_secondary_renderer, ph_render_commands[i]);
    }
    tg_renderer_end(sample_scene.h_secondary_renderer);

    tg_renderer_begin(sample_scene.h_main_renderer);
    tg_renderer_push_directional_light(sample_scene.h_main_renderer, d0, c0);
    //tg_renderer_push_point_light(sample_scene.h_main_renderer, (v3) { lx1, ly1, lz1 }, (v3){ 8.0f, 8.0f, 16.0f });
    tg_terrain_render(&sample_scene.terrain, sample_scene.h_main_renderer);
    ph_render_commands = TG_LIST_POINTER_TO(sample_scene.render_commands, 0);
    for (u32 i = 0; i < sample_scene.render_commands.count; i++)
    {
        tg_renderer_execute(sample_scene.h_main_renderer, ph_render_commands[i]);
    }
    tg_renderer_end(sample_scene.h_main_renderer);

    tg_renderer_present(sample_scene.h_main_renderer);
    tg_renderer_clear(sample_scene.h_secondary_renderer);
    tg_renderer_clear(sample_scene.h_main_renderer);
}

static void tg__game_3d_destroy()
{
    tg_terrain_destroy(&sample_scene.terrain);

    tg_render_command_destroy(sample_scene.h_quad_render_command);
    tg_material_destroy(sample_scene.h_quad_material);
    tg_uniform_buffer_destroy(sample_scene.h_quad_color_ubo);
    tg_mesh_destroy(sample_scene.h_quad_mesh);

    tg_renderer_destroy(sample_scene.h_secondary_renderer);
    tg_renderer_destroy(sample_scene.h_main_renderer);
    tg_list_destroy(&sample_scene.render_commands);
}



typedef struct tg_raytracer_scene
{
    tg_camera         camera;
    u32               last_mouse_x;
    u32               last_mouse_y;
    tg_raytracer_h    h_raytracer;
} tg_raytracer_scene;

tg_raytracer_scene raytrace_scene;

static void tg__raytracer_test_create()
{
    raytrace_scene.camera.position = (v3) { 0.0f, 0.0f, 0.0f };
    raytrace_scene.camera.pitch = 0.0f;
    raytrace_scene.camera.yaw = 0.0f;
    raytrace_scene.camera.roll = 0.0f;
    raytrace_scene.camera.perspective.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
    raytrace_scene.camera.perspective.aspect = tg_platform_get_window_aspect_ratio();
    raytrace_scene.camera.perspective.near = -0.1f;
    raytrace_scene.camera.perspective.far = -1000.0f;
    tg_input_get_mouse_position(&raytrace_scene.last_mouse_x, &raytrace_scene.last_mouse_y);

    raytrace_scene.h_raytracer = tg_raytracer_create(&raytrace_scene.camera);
}

static void tg__raytracer_test_update_and_render(f32 delta_ms)
{
    u32 mouse_x = 0;
    u32 mouse_y = 0;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        raytrace_scene.camera.yaw += TGM_TO_RADIANS(0.064f * (f32)((i32)raytrace_scene.last_mouse_x - (i32)mouse_x));
        raytrace_scene.camera.pitch += TGM_TO_RADIANS(0.064f * (f32)((i32)raytrace_scene.last_mouse_y - (i32)mouse_y));
    }
    raytrace_scene.last_mouse_x = mouse_x;
    raytrace_scene.last_mouse_y = mouse_y;

    tg_raytracer_begin(raytrace_scene.h_raytracer);
    tg_raytracer_push_directional_light(raytrace_scene.h_raytracer, tgm_v3_normalized((v3) { 0.3f, -0.2f, -0.5f }), (v3) { 1.0f, 2.0f, 3.0f });
    tg_raytracer_push_sphere(raytrace_scene.h_raytracer, (v3) { 0.0f, 0.0f, -3.0f }, 1.0f);
    tg_raytracer_push_sphere(raytrace_scene.h_raytracer, (v3) { 2.0f, 1.0f, -5.0f }, 1.0f);
    tg_raytracer_push_sphere(raytrace_scene.h_raytracer, (v3) { -2.5f, 0.8f, -3.0f }, 1.0f);
    tg_raytracer_push_sphere(raytrace_scene.h_raytracer, (v3) { -1.0f, -2.0f, -2.0f }, 1.0f);
    tg_raytracer_end(raytrace_scene.h_raytracer);
    tg_raytracer_present(raytrace_scene.h_raytracer);
}



void tg_application_start()
{
    tg_graphics_init();
    tg_assets_init();
    //tg__game_3d_create();
    tg__raytracer_test_create();

#ifdef TG_DEBUG
    tg_debug_info debug_info = { 0 };
#endif

    tg_timer_h h_timer = tg_platform_timer_create();
    tg_platform_timer_start(h_timer);

    /*--------------------------------------------------------+
    | Start main loop                                         |
    +--------------------------------------------------------*/

    while (running)
    {
        tg_platform_timer_stop(h_timer);
        const f32 delta_ms = tg_platform_timer_elapsed_milliseconds(h_timer);
        tg_platform_timer_reset(h_timer);
        tg_platform_timer_start(h_timer);

#ifdef TG_DEBUG
        debug_info.ms_sum += delta_ms;
        debug_info.fps++;
        if (debug_info.ms_sum > 1000.0f)
        {
            if (debug_info.fps < 60)
            {
                TG_DEBUG_LOG("Low framerate!");
            }
            TG_DEBUG_LOG("%d ms", debug_info.ms_sum / debug_info.fps);
            TG_DEBUG_LOG("%u fps", debug_info.fps);

            debug_info.ms_sum = 0.0f;
            debug_info.fps = 0;
        }
#endif

        tg_input_clear();
        tg_platform_handle_events();
        tg__raytracer_test_update_and_render(delta_ms);
        //tg__game_3d_update_and_render(delta_ms);
    }

    /*--------------------------------------------------------+
    | End main loop                                           |
    +--------------------------------------------------------*/
    tg_platform_timer_destroy(h_timer);
    tg_graphics_wait_idle();
    tg__game_3d_destroy();
    tg_assets_shutdown();
    tg_graphics_shutdown();
}

void tg_application_on_window_resize(u32 width, u32 height)
{
}

void tg_application_quit()
{
	running = TG_FALSE;
}
