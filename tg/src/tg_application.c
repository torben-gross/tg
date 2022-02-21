#include "tg_application.h"

#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"
#include "tg_input.h"
#include "util/tg_string.h"
// TODO: remove
#include "util/tg_noise.h"



#define TG_VOXEL_SIZE_IN_METER    0.1f
#define TG_RUNNING_MPS            (50.0f / TG_VOXEL_SIZE_IN_METER)//(10.44f / TG_VOXEL_SIZE_IN_METER) // TODO: This is the actual reasonable speed
#define TG_RUNNING_KPH            TG_MPS2KPH(TG_RUNNING_MPS)
#define TG_WALKING_KPH            (5.0f / TG_VOXEL_SIZE_IN_METER)
#define TG_WALKING_MPS            TG_KPH2MPS(TG_WALKING_KPH)



#ifdef TG_DEBUG
typedef struct tg_debug_info
{
    f32    dt_sum;
    u32    fps;
} tg_debug_info;
#endif

typedef struct tg_sample_scene
{
    tg_camera       camera;
    tg_raytracer    raytracer;
    u32             last_mouse_x;
    u32             last_mouse_y;
    f32             light_timer;
} tg_sample_scene;



static b32 running = TG_TRUE;
static tg_sample_scene scene = { 0 };



static const v2 pds_extent = { 640.0f, 640.0f };
static const f32 pds_r = 16.0f;
static u32 pds_buffer_count;
static v2* p_pds_point_buffer;

static void tg__scene_create(void)
{
    // TODO: This is for testing only
    u32 pds_buffer_capacity = 0;
    tg_poisson_disk_sampling_2d(pds_extent, pds_r, 32, &pds_buffer_capacity, TG_NULL);
    pds_buffer_count = pds_buffer_capacity;
    p_pds_point_buffer = TG_MALLOC(pds_buffer_capacity * sizeof(v2));
    tg_poisson_disk_sampling_2d(pds_extent, pds_r, 32, &pds_buffer_count, p_pds_point_buffer);

    scene.camera.type = TG_CAMERA_TYPE_PERSPECTIVE;
    //scene.camera.position = (v3) { 128.0f, 141.0f + 50.0f, 128.0f };
    scene.camera.position = (v3) { 0.0f, 0.0f, 10.0f };
    scene.camera.position = tgm_v3_add(scene.camera.position, (v3) { TG_F32_EPSILON, TG_F32_EPSILON, TG_F32_EPSILON }); // TODO: This is just for debugging SVOs
    scene.camera.pitch = 0.0f;
    scene.camera.yaw = 0.0f;
    scene.camera.roll = 0.0f;
    scene.camera.persp.fov_y_in_radians = TG_DEG2RAD(70.0f);
    scene.camera.persp.aspect = tgp_get_window_aspect_ratio();
    scene.camera.persp.n = 0.1f;
    scene.camera.persp.f = 1000.0f;
    tg_input_get_mouse_position(&scene.last_mouse_x, &scene.last_mouse_y);

    tg_raytracer_create(&scene.camera, 64, &scene.raytracer);
    tg_raytracer_create_instance(&scene.raytracer, 0.0f, -64.0f, -500.0f, 512, 32, 1024);
    tg_raytracer_create_instance(&scene.raytracer, 128.0f, 0.0f, 0.0f, 128, 64, 32);
    for (u32 i = 0; i < 13; i++)
    {
        const f32 x = (f32)i * 35.0f - 256.0f;
        tg_raytracer_create_instance(&scene.raytracer, x, -16.0f, -64.0f, 32, 32, 32);
    }
    for (u32 i = 0; i < 13; i++)
    {
        const f32 x = (f32)i * 35.0f - 256.0f;
        tg_raytracer_create_instance(&scene.raytracer, x, 9.0f, -96.0f, 32, 32, 32);
    }
    for (u32 i = 0; i < 13; i++)
    {
        const f32 x = (f32)i * 35.0f - 256.0f;
        tg_raytracer_create_instance(&scene.raytracer, x - 6.0f, 100.0f, -70.0f, 32, 32, 32);
    }
    tg_raytracer_create_instance(&scene.raytracer, -32.0f, 64.0f, 16.0f, 32, 32, 32);
    tg_raytracer_color_lut_set(&scene.raytracer, 0, 1.0f, 0.0f, 0.0f);
    tg_raytracer_color_lut_set(&scene.raytracer, 1, 0.0f, 1.0f, 0.0f);
    tg_raytracer_color_lut_set(&scene.raytracer, 2, 0.0f, 0.0f, 1.0f);
    for (u32 i = 3; i < 256; i++)
    {
        u32 r_u32 = tgm_u32_murmur_hash_3(i);
        u32 g_u32 = tgm_u32_murmur_hash_3(r_u32);
        u32 b_u32 = tgm_u32_murmur_hash_3(g_u32);
        f32 r = r_u32 / (f32)TG_U32_MAX;
        f32 g = g_u32 / (f32)TG_U32_MAX;
        f32 b = b_u32 / (f32)TG_U32_MAX;
        tg_raytracer_color_lut_set(&scene.raytracer, (u8)i, r, g, b);
    }

#if 0 // TODO: voxelize?
    scene.h_sponza_mesh = tgvk_mesh_create2("meshes/sponza.obj", V3(0.01f));
    scene.h_sponza_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->albedo = (v4) { 1.0f, 1.0f, 1.0f, 1.0f };
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->metallic = 0.1f;
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->roughness = 0.4f;
    scene.h_sponza_material = tg_material_create_deferred(tg_vertex_shader_create("shaders/deferred/pbr.vert"), tg_fragment_shader_create("shaders/deferred/pbr.frag"));
    tg_handle p_sponza_handles[1] = { scene.h_sponza_ubo };
    scene.h_sponza_render_command = tg_render_command_create(scene.h_sponza_mesh, scene.h_sponza_material, (v3) { 128.0f, 140.0f, 128.0f }, 1, p_sponza_handles);
    tg_list_insert(&scene.render_commands, &scene.h_sponza_render_command);
    //tg_voxelizer* p_voxelizer = TG_MALLOC_STACK(sizeof(*p_voxelizer));
    //tg_voxelizer_create(p_voxelizer);
    //tg_voxelizer_begin(p_voxelizer);
    //tg_voxelizer_exec(p_voxelizer, scene.h_sponza_render_command);
    //tg_voxelizer_exec(p_voxelizer, scene.p_pbr_spheres[48].h_render_command);
    //tg_voxelizer_end(p_voxelizer, (v3i) { 1, 2, 1 }, scene.p_voxels);
    //tg_voxelizer_destroy(p_voxelizer);
    //TG_FREE_STACK(sizeof(*p_voxelizer));
#endif
}

static void tg__scene_update_and_render(f32 dt_ms)
{


    if (tg_input_is_key_pressed(TG_KEY_F11, TG_TRUE))
    {
        tg_system_time system_time = tgp_get_system_time();
        char p_filename_buffer[TG_MAX_PATH] = { 0 };
        tg_stringf(
            sizeof(p_filename_buffer),
            p_filename_buffer,
            "screenshot_%u_%u_%u_%u_%u_%u_%u.bmp",
            system_time.year,
            system_time.month,
            system_time.day,
            system_time.hour,
            system_time.minute,
            system_time.second,
            system_time.milliseconds
        );
        TG_DEBUG_LOG("Saving screenshot to: %s", p_filename_buffer);
        // TODO:
        //tg_raytracer_screenshot(&scene.raytracer, p_filename_buffer);
    }

    u32 mouse_x;
    u32 mouse_y;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_MIDDLE))
    {
        scene.camera.yaw += TG_DEG2RAD(0.064f * (f32)((i32)scene.last_mouse_x - (i32)mouse_x));
        scene.camera.pitch += TG_DEG2RAD(0.064f * (f32)((i32)scene.last_mouse_y - (i32)mouse_y));
    }
    scene.last_mouse_x = mouse_x;
    scene.last_mouse_y = mouse_y;

    const f32 rotate_speed = 0.001f;
    if (tg_input_is_key_down(TG_KEY_I))
    {
        scene.camera.pitch += rotate_speed * dt_ms;
    }
    if (tg_input_is_key_down(TG_KEY_J))
    {
        scene.camera.yaw += rotate_speed * dt_ms;
    }
    if (tg_input_is_key_down(TG_KEY_K))
    {
        scene.camera.pitch -= rotate_speed * dt_ms;
    }
    if (tg_input_is_key_down(TG_KEY_L))
    {
        scene.camera.yaw -= rotate_speed * dt_ms;
    }
    const m4 camera_rotation = tgm_m4_euler(scene.camera.pitch, scene.camera.yaw, scene.camera.roll);

    const v3 right = { camera_rotation.m00, camera_rotation.m10, camera_rotation.m20 }; // TODO: make these camera functions
    const v3 up = { 0.0f, 1.0f, 0.0f };
    const v3 forward = { -camera_rotation.m02, -camera_rotation.m12, -camera_rotation.m22 };

    v3 velocity = { 0 };
    if (tg_input_is_key_down(TG_KEY_W))
    {
        velocity = tgm_v3_add(velocity, forward);
    }
    if (tg_input_is_key_down(TG_KEY_A))
    {
        velocity = tgm_v3_sub(velocity, right);
    }
    if (tg_input_is_key_down(TG_KEY_S))
    {
        velocity = tgm_v3_sub(velocity, forward);
    }
    if (tg_input_is_key_down(TG_KEY_D))
    {
        velocity = tgm_v3_add(velocity, right);
    }
    if (tg_input_is_key_down(TG_KEY_SPACE))
    {
        velocity = tgm_v3_add(velocity, up);
    }
    if (tg_input_is_key_down(TG_KEY_CONTROL))
    {
        velocity = tgm_v3_sub(velocity, up);
    }

    if (tgm_v3_magsqr(velocity) != 0.0f)
    {
        const f32 player_speed_mps = tg_input_is_key_down(TG_KEY_SHIFT) ? TG_RUNNING_MPS : TG_WALKING_MPS;
        const f32 player_speed_mpms = player_speed_mps / 1000.0f;
        const f32 player_speed = player_speed_mpms * dt_ms;
        velocity = tgm_v3_mulf(tgm_v3_normalized(velocity), player_speed);
        scene.camera.position = tgm_v3_add(scene.camera.position, velocity);
    }

    if (tg_input_get_mouse_wheel_detents(TG_FALSE))
    {
        scene.camera.persp.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
    }

    scene.light_timer += dt_ms;
    while (scene.light_timer > 32000.0f)
    {
        scene.light_timer -= 32000.0f;
    }
    const f32 lx0 = tgm_f32_sin(scene.light_timer / 32000.0f * 2.0f * TG_PI);
    const f32 ly0 = tgm_f32_sin(scene.light_timer / 32000.0f * 2.0f * TG_PI) * 0.5f - 0.5f;
    const f32 lz0 = tgm_f32_cos(scene.light_timer / 32000.0f * 2.0f * TG_PI);
    const f32 lx1 = 127.0f + 5.0f * tgm_f32_cos(scene.light_timer / 4000.0f * 2.0f * TG_PI);
    const f32 ly1 = 149.0f + 5.0f * tgm_f32_sin(scene.light_timer / 4000.0f * 2.0f * TG_PI);
    const f32 lz1 = 112.0f + 2.0f;


    const v3 d0 = tgm_v3_normalized((v3) { lx0, ly0, lz0 });
    //const v3 d0 = tgm_v3_normalized((v3) { 0.3f, -1.0f, -0.2f });
    //const v3 c0d = tgm_v3_mulf((v3) { 0.529f, 0.808f, 0.922f }, 2.0f);
    //const v3 c0n = tgm_v3_mulf((v3) { 0.992f, 0.369f, 0.325f }, 2.0f);
    //const v3 c0 = tgm_v3_lerp(c0n, c0d, -d0.y);
    const v3 c0 = { 3.0f, 3.0f, 3.0f };

    TG_UNUSED(lx0);
    TG_UNUSED(ly0);
    TG_UNUSED(lz0);
    TG_UNUSED(lx1);
    TG_UNUSED(ly1);
    TG_UNUSED(lz1);
    TG_UNUSED(d0);
    TG_UNUSED(c0);

    const m4 m = tgm_m4_scale((v3) { 1.0f, 1.0f, 1.0f });
    tg_raytracer_push_debug_cuboid(&scene.raytracer, m);
    tg_raytracer_push_debug_cuboid(&scene.raytracer, tgm_m4_mul(tgm_m4_translate((v3) { 0.0f, pds_extent.y / 2.0f + 128.0f, -256.0f }), tgm_m4_scale((v3) { pds_extent.x, pds_extent.y, 0.001f })));
    for (u32 i = 0; i < pds_buffer_count; i++)
    {
        const m4 pds_s = tgm_m4_scale((v3) { pds_r, pds_r, 0.001f });
        const m4 pds_t = tgm_m4_translate((v3) { p_pds_point_buffer[i].x - pds_extent.x / 2.0f, p_pds_point_buffer[i].y + 128.0f, -256.0f });
        const m4 pds_m0 = tgm_m4_mul(pds_t, pds_s);
        const m4 pds_m1 = pds_t;
        tg_raytracer_push_debug_cuboid(&scene.raytracer, pds_m0);
        tg_raytracer_push_debug_cuboid(&scene.raytracer, pds_m1);
    }
    tg_raytracer_render(&scene.raytracer);
    tg_raytracer_clear(&scene.raytracer);

#if 0
    tg_renderer_begin(scene.h_secondary_renderer);
    tg_renderer_push_directional_light(scene.h_secondary_renderer, d0, (v3) { 4.0f, 4.0f, 10.0f });
    tg_terrain_render(scene.p_terrain, scene.h_secondary_renderer);
    tg_render_command_h* ph_render_commands = TG_LIST_AT(scene.render_commands, 0);
    for (u32 i = 0; i < scene.render_commands.count; i++)
    {
        tg_renderer_push_render_command(scene.h_secondary_renderer, ph_render_commands[i]);
    }
    tg_renderer_end(scene.h_secondary_renderer, dt_ms, TG_FALSE);

    tg_renderer_begin(scene.h_main_renderer);
    tg_renderer_set_sun_direction(scene.h_main_renderer, d0);
    //tg_renderer_push_directional_light(scene.h_main_renderer, d0, c0);
    tg_renderer_push_point_light(scene.h_main_renderer, (v3) { lx1, ly1, lz1 }, (v3){ 8.0f, 8.0f, 16.0f });
    tg_terrain_render(scene.p_terrain, scene.h_main_renderer);
    //tg_renderer_push_terrain(scene.h_main_renderer, scene.h_terrain);
    ph_render_commands = TG_LIST_AT(scene.render_commands, 0);
    for (u32 i = 0; i < scene.render_commands.count; i++)
    {
        tg_renderer_push_render_command(scene.h_main_renderer, ph_render_commands[i]);
    }

    {
        char p_ms_buffer[256] = { 0 };
        tg_stringf(256, p_ms_buffer, "Time %dms", dt_ms);
        tg_renderer_push_text(scene.h_main_renderer, p_ms_buffer);
    }

#if TG_ENABLE_DEBUG_TOOLS == 1
    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, (v3) { 108.5f, 138.5f, 116.5f }, (v3) { 1.0f, 1.0f, 1.0f }, (v4) { 1.0f, 0.0f, 0.0f, 1.0f });
    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, (v3) { 146.5f, 154.5f, 139.5f }, (v3) { 1.0f, 1.0f, 1.0f }, (v4) { 1.0f, 0.0f, 0.0f, 1.0f });
    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, (v3) { 127.5f, 146.5f, 128.0f }, (v3) { 39.0f, 17.0f, 24.0f }, (v4) { 1.0f, 0.0f, 1.0f, 1.0f });

    const f32 DEBUG_dh = (f32)TG_SVO_DIMS / 2.0f;
    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, (v3) { 64.0f + DEBUG_dh, 128.0f + DEBUG_dh, 64.0f + DEBUG_dh    }, V3((f32)TG_SVO_DIMS), (v4) { 0.5f, 1.0f, 0.0f, 1.0f });
    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, (v3) { 64.0f + DEBUG_dh, 128.0f + DEBUG_dh, 64.0f + TG_SVO_DIMS }, V3(1.0f),             (v4) { 0.5f, 0.0f, 1.0f, 1.0f });
    for (u32 z = 0; z < TG_SVO_DIMS; z++)
    {
        for (u32 y = 0; y < TG_SVO_DIMS; y++)
        {
            for (u32 x = 0; x < TG_SVO_DIMS; x++)
            {
                const u32 i = z * TG_SVO_DIMS * TG_SVO_DIMS + y * TG_SVO_DIMS + x;
                if (scene.p_voxels[i].albedo.w != 0.0f)
                {
                    const v3 p = { (f32)x + 64.5f, (f32)y + 128.5f, (f32)z + 64.5f };
                    const v3 s = V3(0.9f);

                    tg_renderer_draw_cube_DEBUG(scene.h_main_renderer, p, s, scene.p_voxels[i].albedo);
                }
            }
        }
    }
#endif

    tg_renderer_end(scene.h_main_renderer, dt_ms, TG_TRUE);

    tg_renderer_clear(scene.h_secondary_renderer);
    tg_renderer_clear(scene.h_main_renderer);

    i8 delta = 0;
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        delta += 20;
    }
    if (tg_input_is_mouse_button_down(TG_BUTTON_RIGHT))
    {
        delta -= 20;
    }
    if (delta)
    {
        const v3 world_position = tg_renderer_screen_to_world_position(scene.h_main_renderer, mouse_x, mouse_y);
        if (tgm_v3_magsqr(tgm_v3_sub(world_position, scene.camera.position)) < 0.9f * scene.camera.persp.f * scene.camera.persp.f)
        {
            tg_terrain_shape(scene.p_terrain, world_position, 1.5f, delta); // TODO: why does this not work with a radius of less than 1.0f, e.g. TG_PI * 0.25f?
        }
    }
#endif
}

static void tg__scene_destroy(void)
{
    tg_raytracer_destroy(&scene.raytracer);

#if 0 // TODO
    tg_material_destroy(scene.h_sponza_material);
    tg_uniform_buffer_destroy(scene.h_sponza_ubo);
    tg_render_command_destroy(scene.h_sponza_render_command);
    tgvk_mesh_destroy(scene.h_sponza_mesh);
#endif
}



void tg_application_start(void)
{
    tg_graphics_init();
    tg__scene_create();

#ifdef TG_DEBUG
    tg_debug_info debug_info = { 0 };
#endif

    tg_timer timer = { 0 };
    tgp_timer_init(&timer);
    tgp_timer_start(&timer);

    /*--------------------------------------------------------+
    | Start main loop                                         |
    +--------------------------------------------------------*/

    while (running)
    {
        tgp_timer_stop(&timer);
        const f32 dt_ms = tgp_timer_elapsed_ms(&timer);
        tgp_timer_reset(&timer);
        tgp_timer_start(&timer);

#ifdef TG_DEBUG
        debug_info.dt_sum += dt_ms;
        debug_info.fps++;
        if (debug_info.dt_sum > 1000.0f)
        {
            if (debug_info.fps < 60)
            {
                if (tgp_power_source_is_battery())
                {
                    TG_DEBUG_LOG("Low framerate! Power source is battery!\n");
                }
                else
                {
                    TG_DEBUG_LOG("Low framerate!\n");
                }
            }
            TG_DEBUG_LOG("%d ms\n", debug_info.dt_sum / debug_info.fps);
            TG_DEBUG_LOG("%u fps\n", debug_info.fps);

            debug_info.dt_sum = 0.0f;
            debug_info.fps = 0;
        }
#endif

        tg_input_clear();
        tgp_handle_events();
        tg__scene_update_and_render(dt_ms);
    }

    /*--------------------------------------------------------+
    | End main loop                                           |
    +--------------------------------------------------------*/
    tg_graphics_wait_idle();
    tg__scene_destroy();
    tg_graphics_shutdown();
}

void tg_application_on_window_resize(u32 width, u32 height)
{
    tg_graphics_on_window_resize(width, height);
}

void tg_application_quit(void)
{
	running = TG_FALSE;
}
