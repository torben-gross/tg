#include "tg_application.h"



#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_entity.h"
#include "tg_input.h"
#include "tg_marching_cubes.h"
#include "tg_scene.h"
#include "util/tg_list.h"
#include "util/tg_qsort.h"
#include "util/tg_rectangle_packer.h"
#include "util/tg_string.h"
#include "util/tg_timer.h"



#define TG_POINT_LIGHT_COUNT       64
#define TG_CHUNKS_X                8
#define TG_CHUNKS_Z                8
#define TG_CHUNK_VERTEX_COUNT_X    16
#define TG_CHUNK_VERTEX_COUNT_Y    16
#define TG_CHUNK_VERTEX_COUNT_Z    16



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

typedef struct tg_isolevel_uniform_buffer
{
    u32    chunk_index_x;
    u32    chunk_index_y;
    u32    chunk_index_z;
} tg_isolevel_uniform_buffer;

typedef struct tg_marching_cubes_uniform_buffer
{
    u32    chunk_index_x;
    u32    chunk_index_y;
    u32    chunk_index_z;
} tg_marching_cubes_uniform_buffer;

typedef struct tg_test_deferred
{
    tg_scene                       scene;
    tg_list                        entities;
    tg_camera_info                 camera_info;
    tg_camera_h                    secondary_camera_h;
    tg_color_image_h               textures_h[13];
    tg_texture_atlas_h             texture_atlas_h;
    tg_mesh_h                      quad_mesh_h;
    tg_vertex_shader_h             default_vertex_shader_h;
    tg_fragment_shader_h           default_fragment_shader_h;
    tg_material_h                  default_material_h;
    tg_uniform_buffer_h            custom_uniform_buffer_h;
    v3*                            p_custom_uniform_buffer_data;
    tg_color_image_h               image_h;
    tg_vertex_shader_h             custom_vertex_shader_h;
    tg_fragment_shader_h           custom_fragment_shader_h;
    tg_material_h                  custom_material_h;
    tg_entity                      quad_entity;
    tg_mesh_h                      ground_mesh_h;
    tg_entity                      ground_entity;
    tg_mesh_h*                     p_chunk_meshes;
    tg_entity*                     p_chunk_entities;
    f32                            quad_offset;
    f32                            dtsum;
} tg_test_deferred;

typedef struct tg_test_forward
{
    tg_camera_h              camera_h;
    tg_scene                 scene;
    tg_mesh_h                mesh_h;
    tg_mesh_h                mesh2_h;
    tg_vertex_shader_h       vertex_shader_h;
    tg_vertex_shader_h       vertex_shader2_h;
    tg_fragment_shader_h     fragment_shader_h;
    tg_fragment_shader_h     fragment_shader2_h;
    tg_material_h            material_h;
    tg_material_h            material2_h;
    tg_entity                entity;
    tg_entity                entity2;
} tg_test_forward;



b32 running = TG_TRUE;
const char* asset_path = "assets"; // TODO: determine this some other way
tg_test_deferred test_deferred = { 0 };
tg_test_forward test_forward = { 0 };



void tg_application_internal_game_3d_create()
{
    test_deferred.entities = TG_LIST_CREATE(tg_entity);
    test_deferred.camera_info.position = (v3){ 0.0f, 0.0f, 0.0f };
    test_deferred.camera_info.pitch = 0.0f;
    test_deferred.camera_info.yaw = 0.0f;
    test_deferred.camera_info.roll = 0.0f;
    test_deferred.camera_info.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
    test_deferred.camera_info.aspect = tg_platform_get_window_aspect_ratio(&test_deferred.camera_info.aspect);
    test_deferred.camera_info.near = -0.1f;
    test_deferred.camera_info.far = -1000.0f;
    tg_input_get_mouse_position(&test_deferred.camera_info.last_mouse_x, &test_deferred.camera_info.last_mouse_y);
    test_deferred.camera_info.camera_h = tg_camera_create_perspective(&test_deferred.camera_info.position, test_deferred.camera_info.pitch, test_deferred.camera_info.yaw, test_deferred.camera_info.roll, test_deferred.camera_info.fov_y_in_radians, test_deferred.camera_info.near, test_deferred.camera_info.far);
    test_deferred.secondary_camera_h = tg_camera_create_perspective(&test_deferred.camera_info.position, test_deferred.camera_info.pitch, test_deferred.camera_info.yaw, test_deferred.camera_info.roll, test_deferred.camera_info.fov_y_in_radians, test_deferred.camera_info.near, test_deferred.camera_info.far);

    tg_point_light p_point_lights[TG_POINT_LIGHT_COUNT] = { 0 };
    tg_random random = { 0 };
    tgm_random_init(&random, 13031995);
    for (u32 i = 0; i < TG_POINT_LIGHT_COUNT; i++)
    {
        p_point_lights[i].position = (v3){ tgm_random_next_f32(&random) * 100.0f, tgm_random_next_f32(&random) * 50.0f, -tgm_random_next_f32(&random) * 100.0f };
        p_point_lights[i].color = (v3){ tgm_random_next_f32(&random), tgm_random_next_f32(&random), tgm_random_next_f32(&random) };
        p_point_lights[i].radius = tgm_random_next_f32(&random) * 50.0f;
    }

    tg_color_image_create_info color_image_create_info = { 0 };
    tg_platform_get_window_size(&color_image_create_info.width, &color_image_create_info.height);
    color_image_create_info.mip_levels = 1;
    color_image_create_info.format = TG_COLOR_IMAGE_FORMAT_B8G8R8A8;
    color_image_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
    color_image_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
    color_image_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_REPEAT;
    color_image_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_REPEAT;
    color_image_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_REPEAT;

    tg_camera_h p_cameras_h[2] = { test_deferred.camera_info.camera_h, test_deferred.secondary_camera_h };
    test_deferred.scene = tg_scene_create(2, p_cameras_h, TG_POINT_LIGHT_COUNT, p_point_lights);





    for (u32 i = 0; i < 9; i++)
    {
        char image_buffer[256] = { 0 };
        tg_string_format(sizeof(image_buffer), image_buffer, "textures/%i.bmp", i + 1);
        test_deferred.textures_h[i] = tg_color_image_load(image_buffer);
    }
    test_deferred.textures_h[9] = tg_color_image_load("textures/cabin_final.bmp");
    test_deferred.textures_h[10] = tg_color_image_load("textures/test_icon.bmp");
    test_deferred.textures_h[11] = tg_color_image_load("textures/pbb_birch.bmp");
    test_deferred.textures_h[12] = tg_color_image_load("textures/squirrel.bmp");
    test_deferred.texture_atlas_h = tg_texture_atlas_create_from_images(13, test_deferred.textures_h);




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

    test_deferred.quad_mesh_h = tg_mesh_create(4, p_quad_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    test_deferred.default_vertex_shader_h = tg_vertex_shader_create("shaders/geometry.vert");
    test_deferred.default_fragment_shader_h = tg_fragment_shader_create("shaders/geometry.frag");
    test_deferred.default_material_h = tg_material_create_deferred(test_deferred.default_vertex_shader_h, test_deferred.default_fragment_shader_h, 0, TG_NULL);

    test_deferred.custom_uniform_buffer_h = tg_uniform_buffer_create(sizeof(v3));
    test_deferred.p_custom_uniform_buffer_data = tg_uniform_buffer_data(test_deferred.custom_uniform_buffer_h);
    *test_deferred.p_custom_uniform_buffer_data = (v3){ 1.0f, 0.0f, 0.0f };
    test_deferred.image_h = tg_color_image_load("textures/test_icon.bmp");
    test_deferred.custom_vertex_shader_h = tg_vertex_shader_create("shaders/custom_forward.vert");
    test_deferred.custom_fragment_shader_h = tg_fragment_shader_create("shaders/custom_forward.frag");
    tg_handle p_custom_handles[2] = { test_deferred.custom_uniform_buffer_h, tg_camera_get_render_target(test_deferred.secondary_camera_h) };
    test_deferred.custom_material_h = tg_material_create_forward(test_deferred.custom_vertex_shader_h, test_deferred.custom_fragment_shader_h, 2, p_custom_handles);

    test_deferred.quad_entity = tg_entity_create(&test_deferred.scene, test_deferred.quad_mesh_h, test_deferred.custom_material_h);
    test_deferred.quad_offset = -65.0f;
    tg_list_insert(&test_deferred.entities, &test_deferred.quad_entity);

    test_deferred.ground_mesh_h = tg_mesh_create(4, p_ground_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    test_deferred.ground_entity = tg_entity_create(&test_deferred.scene, test_deferred.ground_mesh_h, test_deferred.default_material_h);
    tg_list_insert(&test_deferred.entities, &test_deferred.ground_entity);







    const f32 cell_stride = 1.1f;
    const f32 noise_scale = 0.1f;

    tg_uniform_buffer_h isolevel_uniform_buffer_h = tg_uniform_buffer_create(sizeof(tg_isolevel_uniform_buffer));
    tg_isolevel_uniform_buffer* p_isolevel_uniform_buffer_data = tg_uniform_buffer_data(isolevel_uniform_buffer_h);
    tg_storage_image_3d_h isolevel_storage_image_3d_h = tg_storage_image_3d_create(TG_CHUNK_VERTEX_COUNT_X, TG_CHUNK_VERTEX_COUNT_Y, TG_CHUNK_VERTEX_COUNT_Z, TG_STORAGE_IMAGE_FORMAT_R32_SFLOAT);

    const tg_handle_type p_isolevel_compute_shader_handle_types[2] = {
        TG_HANDLE_TYPE_UNIFORM_BUFFER,
        TG_HANDLE_TYPE_STORAGE_IMAGE_3D
    };

    tg_compute_shader_h isolevel_compute_shader_h = tg_compute_shader_create("shaders/isolevel.comp", 2, p_isolevel_compute_shader_handle_types);
    tg_handle p_isolevel_handles[2] = { isolevel_uniform_buffer_h, isolevel_storage_image_3d_h };
    tg_compute_shader_bind_input(isolevel_compute_shader_h, 0, 2, p_isolevel_handles);



    tg_uniform_buffer_h marching_cubes_uniform_buffer_h = tg_uniform_buffer_create(sizeof(tg_marching_cubes_uniform_buffer));
    tg_marching_cubes_uniform_buffer* p_marching_cubes_uniform_buffer_data = tg_uniform_buffer_data(marching_cubes_uniform_buffer_h);
    tg_compute_buffer_h marching_cubes_compute_buffer_h = tg_compute_buffer_create((TG_CHUNK_VERTEX_COUNT_X - 1) * (TG_CHUNK_VERTEX_COUNT_Y - 1) * (TG_CHUNK_VERTEX_COUNT_Z - 1) * 15 * sizeof(tg_vertex_3d), TG_TRUE);
    tg_vertex_3d* p_verts = tg_compute_buffer_data(marching_cubes_compute_buffer_h);

    const tg_handle_type p_marching_cubes_compute_shader_handle_types[3] = {
        TG_HANDLE_TYPE_UNIFORM_BUFFER,
        TG_HANDLE_TYPE_STORAGE_IMAGE_3D,
        TG_HANDLE_TYPE_COMPUTE_BUFFER
    };

    tg_compute_shader_h marching_cubes_compute_shader_h = tg_compute_shader_create("shaders/marching_cubes.comp", 3, p_marching_cubes_compute_shader_handle_types);
    tg_handle p_compute_shader_handles[3] = { marching_cubes_uniform_buffer_h, isolevel_storage_image_3d_h, marching_cubes_compute_buffer_h };
    tg_compute_shader_bind_input(marching_cubes_compute_shader_h, 0, 3, p_compute_shader_handles);



    const tg_handle_type p_triangles_to_vbo_handle_types[1] = { TG_HANDLE_TYPE_COMPUTE_BUFFER };
    tg_compute_shader_h triangles_to_vbo_compute_shader_h = tg_compute_shader_create("shaders/triangle_fill_vertices.comp", 1, p_triangles_to_vbo_handle_types);
    tg_handle p_triangles_to_vbo_handles[1] = { marching_cubes_compute_buffer_h };
    tg_compute_shader_bind_input(triangles_to_vbo_compute_shader_h, 0, 1, p_triangles_to_vbo_handles);



    test_deferred.p_chunk_meshes = TG_MEMORY_ALLOC(TG_CHUNKS_X * TG_CHUNKS_X * sizeof(*test_deferred.p_chunk_meshes));
    test_deferred.p_chunk_entities = TG_MEMORY_ALLOC(TG_CHUNKS_X * TG_CHUNKS_Z * sizeof(*test_deferred.p_chunk_entities));



    for (u32 chunk_x = 0; chunk_x < TG_CHUNKS_X; chunk_x++)
    {
        for (u32 chunk_z = 0; chunk_z < TG_CHUNKS_Z; chunk_z++)
        {
            p_isolevel_uniform_buffer_data->chunk_index_x = chunk_x;
            p_isolevel_uniform_buffer_data->chunk_index_y = 0;
            p_isolevel_uniform_buffer_data->chunk_index_z = chunk_z;
            tg_compute_shader_dispatch(isolevel_compute_shader_h, TG_CHUNK_VERTEX_COUNT_X, TG_CHUNK_VERTEX_COUNT_Y, TG_CHUNK_VERTEX_COUNT_Z);
            p_marching_cubes_uniform_buffer_data->chunk_index_x = chunk_x;
            p_marching_cubes_uniform_buffer_data->chunk_index_y = 0;
            p_marching_cubes_uniform_buffer_data->chunk_index_z = chunk_z;
            tg_compute_shader_dispatch(marching_cubes_compute_shader_h, TG_CHUNK_VERTEX_COUNT_X - 1, TG_CHUNK_VERTEX_COUNT_X - 1, TG_CHUNK_VERTEX_COUNT_X - 1);
            tg_compute_shader_dispatch(triangles_to_vbo_compute_shader_h, 16875 * (TG_CHUNK_VERTEX_COUNT_X - 1), 1, 1);

            tg_mesh_h chunk_mesh_h = tg_mesh_create_from_storage_buffer(marching_cubes_compute_buffer_h);
            tg_entity chunk_entity = tg_entity_create(&test_deferred.scene, chunk_mesh_h, test_deferred.default_material_h);
            tg_list_insert(&test_deferred.entities, &chunk_entity);

            test_deferred.p_chunk_entities[chunk_z * TG_CHUNKS_X + chunk_x] = chunk_entity;
            test_deferred.p_chunk_meshes[chunk_z * TG_CHUNKS_X + chunk_x] = chunk_mesh_h;
            tg_storage_image_3d_clear(isolevel_storage_image_3d_h);
        }
    }
    tg_compute_shader_destroy(isolevel_compute_shader_h);
    tg_uniform_buffer_destroy(isolevel_uniform_buffer_h);
}

void tg_application_internal_game_3d_update_and_render(f32 delta_ms)
{
    if (tg_input_is_key_down(TG_KEY_K))
    {
        test_deferred.quad_offset += 0.01f * delta_ms;
    }
    if (tg_input_is_key_down(TG_KEY_L))
    {
        test_deferred.quad_offset -= 0.01f * delta_ms;
    }
    v3 quad_offset_translation_z = { 0.0f, 0.0f, test_deferred.quad_offset };
    tg_entity_set_position(&test_deferred.quad_entity, &quad_offset_translation_z);

    test_deferred.dtsum += delta_ms;
    if (test_deferred.dtsum >= 10000.0f)
    {
        test_deferred.dtsum -= 10000.0f;
    }
    const f32 sint = tgm_f32_sin(0.001f * test_deferred.dtsum * 2.0f * (f32)TGM_PI);
    const f32 noise_x = tgm_noise(sint, 0.1f, 0.1f) + 0.5f;
    const f32 noise_y = tgm_noise(0.1f, sint, 0.1f) + 0.5f;
    const f32 noise_z = tgm_noise(0.1f, 0.1f, sint) + 0.5f;
    *test_deferred.p_custom_uniform_buffer_data = (v3){ noise_x, noise_y, noise_z };



    u32 mouse_x;
    u32 mouse_y;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        test_deferred.camera_info.yaw += 0.064f * (f32)((i32)test_deferred.camera_info.last_mouse_x - (i32)mouse_x);
        test_deferred.camera_info.pitch += 0.064f * (f32)((i32)test_deferred.camera_info.last_mouse_y - (i32)mouse_y);
    }
    const m4 camera_rotation = tgm_m4_euler(TGM_TO_RADIANS(test_deferred.camera_info.pitch), TGM_TO_RADIANS(test_deferred.camera_info.yaw), TGM_TO_RADIANS(test_deferred.camera_info.roll));

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
        const f32 camera_base_speed = tg_input_is_key_down(TG_KEY_SHIFT) ? 0.1f : 0.01f;
        const f32 camera_speed = camera_base_speed * delta_ms;
        velocity = tgm_v3_normalized(&velocity);
        velocity = tgm_v3_multiply_f(&velocity, camera_speed);
        test_deferred.camera_info.position = tgm_v3_add_v3(&test_deferred.camera_info.position, &velocity);
    }

    tg_camera_set_view(test_deferred.camera_info.camera_h, &test_deferred.camera_info.position, TGM_TO_RADIANS(test_deferred.camera_info.pitch), TGM_TO_RADIANS(test_deferred.camera_info.yaw), TGM_TO_RADIANS(test_deferred.camera_info.roll));
    //tg_camera_set_view(test_deferred.secondary_camera_h, &test_deferred.camera_info.position, TGM_TO_RADIANS(test_deferred.camera_info.pitch), TGM_TO_RADIANS(test_deferred.camera_info.yaw), TGM_TO_RADIANS(test_deferred.camera_info.roll));
    test_deferred.camera_info.last_mouse_x = mouse_x;
    test_deferred.camera_info.last_mouse_y = mouse_y;

    if (tg_input_get_mouse_wheel_detents(TG_FALSE))
    {
        test_deferred.camera_info.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
        tg_camera_set_perspective_projection(test_deferred.camera_info.camera_h, test_deferred.camera_info.fov_y_in_radians, test_deferred.camera_info.near, test_deferred.camera_info.far);
        //tg_camera_set_perspective_projection(test_deferred.secondary_camera_h, test_deferred.camera_info.fov_y_in_radians, test_deferred.camera_info.near, test_deferred.camera_info.far);
    }

    tg_scene_begin(&test_deferred.scene);
    tg_entity* p_entities = TG_LIST_POINTER_TO(test_deferred.entities, 0);
    for (u32 i = 0; i < test_deferred.entities.count; i++)
    {
        tg_scene_submit(&test_deferred.scene, &p_entities[i]);
    }
    tg_scene_end(&test_deferred.scene);
    tg_camera_present(test_deferred.camera_info.camera_h);
    //tg_camera_present(test_deferred.secondary_camera_h);
}

void tg_application_internal_game_3d_destroy()
{
    tg_texture_atlas_destroy(test_deferred.texture_atlas_h);
    for (u32 i = 0; i < 13; i++)
    {
        tg_color_image_destroy(test_deferred.textures_h[i]);
    }

    for (u32 i = 0; i < TG_CHUNKS_X * TG_CHUNKS_Z; i++)
    {
        tg_entity_destroy(&test_deferred.p_chunk_entities[i]);
        tg_mesh_destroy(test_deferred.p_chunk_meshes[i]);
    }
    TG_MEMORY_FREE(test_deferred.p_chunk_entities);
    TG_MEMORY_FREE(test_deferred.p_chunk_meshes);

    tg_entity_destroy(&test_deferred.quad_entity);
    tg_entity_destroy(&test_deferred.ground_entity);
    tg_mesh_destroy(test_deferred.ground_mesh_h);
    tg_mesh_destroy(test_deferred.quad_mesh_h);

    tg_color_image_destroy(test_deferred.image_h);
    tg_uniform_buffer_destroy(test_deferred.custom_uniform_buffer_h);
    tg_material_destroy(test_deferred.custom_material_h);
    tg_fragment_shader_destroy(test_deferred.custom_fragment_shader_h);
    tg_vertex_shader_destroy(test_deferred.custom_vertex_shader_h);

    tg_material_destroy(test_deferred.default_material_h);
    tg_fragment_shader_destroy(test_deferred.default_fragment_shader_h);
    tg_vertex_shader_destroy(test_deferred.default_vertex_shader_h);

    tg_scene_destroy(&test_deferred.scene);
    tg_camera_destroy(test_deferred.secondary_camera_h);
    tg_camera_destroy(test_deferred.camera_info.camera_h);
    tg_list_destroy(&test_deferred.entities);
}

void tg_application_internal_game_2d_create()
{
    const v3 camera_position = { 0.0f, 0.0f, 0.0f };
    test_deferred.camera_info.camera_h = tg_camera_create_orthographic(&camera_position, 0.0f, 0.0f, 0.0f, -320, 320, -180, 180, -100, 100);
    tg_point_light p_point_lights[TG_POINT_LIGHT_COUNT] = { 0 };
    tg_random random = { 0 };
    tgm_random_init(&random, 13031995);
    for (u32 i = 0; i < TG_POINT_LIGHT_COUNT; i++)
    {
        p_point_lights[i].position = (v3){ tgm_random_next_f32(&random) * 100.0f, tgm_random_next_f32(&random) * 50.0f, -tgm_random_next_f32(&random) * 100.0f };
        p_point_lights[i].color = (v3){ tgm_random_next_f32(&random), tgm_random_next_f32(&random), tgm_random_next_f32(&random) };
        p_point_lights[i].radius = tgm_random_next_f32(&random) * 50.0f;
    }
    test_forward.scene = tg_scene_create(1, &test_forward.camera_h, TG_POINT_LIGHT_COUNT, p_point_lights);
    const v3 p_quad_positions[6] = {
        { -150.0f, -15.0f, 0.0f },
        {   15.0f, -15.0f, 0.0f },
        {   15.0f,  15.0f, 0.0f },
        {   15.0f,  15.0f, 0.0f },
        {  -15.0f,  15.0f, 0.0f },
        { -150.0f, -15.0f, 0.0f }
    };
    const v3 p_quad_positions2[6] = {
        {  -15.0f, -15.0f, -50.0f },
        {  150.0f, -15.0f, -50.0f },
        {   15.0f,  15.0f, -50.0f },
        {   15.0f,  15.0f, -50.0f },
        { -150.0f,  15.0f, -50.0f },
        {  -15.0f, -15.0f, -50.0f }
    };
    test_forward.mesh_h = tg_mesh_create(6, p_quad_positions, TG_NULL, TG_NULL, TG_NULL, 0, TG_NULL);
    test_forward.mesh2_h = tg_mesh_create(6, p_quad_positions2, TG_NULL, TG_NULL, TG_NULL, 0, TG_NULL);
    test_forward.vertex_shader_h = tg_vertex_shader_create("shaders/forward.vert");
    test_forward.vertex_shader2_h = tg_vertex_shader_create("shaders/geometry.vert");
    test_forward.fragment_shader_h = tg_fragment_shader_create("shaders/forward.frag");
    test_forward.fragment_shader2_h = tg_fragment_shader_create("shaders/geometry.frag");
    test_forward.material_h = tg_material_create_forward(test_forward.vertex_shader_h, test_forward.fragment_shader_h, 0, TG_NULL);
    test_forward.material2_h = tg_material_create_deferred(test_forward.vertex_shader2_h, test_forward.fragment_shader2_h, 0, TG_NULL);
    test_forward.entity = tg_entity_create(&test_forward.scene, test_forward.mesh_h, test_forward.material_h);
    test_forward.entity2 = tg_entity_create(&test_forward.scene, test_forward.mesh2_h, test_forward.material2_h);
}

void tg_application_internal_game_2d_update_and_render(f32 delta_ms)
{
    tg_scene_begin(&test_forward.scene);
    tg_scene_submit(&test_forward.scene, &test_forward.entity);
    tg_scene_submit(&test_forward.scene, &test_forward.entity2);
    tg_scene_end(&test_forward.scene);
    tg_camera_present(test_forward.camera_h);
}

void tg_application_internal_game_2d_destroy()
{
    tg_entity_destroy(&test_forward.entity2);
    tg_entity_destroy(&test_forward.entity);
    tg_material_destroy(test_forward.material2_h);
    tg_material_destroy(test_forward.material_h);
    tg_fragment_shader_destroy(test_forward.fragment_shader2_h);
    tg_fragment_shader_destroy(test_forward.fragment_shader_h);
    tg_vertex_shader_destroy(test_forward.vertex_shader2_h);
    tg_vertex_shader_destroy(test_forward.vertex_shader_h);
    tg_mesh_destroy(test_forward.mesh2_h);
    tg_mesh_destroy(test_forward.mesh_h);
    tg_scene_destroy(&test_forward.scene);
    tg_camera_destroy(test_forward.camera_h);
}



void tg_application_start()
{
    tg_graphics_init();
    tg_application_internal_game_3d_create();
    //tg_application_internal_game_2d_create();

#ifdef TG_DEBUG
    tg_debug_info debug_info = { 0 };
#endif

    tg_timer_h timer_h = tg_timer_create();
    tg_timer_start(timer_h);

    /*--------------------------------------------------------+
    | Start main loop                                         |
    +--------------------------------------------------------*/

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
        tg_application_internal_game_3d_update_and_render(delta_ms);
        //tg_application_internal_game_2d_update_and_render(delta_ms);
    }

    /*--------------------------------------------------------+
    | End main loop                                           |
    +--------------------------------------------------------*/
    tg_timer_destroy(timer_h);
    tg_graphics_wait_idle();
    //tg_application_internal_game_2d_destroy();
    tg_application_internal_game_3d_destroy();
    tg_graphics_shutdown();
}

void tg_application_on_window_resize(u32 width, u32 height)
{
}

void tg_application_quit()
{
	running = TG_FALSE;
}

const char* tg_application_get_asset_path()
{
    return asset_path;
}
