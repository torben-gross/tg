#include "tg_application.h"

#include "graphics/tg_graphics.h"
#include "graphics/tg_sparse_voxel_octree.h"
#include "memory/tg_memory.h"
#include "physics/tg_physics.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"
#include "tg_input.h"
#include "tg_transvoxel_terrain.h"
#include "util/tg_list.h"
#include "util/tg_string.h"



#ifdef TG_DEBUG
typedef struct tg_debug_info
{
    f32    dt_sum;
    u32    fps;
} tg_debug_info;
#endif

typedef struct tg_pbr_sphere
{
    tg_uniform_buffer_h    h_ubo;
    tg_material_h          h_material;
    tg_render_command_h    h_render_command;
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
    tg_list                    render_commands;
    tg_camera                  camera;
    tg_renderer_h              h_main_renderer;
    tg_renderer_h              h_secondary_renderer;
    u32                        last_mouse_x;
    u32                        last_mouse_y;
    struct quad
    {
        tg_mesh_h              h_quad_mesh;
        tg_uniform_buffer_h    h_quad_color_ubo;
        tg_material_h          h_quad_material;
        tg_render_command_h    h_quad_render_command;
        f32                    quad_offset_z;
        f32                    quad_delta_time_sum_looped;
    };

    tg_mesh_h                  h_sponza_mesh;
    tg_render_command_h        h_sponza_render_command;
    tg_kd_tree*                p_sponza_kd_tree;
    tg_voxel                   p_voxels[TG_SVO_DIMS3];
    tg_uniform_buffer_h        h_sponza_ubo;
    tg_mesh_h                  h_my_mesh;
    tg_uniform_buffer_h        h_my_ubo;

    u8                         p_light_values[6];
    tg_mesh_h                  h_pbr_sphere_mesh;
    tg_mesh_h                  h_pbr_sphere_mesh2;
    tg_pbr_sphere              p_pbr_spheres[49];
    tg_mesh_h                  h_probe_mesh;
    v3                         probe_translation;
    tg_cube_map_h              h_probe_cube_map;
    tg_render_command_h        h_probe_render_command;
    tg_terrain                 terrain;
    f32                        light_timer;
} tg_sample_scene;



b32 running = TG_TRUE;
tg_sample_scene scene = { 0 };



static v3 tg__random_dir_hemisphere(tg_random* p_random, v3 normal)
{
    const f32 x = tgm_f32_acos(normal.x);
    const f32 y = tgm_f32_acos(normal.y);
    const f32 z = tgm_f32_acos(normal.z);

    const f32 ph = TG_PI / 2.0f;
    const f32 lx = x - ph;
    const f32 rx = x + ph;
    const f32 ly = y - ph;
    const f32 ry = y + ph;
    const f32 lz = z - ph;
    const f32 rz = z + ph;

    v3 result = tgm_v3_normalized((v3) {
        tgm_f32_cos(tgm_random_next_f32_between(p_random, lx, rx)),
        tgm_f32_cos(tgm_random_next_f32_between(p_random, ly, ry)),
        tgm_f32_cos(tgm_random_next_f32_between(p_random, lz, rz))
    });
    return result;
}

static void tg__raycast()
{
    v3 p_ray_origins[6] = {
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f }),
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f }),
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f }),
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f }),
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f }),
        tgm_v3_sub(scene.probe_translation, (v3) { 128.0f, 140.0f, 128.0f })
    };
    v3 p_ray_directions[6] = {
        {  1.0f,  0.0f,  0.0f },
        { -1.0f,  0.0f,  0.0f },
        {  0.0f,  1.0f,  0.0f },
        {  0.0f, -1.0f,  0.0f },
        {  0.0f,  0.0f,  1.0f },
        {  0.0f,  0.0f, -1.0f }
    };
    b32 p_raycast_results[6] = { 0 };
    tg_raycast_hit p_raycast_hits[6] = { 0 };
    u32 ms = (u32)tg_platform_get_system_time().milliseconds;
    tg_random r = tgm_random_init(ms == 0 ? 1 : ms);
    for (u8 i = 0; i < 6; i++)
    {
        p_ray_directions[i] = tg__random_dir_hemisphere(&r, p_ray_directions[i]);
        p_raycast_results[i] = tg_raycast_kd_tree(p_ray_origins[i], p_ray_directions[i], scene.p_sponza_kd_tree, &p_raycast_hits[i]);
        const f32 t = 0.02f;
        if (p_raycast_results[i])
        {
            p_ray_origins[i] = p_raycast_hits[i].hit;
            p_ray_directions[i] = (v3){ 0.0f, 1.0f, 0.0f };
            p_raycast_results[i] = tg_raycast_kd_tree(p_ray_origins[i], p_ray_directions[i], scene.p_sponza_kd_tree, &p_raycast_hits[i]);
            scene.p_light_values[i] = (u8)((1 - t) * (f32)scene.p_light_values[i] + t * (1.0f - (f32)p_raycast_results[i]) * 127.0f);
        }
        else
        {
            scene.p_light_values[i] = (u8)((1 - t) * scene.p_light_values[i] + t * 255.0f);
        }
    }
    tg_cube_map_set_data(scene.h_probe_cube_map, scene.p_light_values);
}

static void tg__game_3d_create()
{
    scene.render_commands = TG_LIST_CREATE(tg_render_command_h);
    scene.camera.type = TG_CAMERA_TYPE_PERSPECTIVE;
    scene.camera.position = (v3) { 128.0f, 141.0f, 128.0f };
    scene.camera.pitch = 0.0f;
    scene.camera.yaw = 0.0f;
    scene.camera.roll = 0.0f;
    scene.camera.persp.fov_y_in_radians = TG_TO_RADIANS(70.0f);
    scene.camera.persp.aspect = tg_platform_get_window_aspect_ratio();
    scene.camera.persp.n = -0.1f;
    scene.camera.persp.f = -1000.0f;
    tg_input_get_mouse_position(&scene.last_mouse_x, &scene.last_mouse_y);
    scene.h_main_renderer = tg_renderer_create(&scene.camera);
    scene.h_secondary_renderer = tg_renderer_create(&scene.camera);
    tg_renderer_enable_shadows(scene.h_main_renderer, TG_FALSE);
    tg_renderer_enable_shadows(scene.h_secondary_renderer, TG_FALSE);




    const v3 p_quad_positions[4] = {
        { -3.2f, -1.8f, 0.0f },
        {  3.2f, -1.8f, 0.0f },
        {  3.2f,  1.8f, 0.0f },
        { -3.2f,  1.8f, 0.0f }
    };
    const v2 p_quad_uvs[4] = {
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f }
    };
    const u16 p_quad_indices[6] = {
        0, 1, 2, 2, 3, 0
    };

    scene.h_quad_mesh = tg_mesh_create();
    tg_mesh_set_indices(scene.h_quad_mesh, 6, p_quad_indices);
    tg_mesh_set_positions(scene.h_quad_mesh, 4, p_quad_positions);
    tg_mesh_regenerate_normals(scene.h_quad_mesh);
    tg_mesh_set_uvs(scene.h_quad_mesh, 4, p_quad_uvs);
    scene.h_quad_color_ubo = tg_uniform_buffer_create(sizeof(v3));
    *((v3*)tg_uniform_buffer_data(scene.h_quad_color_ubo)) = (v3) { 1.0f, 0.0f, 0.0f };
    tg_handle p_custom_handles[2] = { scene.h_quad_color_ubo, tg_renderer_get_render_target(scene.h_secondary_renderer) };
    scene.h_quad_material = tg_material_create_forward(tg_vertex_shader_get("shaders/forward/forward.vert"), tg_fragment_shader_get("shaders/forward/texture.frag"));

    scene.h_quad_render_command = tg_render_command_create(scene.h_quad_mesh, scene.h_quad_material, (v3) { 0.0f, 133.0f, 0.0f }, 2, p_custom_handles);
    scene.quad_offset_z = -65.0f;
    tg_list_insert(&scene.render_commands, &scene.h_quad_render_command);













    scene.h_probe_mesh = tg_mesh_create_sphere(0.5f, 64, 32, TG_TRUE, TG_TRUE, TG_FALSE);
    scene.probe_translation = (v3){ 128.0f + 7.0f, 153.0f, 128.0f };
    scene.h_probe_cube_map = tg_cube_map_create(1, TG_COLOR_IMAGE_FORMAT_R8, TG_NULL);
    tg_material_h h_probe_material = tg_material_create_forward(tg_vertex_shader_get("shaders/forward/forward.vert"), tg_fragment_shader_get("shaders/forward/probe.frag"));
    tg_handle p_probe_handles[1] = { scene.h_probe_cube_map };
    scene.h_probe_render_command = tg_render_command_create(scene.h_probe_mesh, h_probe_material, scene.probe_translation, 1, p_probe_handles);
    tg_list_insert(&scene.render_commands, &scene.h_probe_render_command);



    scene.h_pbr_sphere_mesh = tg_mesh_create_sphere_flat(0.5f, 128, 64, TG_TRUE, TG_TRUE, TG_FALSE);
    scene.h_pbr_sphere_mesh2 = tg_mesh_create_sphere_flat(3.0f, 128, 64, TG_TRUE, TG_TRUE, TG_FALSE);
    for (u32 y = 0; y < 7; y++)
    {
        for (u32 x = 0; x < 7; x++)
        {

            const u32 i = y * 7 + x;

            const v3 sphere_translation = { 128.0f - 7.0f + (f32)x * 2.0f, 143.0f + (f32)y * 2.0f, 112.0f };
            scene.p_pbr_spheres[i].h_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
            ((tg_pbr_material*)tg_uniform_buffer_data(scene.p_pbr_spheres[i].h_ubo))->albedo = (v4){ 1.0f, 1.0f, 1.0f, 1.0f };
            ((tg_pbr_material*)tg_uniform_buffer_data(scene.p_pbr_spheres[i].h_ubo))->metallic = (f32)x / 6.0f;
            ((tg_pbr_material*)tg_uniform_buffer_data(scene.p_pbr_spheres[i].h_ubo))->roughness = ((f32)y + 0.1f) / 6.5f;
            ((tg_pbr_material*)tg_uniform_buffer_data(scene.p_pbr_spheres[i].h_ubo))->ao = 1.0f;
            scene.p_pbr_spheres[i].h_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred/pbr.vert"), tg_fragment_shader_get("shaders/deferred/pbr.frag"));
            tg_handle p_handles[1] = { scene.p_pbr_spheres[i].h_ubo };
            if (x == 6 && y == 6)
            {
                scene.p_pbr_spheres[i].h_render_command = tg_render_command_create(
                    scene.h_pbr_sphere_mesh2, scene.p_pbr_spheres[i].h_material,
                    (v3) { 133.0f, 140.0f, 128.0f },
                    1, p_handles
                );
            }
            else
            {
                scene.p_pbr_spheres[i].h_render_command = tg_render_command_create(scene.h_pbr_sphere_mesh, scene.p_pbr_spheres[i].h_material, sphere_translation, 1, p_handles);
            }
            tg_list_insert(&scene.render_commands, &scene.p_pbr_spheres[i].h_render_command);
        }
    }








    scene.h_sponza_mesh = tg_mesh_create2("meshes/sponza.obj", V3(0.01f));
    scene.p_sponza_kd_tree = tg_kd_tree_create(scene.h_sponza_mesh);

    scene.h_sponza_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->albedo = (v4) { 1.0f, 1.0f, 1.0f, 1.0f };
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->metallic = 0.1f;
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->roughness = 0.4f;
    ((tg_pbr_material*)tg_uniform_buffer_data(scene.h_sponza_ubo))->ao = 1.0f;
    tg_material_h h_sponza_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred/pbr.vert"), tg_fragment_shader_get("shaders/deferred/pbr.frag"));

    tg_handle p_sponza_handles[1] = { scene.h_sponza_ubo };
    scene.h_sponza_render_command = tg_render_command_create(scene.h_sponza_mesh, h_sponza_material, (v3) { 128.0f, 140.0f, 128.0f }, 1, p_sponza_handles);
    tg_list_insert(&scene.render_commands, &scene.h_sponza_render_command);
    tg__raycast();

    // TODO: remove
    //const v3i min_corner = { 108, 138, 116 };
    //const v3i dimensions = { 40, 18, 24 };
    tg_voxelizer* p_voxelizer = TG_MEMORY_STACK_ALLOC(sizeof(*p_voxelizer));
    tg_voxelizer_create(p_voxelizer);
    tg_voxelizer_begin(p_voxelizer);
    tg_voxelizer_exec(p_voxelizer, scene.h_sponza_render_command);
    tg_voxelizer_exec(p_voxelizer, scene.p_pbr_spheres[48].h_render_command);
    tg_voxelizer_end(p_voxelizer, (v3i) { 1, 2, 1 }, scene.p_voxels);
    tg_voxelizer_destroy(p_voxelizer);
    TG_MEMORY_STACK_FREE(sizeof(*p_voxelizer));



    //scene.h_my_mesh = tg_mesh_create2("meshes/untitled.obj", V3(0.12f));
    //scene.h_my_mesh = tg_mesh_create2("meshes/nymph1.obj", V3(1.0f));
    //scene.h_my_ubo = tg_uniform_buffer_create(sizeof(tg_pbr_material));
    //((tg_pbr_material*)tg_uniform_buffer_data(scene.h_my_ubo))->albedo = (v4){ 0.816f, 0.506f, 0.024f, 1.0f };
    //((tg_pbr_material*)tg_uniform_buffer_data(scene.h_my_ubo))->metallic = 1.0f;
    //((tg_pbr_material*)tg_uniform_buffer_data(scene.h_my_ubo))->roughness = 0.4f;
    //((tg_pbr_material*)tg_uniform_buffer_data(scene.h_my_ubo))->ao = 1.0f;
    //tg_material_h h_my_material = tg_material_create_deferred(tg_vertex_shader_get("shaders/deferred/pbr.vert"), tg_fragment_shader_get("shaders/deferred/pbr.frag"));
    //tg_handle p_my_handles[1] = { scene.h_my_ubo };
    //tg_render_command_h h_my_render_command = tg_render_command_create(scene.h_my_mesh, h_my_material, (v3) { 128.0f, 141.9f, 126.0f }, 1, p_my_handles);
    //tg_list_insert(&scene.render_commands, &h_my_render_command);

    scene.terrain = tg_terrain_create(&scene.camera);
}

static void tg__game_3d_update_and_render(f32 dt)
{
    tg__raycast();
    if (tg_input_is_key_down(TG_KEY_K))
    {
        scene.quad_offset_z += 0.01f * dt;
    }
    if (tg_input_is_key_down(TG_KEY_L))
    {
        scene.quad_offset_z -= 0.01f * dt;
    }
    v3 quad_offset_translation_z = { 0.0f, 133.0f, scene.quad_offset_z };
    tg_render_command_set_position(scene.h_quad_render_command, quad_offset_translation_z);

    scene.quad_delta_time_sum_looped += dt;
    if (scene.quad_delta_time_sum_looped >= 10000.0f)
    {
        scene.quad_delta_time_sum_looped -= 10000.0f;
    }
    const f32 sint = tgm_f32_sin(0.001f * scene.quad_delta_time_sum_looped * 2.0f * TG_PI);
    const f32 noise_x = tgm_noise(sint, 0.1f, 0.1f) + 0.5f;
    const f32 noise_y = tgm_noise(0.1f, sint, 0.1f) + 0.5f;
    const f32 noise_z = tgm_noise(0.1f, 0.1f, sint) + 0.5f;
    *((v3*)tg_uniform_buffer_data(scene.h_quad_color_ubo)) = (v3) { noise_x, noise_y, noise_z };



    u32 mouse_x;
    u32 mouse_y;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        scene.camera.yaw += TG_TO_RADIANS(0.064f * (f32)((i32)scene.last_mouse_x - (i32)mouse_x));
        scene.camera.pitch += TG_TO_RADIANS(0.064f * (f32)((i32)scene.last_mouse_y - (i32)mouse_y));
    }
    const m4 camera_rotation = tgm_m4_euler(scene.camera.pitch, scene.camera.yaw, scene.camera.roll);

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
        const f32 camera_speed = camera_base_speed * dt;
        velocity = tgm_v3_mulf(tgm_v3_normalized(velocity), camera_speed);
        scene.camera.position = tgm_v3_add(scene.camera.position, velocity);
    }

    scene.last_mouse_x = mouse_x;
    scene.last_mouse_y = mouse_y;

    if (tg_input_get_mouse_wheel_detents(TG_FALSE))
    {
        scene.camera.persp.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
    }

    tg_terrain_update(&scene.terrain, dt);


    scene.light_timer += dt;
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


    //const v3 d0 = tgm_v3_normalized((v3) { lx0, ly0, lz0 });
    const v3 d0 = tgm_v3_normalized((v3) { 0.3f, -1.0f, -0.2f });
    const v3 c0d = tgm_v3_mulf((v3) { 0.529f, 0.808f, 0.922f }, 2.0f);
    const v3 c0n = tgm_v3_mulf((v3) { 0.992f, 0.369f, 0.325f }, 2.0f);
    //const v3 c0 = tgm_v3_lerp(c0n, c0d, -d0.y);
    const v3 c0 = V3(1.0f);

    tg_renderer_begin(scene.h_secondary_renderer);
    tg_renderer_push_directional_light(scene.h_secondary_renderer, d0, (v3) { 4.0f, 4.0f, 10.0f });
    tg_terrain_render(&scene.terrain, scene.h_secondary_renderer);
    tg_render_command_h* ph_render_commands = TG_LIST_POINTER_TO(scene.render_commands, 0);
    for (u32 i = 0; i < scene.render_commands.count; i++)
    {
        tg_renderer_exec(scene.h_secondary_renderer, ph_render_commands[i]);
    }
    tg_renderer_end(scene.h_secondary_renderer, dt, TG_FALSE);

    tg_renderer_begin(scene.h_main_renderer);
    tg_renderer_push_directional_light(scene.h_main_renderer, d0, c0);
    //tg_renderer_push_point_light(scene.h_main_renderer, (v3) { lx1, ly1, lz1 }, (v3){ 8.0f, 8.0f, 16.0f });
    tg_terrain_render(&scene.terrain, scene.h_main_renderer);
    ph_render_commands = TG_LIST_POINTER_TO(scene.render_commands, 0);
    for (u32 i = 0; i < scene.render_commands.count; i++)
    {
        tg_renderer_exec(scene.h_main_renderer, ph_render_commands[i]);
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

    tg_renderer_end(scene.h_main_renderer, dt, TG_TRUE);

    tg_renderer_clear(scene.h_secondary_renderer);
    tg_renderer_clear(scene.h_main_renderer);
}

static void tg__game_3d_destroy()
{
    tg_terrain_destroy(&scene.terrain);

    tg_render_command_destroy(scene.h_quad_render_command);
    tg_material_destroy(scene.h_quad_material);
    tg_uniform_buffer_destroy(scene.h_quad_color_ubo);
    tg_mesh_destroy(scene.h_quad_mesh);

    tg_renderer_destroy(scene.h_secondary_renderer);
    tg_renderer_destroy(scene.h_main_renderer);
    tg_list_destroy(&scene.render_commands);
}



void tg_application_start()
{
    tg_graphics_init();
    tg_assets_init();
    tg__game_3d_create();

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
        const f32 dt = tg_platform_timer_elapsed_milliseconds(h_timer);
        tg_platform_timer_reset(h_timer);
        tg_platform_timer_start(h_timer);

#ifdef TG_DEBUG
        debug_info.dt_sum += dt;
        debug_info.fps++;
        if (debug_info.dt_sum > 1000.0f)
        {
            if (debug_info.fps < 60)
            {
                TG_DEBUG_LOG("Low framerate!\n");
            }
            TG_DEBUG_LOG("%d ms\n", debug_info.dt_sum / debug_info.fps);
            TG_DEBUG_LOG("%u fps\n", debug_info.fps);

            debug_info.dt_sum = 0.0f;
            debug_info.fps = 0;
        }
#endif

        tg_input_clear();
        tg_platform_handle_events();
        tg__game_3d_update_and_render(dt);
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
