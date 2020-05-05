#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "math/tg_math.h"
#include "tg_common.h"
#include "util/tg_list.h"



TG_DECLARE_TYPE(tg_entity);
TG_DECLARE_TYPE(tg_scene);

TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_color_image);
TG_DECLARE_HANDLE(tg_compute_buffer);
TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_deferred_renderer);
TG_DECLARE_HANDLE(tg_depth_image);
TG_DECLARE_HANDLE(tg_entity_graphics_data_ptr);
TG_DECLARE_HANDLE(tg_forward_renderer);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_index_buffer);
TG_DECLARE_HANDLE(tg_render_target);
TG_DECLARE_HANDLE(tg_storage_image_3d);
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

typedef enum tg_handle_type
{
	TG_HANDLE_TYPE_INVALID = 0,
	TG_HANDLE_TYPE_CAMERA,
	TG_HANDLE_TYPE_COLOR_IMAGE,
	TG_HANDLE_TYPE_COMPUTE_BUFFER,
	TG_HANDLE_TYPE_COMPUTE_SHADER,
	TG_HANDLE_TYPE_DEPTH_IMAGE,
	TG_HANDLE_TYPE_ENTITY_GRAPHICS_DATA_PTR,
	TG_HANDLE_TYPE_FRAGMENT_SHADER,
	TG_HANDLE_TYPE_MATERIAL,
	TG_HANDLE_TYPE_MESH,
	TG_HANDLE_TYPE_INDEX_BUFFER,
	TG_HANDLE_TYPE_DEFERRED_RENDERER,
	TG_HANDLE_TYPE_FORWARD_RENDERER,
	TG_HANDLE_TYPE_RENDER_TARGET,
	TG_HANDLE_TYPE_STORAGE_IMAGE_3D,
	TG_HANDLE_TYPE_TEXTURE_ATLAS,
	TG_HANDLE_TYPE_UNIFORM_BUFFER,
	TG_HANDLE_TYPE_VERTEX_BUFFER,
	TG_HANDLE_TYPE_VERTEX_SHADER
} tg_handle_type;

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

typedef enum tg_storage_image_format
{
	TG_STORAGE_IMAGE_FORMAT_R32_SFLOAT,
	TG_STORAGE_IMAGE_FORMAT_R32G32_SFLOAT,
	TG_STORAGE_IMAGE_FORMAT_R32G32B32_SFLOAT,
	TG_STORAGE_IMAGE_FORMAT_R32G32B32A32_SFLOAT
} tg_storage_image_format;



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



void                             tg_graphics_init();
void                             tg_graphics_on_window_resize(u32 width, u32 height);
void                             tg_graphics_wait_idle();
void                             tg_graphics_shutdown();



void                             tg_camera_clear(tg_camera_h camera_h);
tg_camera_h                      tg_camera_create_orthographic(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
tg_camera_h                      tg_camera_create_perspective(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far);
void                             tg_camera_destroy(tg_camera_h camera_h);
tg_render_target_h               tg_camera_get_render_target(tg_camera_h camera_h);
void                             tg_camera_present(tg_camera_h camera_h);
void                             tg_camera_set_orthographic_projection(tg_camera_h camera_h, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
void                             tg_camera_set_perspective_projection(tg_camera_h camera_h, f32 fov_y, f32 near, f32 far);
void                             tg_camera_set_view(tg_camera_h camera_h, const v3* p_position, f32 pitch, f32 yaw, f32 roll);

tg_color_image_h                 tg_color_image_load(const char* p_filename);
tg_color_image_h                 tg_color_image_create(const tg_color_image_create_info* p_color_image_create_info);
void                             tg_color_image_destroy(tg_color_image_h color_image_h);

tg_compute_buffer_h              tg_compute_buffer_create(u64 size, b32 visible);
u64                              tg_compute_buffer_size(tg_compute_buffer_h compute_buffer_h);
void*                            tg_compute_buffer_data(tg_compute_buffer_h compute_buffer_h);
void                             tg_compute_buffer_destroy(tg_compute_buffer_h compute_buffer_h);

void                             tg_compute_shader_bind_input(tg_compute_shader_h compute_shader_h, u32 first_handle_index, u32 handle_count, tg_handle* p_handles);
tg_compute_shader_h              tg_compute_shader_create(const char* filename, u32 handle_type_count, const tg_handle_type* p_handle_types);
void                             tg_compute_shader_dispatch(tg_compute_shader_h compute_shader_h, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void                             tg_compute_shader_destroy(tg_compute_shader_h compute_shader_h);

tg_entity_graphics_data_ptr_h    tg_entity_graphics_data_ptr_create(tg_entity* p_entity, tg_scene* p_scene);
void                             tg_entity_graphics_data_ptr_destroy(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h);
void                             tg_entity_graphics_data_ptr_set_model_matrix(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h, const m4* p_model_matrix);

tg_depth_image_h                 tg_depth_image_create(const tg_depth_image_create_info* p_depth_image_create_info);
void                             tg_depth_image_destroy(tg_depth_image_h depth_image_h);

tg_fragment_shader_h             tg_fragment_shader_create(const char* p_filename);
void                             tg_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h);

tg_material_h                    tg_material_create_deferred(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 handle_count, tg_handle* p_handles);
tg_material_h                    tg_material_create_forward(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 handle_count, tg_handle* p_handles);
void                             tg_material_destroy(tg_material_h material_h);
b32                              tg_material_is_deferred(tg_material_h material_h);
b32                              tg_material_is_forward(tg_material_h material_h);

tg_mesh_h                        tg_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices);
tg_mesh_h                        tg_mesh_create_from_storage_buffer(tg_compute_buffer_h storage_buffer_h);
void                             tg_mesh_destroy(tg_mesh_h mesh_h);

void                             tg_storage_image_3d_copy_to_storage_buffer(tg_storage_image_3d_h storage_image_3d_h, tg_compute_buffer_h compute_buffer_h);
void                             tg_storage_image_3d_clear(tg_storage_image_3d_h storage_image_3d_h);
tg_storage_image_3d_h            tg_storage_image_3d_create(u32 width, u32 height, u32 depth, tg_storage_image_format format);
void                             tg_storage_image_3d_destroy(tg_storage_image_3d_h storage_image_3d_h);

tg_texture_atlas_h               tg_texture_atlas_create_from_images(u32 image_count, tg_color_image_h* p_color_images_h);
void                             tg_texture_atlas_destroy(tg_texture_atlas_h texture_atlas_h);

tg_uniform_buffer_h              tg_uniform_buffer_create(u64 size);
void*                            tg_uniform_buffer_data(tg_uniform_buffer_h uniform_buffer_h);
void                             tg_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h);

tg_vertex_shader_h               tg_vertex_shader_create(const char* p_filename);
void                             tg_vertex_shader_destroy(tg_vertex_shader_h p_vertex_shader_h);



/*------------------------------------------------------------+
| Deferred Renderer                                           |
+------------------------------------------------------------*/

void                             tg_deferred_renderer_begin(tg_deferred_renderer_h deferred_renderer_h);
tg_deferred_renderer_h           tg_deferred_renderer_create(tg_camera_h camera_h, u32 point_light_count, const tg_point_light* p_point_lights);
void                             tg_deferred_renderer_destroy(tg_deferred_renderer_h deferred_renderer_h);
void                             tg_deferred_renderer_draw(tg_deferred_renderer_h deferred_renderer_h, tg_entity* p_entity, u32 entity_graphics_data_ptr_index);
void                             tg_deferred_renderer_end(tg_deferred_renderer_h deferred_renderer_h);
void                             tg_deferred_renderer_on_window_resize(tg_deferred_renderer_h deferred_renderer_h, u32 width, u32 height);



/*------------------------------------------------------------+
| Forward Renderer                                            |
+------------------------------------------------------------*/

void                             tg_forward_renderer_begin(tg_forward_renderer_h forward_renderer_h);
tg_forward_renderer_h            tg_forward_renderer_create(tg_camera_h camera_h);
void                             tg_forward_renderer_destroy(tg_forward_renderer_h forward_renderer_h);
void                             tg_forward_renderer_draw(tg_forward_renderer_h forward_renderer_h, tg_entity* p_entity, u32 entity_graphics_data_ptr_index);
void                             tg_forward_renderer_end(tg_forward_renderer_h forward_renderer_h);
void                             tg_forward_renderer_on_window_resize(tg_forward_renderer_h forward_renderer_h, u32 width, u32 height);

#endif
