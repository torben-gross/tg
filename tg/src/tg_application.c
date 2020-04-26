#include "tg_application.h"



#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_entity.h"
#include "tg_input.h"
#include "tg_marching_cubes.h"
#include "util/tg_list.h"
#include "util/tg_qsort.h"
#include "util/tg_rectangle_packer.h"
#include "util/tg_string.h"
#include "util/tg_timer.h"



#define TG_POINT_LIGHT_COUNT    64
#define TG_CHUNKS_X             4
#define TG_CHUNKS_Z             4



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

typedef struct tg_chunk_uniform_buffer
{
    u32    chunk_vertex_count_x;
    u32    chunk_vertex_count_y;
    u32    chunk_vertex_count_z;
    u32    chunk_coordinate_x;
    u32    chunk_coordinate_y;
    u32    chunk_coordinate_z;
    f32    cell_stride_x;
    f32    cell_stride_y;
    f32    cell_stride_z;
    f32    noise_scale_x;
    f32    noise_scale_y;
    f32    noise_scale_z;
} tg_chunk_uniform_buffer;

typedef struct tg_test_3d
{
    tg_list_h                      entities;
    tg_camera_info                 camera_info;
    tg_renderer_3d_h               renderer_3d_h;
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
    tg_model_h                     quad_model_h;
    tg_entity_h                    quad_entity_h;
    tg_mesh_h                      ground_mesh_h;
    tg_model_h                     ground_model_h;
    tg_entity_h                    ground_entity_h;
    tg_mesh_h*                     p_chunk_meshes;
    tg_model_h*                    p_chunk_models;
    tg_entity_h*                   p_chunk_entities;
    f32                            quad_offset;
    f32                            dtsum;
} tg_test_3d;



b32 running = TG_TRUE;
const char* asset_path = "assets"; // TODO: determine this some other way
tg_test_3d test_3d = { 0 };



void tg_application_internal_test_3d_create()
{
    test_3d.entities = TG_LIST_CREATE(tg_entity_h);
    test_3d.camera_info.position = (v3){ 0.0f, 0.0f, 0.0f };
    test_3d.camera_info.pitch = 0.0f;
    test_3d.camera_info.yaw = 0.0f;
    test_3d.camera_info.roll = 0.0f;
    test_3d.camera_info.fov_y_in_radians = TGM_TO_RADIANS(70.0f);
    test_3d.camera_info.aspect = tg_platform_get_window_aspect_ratio(&test_3d.camera_info.aspect);
    test_3d.camera_info.near = -0.1f;
    test_3d.camera_info.far = -1000.0f;
    tg_input_get_mouse_position(&test_3d.camera_info.last_mouse_x, &test_3d.camera_info.last_mouse_y);
    test_3d.camera_info.camera_h = tgg_camera_create(&test_3d.camera_info.position, test_3d.camera_info.pitch, test_3d.camera_info.yaw, test_3d.camera_info.roll, test_3d.camera_info.fov_y_in_radians, test_3d.camera_info.near, test_3d.camera_info.far);

    tg_point_light p_point_lights[TG_POINT_LIGHT_COUNT] = { 0 };
    tg_random random = { 0 };
    tgm_random_init(&random, 13031995);
    for (u32 i = 0; i < TG_POINT_LIGHT_COUNT; i++)
    {
        p_point_lights[i].position = (v3){ tgm_random_next_f32(&random) * 100.0f, tgm_random_next_f32(&random) * 50.0f, -tgm_random_next_f32(&random) * 100.0f };
        p_point_lights[i].color = (v3){ tgm_random_next_f32(&random), tgm_random_next_f32(&random), tgm_random_next_f32(&random) };
        p_point_lights[i].radius = tgm_random_next_f32(&random) * 50.0f;
    }
    tg_color_image_create_info image_create_info = { 0 };
    {
        tg_platform_get_window_size(&image_create_info.width, &image_create_info.height);
        image_create_info.mip_levels = 1;
        image_create_info.format = TG_COLOR_IMAGE_FORMAT_B8G8R8A8;
        image_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
        image_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
        image_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_REPEAT;
        image_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_REPEAT;
        image_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_REPEAT;
    }
    test_3d.renderer_3d_h = tgg_renderer_3d_create(test_3d.camera_info.camera_h, TG_POINT_LIGHT_COUNT, p_point_lights);





    for (u32 i = 0; i < 9; i++)
    {
        char image_buffer[256] = { 0 };
        tg_string_format(sizeof(image_buffer), image_buffer, "textures/%i.bmp", i + 1);
        test_3d.textures_h[i] = tgg_color_image_load(image_buffer);
    }
    test_3d.textures_h[9] = tgg_color_image_load("textures/cabin_final.bmp");
    test_3d.textures_h[10] = tgg_color_image_load("textures/test_icon.bmp");
    test_3d.textures_h[11] = tgg_color_image_load("textures/pbb_birch.bmp");
    test_3d.textures_h[12] = tgg_color_image_load("textures/squirrel.bmp");
    test_3d.texture_atlas_h = tgg_texture_atlas_create_from_images(13, test_3d.textures_h);




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

    test_3d.quad_mesh_h = tgg_mesh_create(4, p_quad_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    test_3d.default_vertex_shader_h = tgg_vertex_shader_create("shaders/geometry.vert");
    test_3d.default_fragment_shader_h = tgg_fragment_shader_create("shaders/geometry.frag");
    test_3d.default_material_h = tgg_material_create(test_3d.default_vertex_shader_h, test_3d.default_fragment_shader_h, 0, TG_NULL, TG_NULL);

    test_3d.custom_uniform_buffer_h = tgg_uniform_buffer_create(sizeof(v3));
    test_3d.p_custom_uniform_buffer_data = tgg_uniform_buffer_data(test_3d.custom_uniform_buffer_h);
    *test_3d.p_custom_uniform_buffer_data = (v3){ 1.0f, 0.0f, 0.0f };
    test_3d.image_h = tgg_color_image_load("textures/test_icon.bmp");
    tg_shader_input_element p_shader_input_elements[2] = { 0 };
    {
        p_shader_input_elements[0].type = TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER;
        p_shader_input_elements[0].array_element_count = 1;
        p_shader_input_elements[1].type = TG_SHADER_INPUT_ELEMENT_TYPE_COLOR_IMAGE;
        p_shader_input_elements[1].array_element_count = 1;
    }
    tg_handle p_custom_handles[2] = { test_3d.custom_uniform_buffer_h, test_3d.texture_atlas_h };
    test_3d.custom_vertex_shader_h = tgg_vertex_shader_create("shaders/custom_geometry.vert");
    test_3d.custom_fragment_shader_h = tgg_fragment_shader_create("shaders/custom_geometry.frag");
    test_3d.custom_material_h = tgg_material_create(test_3d.custom_vertex_shader_h, test_3d.custom_fragment_shader_h, 2, p_shader_input_elements, p_custom_handles);

    test_3d.quad_model_h = tgg_model_create(test_3d.quad_mesh_h, test_3d.custom_material_h);
    test_3d.quad_entity_h = tg_entity_create(test_3d.renderer_3d_h, test_3d.quad_model_h);
    test_3d.quad_offset = -65.0f;
    tg_list_insert(test_3d.entities, &test_3d.quad_entity_h);

    test_3d.ground_mesh_h = tgg_mesh_create(4, p_ground_positions, TG_NULL, p_uvs, TG_NULL, 6, p_indices);
    test_3d.ground_model_h = tgg_model_create(test_3d.ground_mesh_h, test_3d.default_material_h);
    test_3d.ground_entity_h = tg_entity_create(test_3d.renderer_3d_h, test_3d.ground_model_h);
    tg_list_insert(test_3d.entities, &test_3d.ground_entity_h);







    const u32 chunk_vertex_count = 32;
    const f32 cell_stride = 1.1f;
    const f32 noise_scale = 0.1f;


    tg_uniform_buffer_h chunk_uniform_buffer_h = tgg_uniform_buffer_create(sizeof(tg_chunk_uniform_buffer));
    tg_chunk_uniform_buffer* p_chunk_uniform_buffer_data = tgg_uniform_buffer_data(chunk_uniform_buffer_h);
    {
        p_chunk_uniform_buffer_data->chunk_vertex_count_x = chunk_vertex_count;
        p_chunk_uniform_buffer_data->chunk_vertex_count_y = chunk_vertex_count;
        p_chunk_uniform_buffer_data->chunk_vertex_count_z = chunk_vertex_count;
        p_chunk_uniform_buffer_data->cell_stride_x = cell_stride;
        p_chunk_uniform_buffer_data->cell_stride_y = cell_stride;
        p_chunk_uniform_buffer_data->cell_stride_z = cell_stride;
        p_chunk_uniform_buffer_data->noise_scale_x = noise_scale;
        p_chunk_uniform_buffer_data->noise_scale_y = noise_scale;
        p_chunk_uniform_buffer_data->noise_scale_z = noise_scale;
    }
    tg_compute_buffer_h isolevel_buffer_h = tgg_compute_buffer_create((u64)p_chunk_uniform_buffer_data->chunk_vertex_count_x * (u64)p_chunk_uniform_buffer_data->chunk_vertex_count_y * (u64)p_chunk_uniform_buffer_data->chunk_vertex_count_z * sizeof(f32));
    f32* p_isolevel_buffer_data = tgg_compute_buffer_data(isolevel_buffer_h);
    tg_shader_input_element p_isolevel_compute_shader_input_elements[2] = { 0 };
    {
        p_isolevel_compute_shader_input_elements[0].type = TG_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER;
        p_isolevel_compute_shader_input_elements[0].array_element_count = 1;

        p_isolevel_compute_shader_input_elements[1].type = TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER;
        p_isolevel_compute_shader_input_elements[1].array_element_count = 1;
    }
    tg_compute_shader_h isolevel_compute_shader_h = tgg_compute_shader_create("shaders/isolevel.comp", 2, p_isolevel_compute_shader_input_elements);
    tg_handle pp_handles[2] = { isolevel_buffer_h, chunk_uniform_buffer_h };
    tgg_compute_shader_bind_input(isolevel_compute_shader_h, pp_handles);

    tg_marching_cubes_triangle* p_chunk_triangles = TG_MEMORY_ALLOC((u64)chunk_vertex_count * (u64)chunk_vertex_count * (u64)chunk_vertex_count * 12 * sizeof(*p_chunk_triangles));
    test_3d.p_chunk_meshes = TG_MEMORY_ALLOC((u64)TG_CHUNKS_X * (u64)TG_CHUNKS_X * sizeof(*test_3d.p_chunk_meshes));
    test_3d.p_chunk_models = TG_MEMORY_ALLOC((u64)TG_CHUNKS_X * (u64)TG_CHUNKS_Z * sizeof(*test_3d.p_chunk_models));
    test_3d.p_chunk_entities = TG_MEMORY_ALLOC((u64)TG_CHUNKS_X * (u64)TG_CHUNKS_Z * sizeof(*test_3d.p_chunk_entities));
    for (u32 chunk_x = 0; chunk_x < TG_CHUNKS_X; chunk_x++)
    {
        for (u32 chunk_z = 0; chunk_z < TG_CHUNKS_Z; chunk_z++)
        {
            p_chunk_uniform_buffer_data->chunk_coordinate_x = chunk_x;
            p_chunk_uniform_buffer_data->chunk_coordinate_y = 0;
            p_chunk_uniform_buffer_data->chunk_coordinate_z = chunk_z;
            tgg_compute_shader_dispatch(isolevel_compute_shader_h, p_chunk_uniform_buffer_data->chunk_vertex_count_x, p_chunk_uniform_buffer_data->chunk_vertex_count_y, p_chunk_uniform_buffer_data->chunk_vertex_count_z);

            u32 triangle_count = 0;
            for (u32 z = 0; z < chunk_vertex_count - 1; z++)
            {
                for (u32 y = 0; y < chunk_vertex_count - 1; y++)
                {
                    for (u32 x = 0; x < chunk_vertex_count - 1; x++)
                    {
                        tg_marching_cubes_grid_cell grid_cell = { 0 };
                        {
                            grid_cell.positions[0] = (v3){ (f32)x * cell_stride, (f32)y * cell_stride, -(f32)(z + 1) * cell_stride };
                            grid_cell.positions[1] = (v3){ (f32)(x + 1) * cell_stride, (f32)y * cell_stride, -(f32)(z + 1) * cell_stride };
                            grid_cell.positions[2] = (v3){ (f32)(x + 1) * cell_stride, (f32)y * cell_stride, -(f32)z * cell_stride };
                            grid_cell.positions[3] = (v3){ (f32)x * cell_stride, (f32)y * cell_stride, -(f32)z * cell_stride };
                            grid_cell.positions[4] = (v3){ (f32)x * cell_stride, (f32)(y + 1) * cell_stride, -(f32)(z + 1) * cell_stride };
                            grid_cell.positions[5] = (v3){ (f32)(x + 1) * cell_stride, (f32)(y + 1) * cell_stride, -(f32)(z + 1) * cell_stride };
                            grid_cell.positions[6] = (v3){ (f32)(x + 1) * cell_stride, (f32)(y + 1) * cell_stride, -(f32)z * cell_stride };
                            grid_cell.positions[7] = (v3){ (f32)x * cell_stride, (f32)(y + 1) * cell_stride, -(f32)z * cell_stride };

                            grid_cell.values[0] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z - 1) + chunk_vertex_count * y + x];
                            grid_cell.values[1] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z - 1) + chunk_vertex_count * y + (x + 1)];
                            grid_cell.values[2] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z) + chunk_vertex_count * y + (x + 1)];
                            grid_cell.values[3] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z) + chunk_vertex_count * y + x];
                            grid_cell.values[4] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z - 1) + chunk_vertex_count * (y + 1) + x];
                            grid_cell.values[5] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z - 1) + chunk_vertex_count * (y + 1) + (x + 1)];
                            grid_cell.values[6] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z) + chunk_vertex_count * (y + 1) + (x + 1)];
                            grid_cell.values[7] = p_isolevel_buffer_data[chunk_vertex_count * chunk_vertex_count * ((chunk_vertex_count - 1) - z) + chunk_vertex_count * (y + 1) + x];
                        }
                        triangle_count += tg_marching_cubes_polygonise(&grid_cell, -0.2f, &p_chunk_triangles[triangle_count]);
                    }
                }
            }

            if (triangle_count != 0)
            {
                const v3* p_chunk_positions = (v3*)p_chunk_triangles;
                const u32 chunk_total_vertex_count = 3 * triangle_count;

                tg_mesh_h chunk_mesh_h = tgg_mesh_create(chunk_total_vertex_count, p_chunk_positions, TG_NULL, TG_NULL, TG_NULL, 0, TG_NULL);
                tg_model_h chunk_model_h = tgg_model_create(chunk_mesh_h, test_3d.default_material_h);
                tg_entity_h chunk_entity_h = tg_entity_create(test_3d.renderer_3d_h, chunk_model_h);
                tg_list_insert(test_3d.entities, &chunk_entity_h);
                v3 chunk_translation = { (f32)(chunk_vertex_count - 1) * cell_stride * (f32)chunk_x, 0.0f, -(f32)(chunk_vertex_count - 1) * cell_stride * (f32)chunk_z }; // TODO: could be baked in
                tg_entity_set_position(chunk_entity_h, &chunk_translation);

                test_3d.p_chunk_entities[chunk_z * TG_CHUNKS_X + chunk_x] = chunk_entity_h;
                test_3d.p_chunk_meshes[chunk_z * TG_CHUNKS_X + chunk_x] = chunk_mesh_h;
                test_3d.p_chunk_models[chunk_z * TG_CHUNKS_X + chunk_x] = chunk_model_h;
            }
        }
    }
    tgg_compute_shader_destroy(isolevel_compute_shader_h);
    tgg_compute_buffer_destroy(isolevel_buffer_h);
    tgg_uniform_buffer_destroy(chunk_uniform_buffer_h);

    TG_MEMORY_FREE(p_chunk_triangles);
}

void tg_application_internal_test_3d_update_and_render(f32 delta_ms)
{
    if (tg_input_is_key_down(TG_KEY_K))
    {
        test_3d.quad_offset += 0.01f * delta_ms;
    }
    if (tg_input_is_key_down(TG_KEY_L))
    {
        test_3d.quad_offset -= 0.01f * delta_ms;
    }
    v3 quad_offset_translation_z = { 0.0f, 0.0f, test_3d.quad_offset };
    tg_entity_set_position(test_3d.quad_entity_h, &quad_offset_translation_z);

    test_3d.dtsum += delta_ms;
    if (test_3d.dtsum >= 10000.0f)
    {
        test_3d.dtsum -= 10000.0f;
    }
    const f32 sint = tgm_f32_sin(0.001f * test_3d.dtsum * 2.0f * (f32)TGM_PI);
    const f32 noise_x = tgm_noise(sint, 0.1f, 0.1f) + 0.5f;
    const f32 noise_y = tgm_noise(0.1f, sint, 0.1f) + 0.5f;
    const f32 noise_z = tgm_noise(0.1f, 0.1f, sint) + 0.5f;
    *test_3d.p_custom_uniform_buffer_data = (v3){ noise_x, noise_y, noise_z };



    u32 mouse_x;
    u32 mouse_y;
    tg_input_get_mouse_position(&mouse_x, &mouse_y);
    if (tg_input_is_mouse_button_down(TG_BUTTON_LEFT))
    {
        test_3d.camera_info.yaw += 0.064f * (f32)((i32)test_3d.camera_info.last_mouse_x - (i32)mouse_x);
        test_3d.camera_info.pitch += 0.064f * (f32)((i32)test_3d.camera_info.last_mouse_y - (i32)mouse_y);
    }
    const m4 camera_rotation = tgm_m4_euler(TGM_TO_RADIANS(test_3d.camera_info.pitch), TGM_TO_RADIANS(test_3d.camera_info.yaw), TGM_TO_RADIANS(test_3d.camera_info.roll));

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
        test_3d.camera_info.position = tgm_v3_add_v3(&test_3d.camera_info.position, &velocity);
    }

    tgg_camera_set_view(&test_3d.camera_info.position, TGM_TO_RADIANS(test_3d.camera_info.pitch), TGM_TO_RADIANS(test_3d.camera_info.yaw), TGM_TO_RADIANS(test_3d.camera_info.roll), test_3d.camera_info.camera_h);
    test_3d.camera_info.last_mouse_x = mouse_x;
    test_3d.camera_info.last_mouse_y = mouse_y;

    if (tg_input_get_mouse_wheel_detents(TG_FALSE))
    {
        test_3d.camera_info.fov_y_in_radians -= 0.1f * tg_input_get_mouse_wheel_detents(TG_TRUE);
        tgg_camera_set_projection(test_3d.camera_info.fov_y_in_radians, test_3d.camera_info.near, test_3d.camera_info.far, test_3d.camera_info.camera_h);
    }

    tgg_renderer_3d_begin(test_3d.renderer_3d_h);
    const u32 entity_count = tg_list_count(test_3d.entities);
    tg_entity_h* p_entities = tg_list_pointer_to(test_3d.entities, 0);
    for (u32 i = 0; i < entity_count; i++)
    {
        tgg_renderer_3d_draw_entity(test_3d.renderer_3d_h, p_entities[i]);
    }
    tgg_renderer_3d_end(test_3d.renderer_3d_h);
    tgg_renderer_3d_present(test_3d.renderer_3d_h);
}

void tg_application_internal_test_3d_destroy()
{
    tgg_texture_atlas_destroy(test_3d.texture_atlas_h);
    for (u32 i = 0; i < 13; i++)
    {
        tgg_color_image_destroy(test_3d.textures_h[i]);
    }

    for (u32 i = 0; i < TG_CHUNKS_X * TG_CHUNKS_Z; i++)
    {
        if (test_3d.p_chunk_entities[i])
        {
            tg_entity_destroy(test_3d.p_chunk_entities[i]);
            tgg_model_destroy(test_3d.p_chunk_models[i]);
            tgg_mesh_destroy(test_3d.p_chunk_meshes[i]);
        }
    }
    TG_MEMORY_FREE(test_3d.p_chunk_entities);
    TG_MEMORY_FREE(test_3d.p_chunk_models);
    TG_MEMORY_FREE(test_3d.p_chunk_meshes);

    tg_entity_destroy(test_3d.quad_entity_h);
    tg_entity_destroy(test_3d.ground_entity_h);
    tgg_model_destroy(test_3d.ground_model_h);
    tgg_mesh_destroy(test_3d.ground_mesh_h);
    tgg_model_destroy(test_3d.quad_model_h);
    tgg_mesh_destroy(test_3d.quad_mesh_h);

    tgg_color_image_destroy(test_3d.image_h);
    tgg_uniform_buffer_destroy(test_3d.custom_uniform_buffer_h);
    tgg_material_destroy(test_3d.custom_material_h);
    tgg_fragment_shader_destroy(test_3d.custom_fragment_shader_h);
    tgg_vertex_shader_destroy(test_3d.custom_vertex_shader_h);

    tgg_material_destroy(test_3d.default_material_h);
    tgg_fragment_shader_destroy(test_3d.default_fragment_shader_h);
    tgg_vertex_shader_destroy(test_3d.default_vertex_shader_h);

    tgg_camera_destroy(test_3d.camera_info.camera_h);
    tg_list_destroy(test_3d.entities);
    tgg_renderer_3d_shutdown(test_3d.renderer_3d_h);
}



void tg_application_start()
{
    tgg_init();
    tg_application_internal_test_3d_create();

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
        tg_application_internal_test_3d_update_and_render(delta_ms);
    }

    /*--------------------------------------------------------+
    | End main loop                                           |
    +--------------------------------------------------------*/
    tg_timer_destroy(timer_h);
    tg_application_internal_test_3d_destroy();
    tgg_shutdown();
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
