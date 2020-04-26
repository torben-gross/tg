#ifndef tgg_H
#define tgg_H

#include "math/tg_math.h"
#include "tg_common.h"
#include "util/tg_list.h"



TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_color_image);
TG_DECLARE_HANDLE(tg_compute_buffer);
TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_depth_image);
TG_DECLARE_HANDLE(tg_entity);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_model);
TG_DECLARE_HANDLE(tg_index_buffer);
TG_DECLARE_HANDLE(tg_renderer_3d);
TG_DECLARE_HANDLE(tg_texture_atlas);
TG_DECLARE_HANDLE(tg_uniform_buffer);
TG_DECLARE_HANDLE(tg_vertex_buffer);
TG_DECLARE_HANDLE(tg_vertex_shader);

typedef void* tg_handle;



typedef enum tg_color_image_format
{
	TG_COLOR_IMAGE_FORMAT_A8B8G8R8,
	TG_COLOR_IMAGE_FORMAT_A8R8G8B8,
	TG_COLOR_IMAGE_FORMAT_B8G8R8A8,
	TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT,
	TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT,
	TG_COLOR_IMAGE_FORMAT_R8,
	TG_COLOR_IMAGE_FORMAT_R8G8,
	TG_COLOR_IMAGE_FORMAT_R8G8B8,
	TG_COLOR_IMAGE_FORMAT_R8G8B8A8
} tg_color_image_format;

typedef enum tg_depth_image_format
{
	TG_DEPTH_IMAGE_FORMAT_D16_UNORM,
	TG_DEPTH_IMAGE_FORMAT_D32_SFLOAT
} tg_depth_image_format;

typedef enum tg_image_address_mode
{
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER,
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE,
	TG_IMAGE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
	TG_IMAGE_ADDRESS_MODE_MIRRORED_REPEAT,
	TG_IMAGE_ADDRESS_MODE_REPEAT
} tg_image_address_mode;

typedef enum tg_image_filter
{
	TG_IMAGE_FILTER_LINEAR,
	TG_IMAGE_FILTER_NEAREST
} tg_image_filter;

typedef enum tg_shader_input_element_type
{
	TG_SHADER_INPUT_ELEMENT_TYPE_COLOR_IMAGE,
	TG_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER,
	TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER
} tg_shader_input_element_type;



typedef struct tg_color_image_create_info
{
	u32                      width;
	u32                      height;
	u32                      mip_levels;
	tg_color_image_format    format;
	tg_image_filter          min_filter;
	tg_image_filter          mag_filter;
	tg_image_address_mode    address_mode_u;
	tg_image_address_mode    address_mode_v;
	tg_image_address_mode    address_mode_w;
} tg_color_image_create_info;

typedef struct tg_depth_image_create_info
{
	u32                      width;
	u32                      height;
	tg_depth_image_format    format;
	tg_image_filter          min_filter;
	tg_image_filter          mag_filter;
	tg_image_address_mode    address_mode_u;
	tg_image_address_mode    address_mode_v;
	tg_image_address_mode    address_mode_w;
} tg_depth_image_create_info;

typedef struct tg_shader_input_element
{
	tg_shader_input_element_type    type;
	u32                             array_element_count;
} tg_shader_input_element;

typedef struct tg_vertex_3d
{
	v3    position;
	v3    normal;
	v2    uv;
	v3    tangent;
	v3    bitangent;
} tg_vertex_3d;

typedef struct tg_point_light
{
	v3    position;
	v3    color;
	f32   radius;
} tg_point_light;



void                    tgg_init();
void                    tgg_on_window_resize(u32 width, u32 height);
//void                    tgg_present(tg_image_h image_h); // TODO: this
void                    tgg_shutdown();



tg_camera_h             tgg_camera_create(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far);
m4                      tgg_camera_get_projection(tg_camera_h camera_h);
m4                      tgg_camera_get_view(tg_camera_h camera_h);
void                    tgg_camera_set_projection(f32 fov_y, f32 near, f32 far, tg_camera_h camera_h);
void                    tgg_camera_set_view(const v3* p_position, f32 pitch, f32 yaw, f32 roll, tg_camera_h camera_h);
void                    tgg_camera_destroy(tg_camera_h camera_h);

tg_color_image_h        tgg_color_image_load(const char* p_filename);
tg_color_image_h        tgg_color_image_create(const tg_color_image_create_info* p_color_image_create_info);
void                    tgg_color_image_destroy(tg_color_image_h color_image_h);

tg_depth_image_h        tgg_depth_image_create(const tg_depth_image_create_info* p_depth_image_create_info);
void                    tgg_depth_image_destroy(tg_depth_image_h depth_image_h);

tg_compute_buffer_h     tgg_compute_buffer_create(u64 size);
void*                   tgg_compute_buffer_data(tg_compute_buffer_h compute_buffer_h);
void                    tgg_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h);

tg_compute_shader_h     tgg_compute_shader_create(const char* filename, u32 input_element_count, tg_shader_input_element* p_shader_input_elements);
void                    tgg_compute_shader_bind_input(tg_compute_shader_h compute_shader_h, tg_handle* p_shader_input_element_handles);
void                    tgg_compute_shader_dispatch(tg_compute_shader_h compute_shader_h, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void                    tgg_compute_shader_destroy(tg_compute_shader_h compute_shader_h);

tg_fragment_shader_h    tgg_fragment_shader_create(const char* p_filename);
void                    tgg_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h);

tg_material_h           tgg_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 input_element_count, tg_shader_input_element* p_input_elements, tg_handle* p_input_element_handles);
void                    tgg_material_destroy(tg_material_h material_h);

tg_mesh_h               tgg_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices);
void                    tgg_mesh_destroy(tg_mesh_h mesh_h);

tg_model_h              tgg_model_create(tg_mesh_h mesh_h, tg_material_h material_h);
void                    tgg_model_destroy(tg_model_h model_h);

tg_texture_atlas_h      tgg_texture_atlas_create_from_images(u32 image_count, tg_color_image_h* p_color_images_h);
void                    tgg_texture_atlas_destroy(tg_texture_atlas_h texture_atlas_h);

tg_uniform_buffer_h     tgg_uniform_buffer_create(u64 size);
void*                   tgg_uniform_buffer_data(tg_uniform_buffer_h uniform_buffer_h);
void                    tgg_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h);

tg_vertex_shader_h      tgg_vertex_shader_create(const char* p_filename);
void                    tgg_vertex_shader_destroy(tg_vertex_shader_h p_vertex_shader_h);



/*------------------------------------------------------------+
| 3D Renderer                                                 |
+------------------------------------------------------------*/

tg_renderer_3d_h        tgg_renderer_3d_create(const tg_camera_h camera_h, tg_color_image_h render_target, u32 point_light_count, const tg_point_light* p_point_lights);
void                    tgg_renderer_3d_register(tg_renderer_3d_h renderer_3d_h, tg_entity_h entity_h);
void                    tgg_renderer_3d_begin(tg_renderer_3d_h renderer_3d_h);
void                    tgg_renderer_3d_draw_entity(tg_renderer_3d_h renderer_3d_h, tg_entity_h entity_h);
void                    tgg_renderer_3d_end(tg_renderer_3d_h renderer_3d_h);
void                    tgg_renderer_3d_present(tg_renderer_3d_h renderer_3d_h);
void                    tgg_renderer_3d_shutdown(tg_renderer_3d_h renderer_3d_h);
void                    tgg_renderer_3d_on_window_resize(tg_renderer_3d_h renderer_3d_h, u32 w, u32 h);

#endif
