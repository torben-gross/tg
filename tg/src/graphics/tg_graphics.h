#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "math/tg_math.h"
#include "tg_common.h"



#define TG_MAX_COMPUTE_SHADERS            16
#define TG_MAX_FRAGMENT_SHADERS           32
#define TG_MAX_MATERIALS                  512
#define TG_MAX_MESHES                     65536
#define TG_MAX_RENDER_COMMANDS            65536
#define TG_MAX_RENDERERS                  4
#define TG_MAX_STORAGE_BUFFERS            32
#define TG_MAX_UNIFORM_BUFFERS            256
#define TG_MAX_VERTEX_SHADERS             32

#define TG_MAX_SHADER_ATTACHMENTS         8
#define TG_MAX_SHADER_GLOBAL_RESOURCES    32
#define TG_MAX_SHADER_INPUTS              32
#define TG_SHADER_RESERVED_BINDINGS       2

#define TG_CASCADED_SHADOW_MAPS           3
#define TG_CASCADED_SHADOW_MAP_SIZE       1024

#define TG_MAX_DIRECTIONAL_LIGHTS         512
#define TG_MAX_POINT_LIGHTS               512



TG_DECLARE_HANDLE(tg_color_image);
TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_deferred_renderer);
TG_DECLARE_HANDLE(tg_depth_image);
TG_DECLARE_HANDLE(tg_forward_renderer);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_index_buffer);
TG_DECLARE_HANDLE(tg_render_command);
TG_DECLARE_HANDLE(tg_render_target);
TG_DECLARE_HANDLE(tg_renderer);
TG_DECLARE_HANDLE(tg_storage_buffer);
TG_DECLARE_HANDLE(tg_storage_image_3d);
TG_DECLARE_HANDLE(tg_transvoxel_terrain);
TG_DECLARE_HANDLE(tg_texture_atlas);
TG_DECLARE_HANDLE(tg_uniform_buffer);
TG_DECLARE_HANDLE(tg_vertex_buffer);
TG_DECLARE_HANDLE(tg_vertex_shader);

typedef void* tg_handle;



typedef enum tg_camera_type
{
	TG_CAMERA_TYPE_ORTHOGRAPHIC,
	TG_CAMERA_TYPE_PERSPECTIVE
} tg_camera_type;

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
	TG_HANDLE_TYPE_STORAGE_BUFFER,
	TG_HANDLE_TYPE_COLOR_IMAGE,
	TG_HANDLE_TYPE_COMPUTE_SHADER,
	TG_HANDLE_TYPE_DEPTH_IMAGE,
	TG_HANDLE_TYPE_FRAGMENT_SHADER,
	TG_HANDLE_TYPE_MATERIAL,
	TG_HANDLE_TYPE_MESH,
	TG_HANDLE_TYPE_INDEX_BUFFER,
	TG_HANDLE_TYPE_DEFERRED_RENDERER,
	TG_HANDLE_TYPE_FORWARD_RENDERER,
	TG_HANDLE_TYPE_RENDER_COMMAND,
	TG_HANDLE_TYPE_RENDER_TARGET,
	TG_HANDLE_TYPE_RENDERER,
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



typedef struct tg_camera
{
	tg_camera_type    type;
	v3                position;
	f32               pitch;
	f32               yaw;
	f32               roll;
	union
	{
		struct
		{
			f32       left;
			f32       right;
			f32       bottom;
			f32       top;
			f32       far;
			f32       near;
		} orthographic;
		struct
		{
			f32       fov_y_in_radians;
			f32       aspect;
			f32       near;
			f32       far;
		} perspective;
	};
} tg_camera;

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

typedef struct tg_bounds // TODO: this should be part of physics
{
	v3    min;
	v3    max;
} tg_bounds;

typedef struct tg_vertex
{
	v3    position;
	v3    normal;
	v2    uv;
	v3    tangent;
	v3    bitangent;
} tg_vertex;



void                             tg_graphics_init();
void                             tg_graphics_on_window_resize(u32 width, u32 height);
void                             tg_graphics_shutdown();
void                             tg_graphics_wait_idle();



tg_color_image_h                 tg_color_image_create(const char* p_filename);
tg_color_image_h                 tg_color_image_create_empty(const tg_color_image_create_info* p_color_image_create_info);
void                             tg_color_image_destroy(tg_color_image_h h_color_image);

void                             tg_compute_shader_bind_input(tg_compute_shader_h h_compute_shader, u32 first_handle_index, u32 handle_count, tg_handle* p_handles);
tg_compute_shader_h              tg_compute_shader_create(const char* filename);
void                             tg_compute_shader_dispatch(tg_compute_shader_h h_compute_shader, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void                             tg_compute_shader_destroy(tg_compute_shader_h h_compute_shader);
tg_compute_shader_h              tg_compute_shader_get(const char* filename);

tg_depth_image_h                 tg_depth_image_create(const tg_depth_image_create_info* p_depth_image_create_info);
void                             tg_depth_image_destroy(tg_depth_image_h h_depth_image);

tg_fragment_shader_h             tg_fragment_shader_create(const char* p_filename);
void                             tg_fragment_shader_destroy(tg_fragment_shader_h h_fragment_shader);
tg_fragment_shader_h             tg_fragment_shader_get(const char* p_filename);

tg_material_h                    tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
tg_material_h                    tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
void                             tg_material_destroy(tg_material_h h_material);
b32                              tg_material_is_deferred(tg_material_h h_material);
b32                              tg_material_is_forward(tg_material_h h_material);

tg_mesh_h                        tg_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices);
tg_mesh_h                        tg_mesh_create2(u32 vertex_count, u32 vertex_stride, const void* p_vertices, u32 index_count, const u16* p_indices);
tg_mesh_h                        tg_mesh_create_empty(u32 vertex_capacity, u32 index_capacity);
tg_mesh_h                        tg_mesh_create_sphere(f32 radius, u32 sector_count, u32 stack_count);
void                             tg_mesh_destroy(tg_mesh_h h_mesh);
void                             tg_mesh_update(tg_mesh_h h_mesh, u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices);
void                             tg_mesh_update2(tg_mesh_h h_mesh, u32 vertex_count, u32 vertex_stride, const void* p_vertices, u32 index_count, const u16* p_indices); // TODO: this needs to set a flag or a time, so that the camera knows, that it needs a reset

tg_renderer_h                    tg_renderer_create(tg_camera* p_camera);
void                             tg_renderer_destroy(tg_renderer_h h_renderer);
void                             tg_renderer_enable_shadows(tg_renderer_h h_renderer, b32 enable);
void                             tg_renderer_begin(tg_renderer_h h_renderer);
void                             tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color);
void                             tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color);
void                             tg_renderer_execute(tg_renderer_h h_renderer, tg_render_command_h h_render_command);
void                             tg_renderer_end(tg_renderer_h h_renderer);
void                             tg_renderer_present(tg_renderer_h h_renderer);
void                             tg_renderer_clear(tg_renderer_h h_renderer);
tg_render_target_h               tg_renderer_get_render_target(tg_renderer_h h_renderer);

tg_storage_buffer_h              tg_storage_buffer_create(u64 size, b32 visible);
u64                              tg_storage_buffer_size(tg_storage_buffer_h h_storage_buffer);
void*                            tg_storage_buffer_data(tg_storage_buffer_h h_storage_buffer);
void                             tg_storage_buffer_destroy(tg_storage_buffer_h h_storage_buffer);

void                             tg_storage_image_3d_copy_to_storage_buffer(tg_storage_image_3d_h h_storage_image_3d, tg_storage_buffer_h h_storage_buffer);
void                             tg_storage_image_3d_clear(tg_storage_image_3d_h h_storage_image_3d);
tg_storage_image_3d_h            tg_storage_image_3d_create(u32 width, u32 height, u32 depth, tg_storage_image_format format);
void                             tg_storage_image_3d_destroy(tg_storage_image_3d_h h_storage_image_3d);

tg_texture_atlas_h               tg_texture_atlas_create_from_images(u32 image_count, tg_color_image_h* ph_color_images);
void                             tg_texture_atlas_destroy(tg_texture_atlas_h h_texture_atlas);

tg_uniform_buffer_h              tg_uniform_buffer_create(u64 size);
void*                            tg_uniform_buffer_data(tg_uniform_buffer_h h_uniform_buffer);
void                             tg_uniform_buffer_destroy(tg_uniform_buffer_h h_uniform_buffer);

tg_render_command_h              tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material, v3 position, u32 global_resource_count, tg_handle* p_global_resources);
void                             tg_render_command_destroy(tg_render_command_h h_render_command);
void                             tg_render_command_set_position(tg_render_command_h h_render_command, v3 position);

tg_vertex_shader_h               tg_vertex_shader_create(const char* p_filename);
void                             tg_vertex_shader_destroy(tg_vertex_shader_h h_vertex_shader);
tg_vertex_shader_h               tg_vertex_shader_get(const char* p_filename);

#endif
