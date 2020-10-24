#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "math/tg_math.h"
#include "tg_common.h"



#define TG_MAX_SWAPCHAIN_IMAGES           2

#define TG_MAX_COLOR_IMAGES               32
#define TG_MAX_COMPUTE_SHADERS            16
#define TG_MAX_CUBE_MAPS                  16
#define TG_MAX_DEPTH_IMAGES               8
#define TG_MAX_FRAGMENT_SHADERS           32
#define TG_MAX_MATERIALS                  512
#define TG_MAX_MESHES                     65536
#define TG_MAX_RENDER_COMMANDS            65536
#define TG_MAX_RENDERERS                  4
#define TG_MAX_STORAGE_BUFFERS            8
#define TG_MAX_STORAGE_IMAGES_3D          16
#define TG_MAX_UNIFORM_BUFFERS            64
#define TG_MAX_VERTEX_SHADERS             32

#define TG_MAX_SHADER_ATTACHMENTS         8
#define TG_MAX_SHADER_GLOBAL_RESOURCES    32
#define TG_MAX_SHADER_INPUTS              32
#define TG_SHADER_RESERVED_BINDINGS       2

#define TG_CASCADED_SHADOW_MAPS           4
#define TG_CASCADED_SHADOW_MAP_SIZE       1024

#define TG_SSAO_MAP_SIZE                  2048

#define TG_MAX_DIRECTIONAL_LIGHTS         512
#define TG_MAX_POINT_LIGHTS               512

#define TG_IMAGE_MAX_MIP_LEVELS(w, h)     ((u32)tgm_f32_log2((f32)tgm_u32_max((u32)w, (u32)h)) + 1)

#define TG_CAMERA_LEFT(camera)            (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){ -1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_RIGHT(camera)           (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_DOWN(camera)            (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f, -1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_UP(camera)              (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_FORWARD(camera)         (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f, -1.0f,  0.0f }).xyz)
#define TG_CAMERA_BACKWARD(camera)        (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f,  1.0f,  0.0f }).xyz)



TG_DECLARE_HANDLE(tg_color_image);
TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_cube_map);
TG_DECLARE_HANDLE(tg_depth_image);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_render_command);
TG_DECLARE_HANDLE(tg_render_target);
TG_DECLARE_HANDLE(tg_renderer);
TG_DECLARE_HANDLE(tg_storage_buffer);
TG_DECLARE_HANDLE(tg_storage_image_3d);
TG_DECLARE_HANDLE(tg_uniform_buffer);
TG_DECLARE_HANDLE(tg_vertex_shader);

typedef void* tg_handle;



typedef enum tg_camera_type
{
	TG_CAMERA_TYPE_ORTHOGRAPHIC,
	TG_CAMERA_TYPE_PERSPECTIVE
} tg_camera_type;

typedef enum tg_color_image_format
{
	TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM          = 51,
	TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM          = 44,
	TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT     = 97,
    TG_COLOR_IMAGE_FORMAT_R32_UINT                = 98,
	TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT     = 109,
	TG_COLOR_IMAGE_FORMAT_R8_UNORM                = 9,
	TG_COLOR_IMAGE_FORMAT_R8G8_UNORM              = 16,
	TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM            = 23,
	TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM          = 37
} tg_color_image_format;

typedef enum tg_depth_image_format
{
	TG_DEPTH_IMAGE_FORMAT_D16_UNORM     = 124,
	TG_DEPTH_IMAGE_FORMAT_D32_SFLOAT    = 126
} tg_depth_image_format;

typedef enum tg_image_address_mode
{
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
	TG_IMAGE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
	TG_IMAGE_ADDRESS_MODE_MIRRORED_REPEAT = 1,
	TG_IMAGE_ADDRESS_MODE_REPEAT = 0
} tg_image_address_mode;

typedef enum tg_image_filter
{
	TG_IMAGE_FILTER_LINEAR = 1,
	TG_IMAGE_FILTER_NEAREST = 0
} tg_image_filter;

typedef enum tg_shader_type
{
	TG_SHADER_TYPE_COMPUTE,
	TG_SHADER_TYPE_FRAGMENT,
	TG_SHADER_TYPE_GEOMETRY,
	TG_SHADER_TYPE_VERTEX
} tg_shader_type;

typedef enum tg_structure_type
{
	TG_STRUCTURE_TYPE_INVALID = 0,
	TG_STRUCTURE_TYPE_COLOR_IMAGE,
	TG_STRUCTURE_TYPE_COMPUTE_SHADER,
    TG_STRUCTURE_TYPE_CUBE_MAP,
	TG_STRUCTURE_TYPE_DEPTH_IMAGE,
	TG_STRUCTURE_TYPE_FRAGMENT_SHADER,
	TG_STRUCTURE_TYPE_MATERIAL,
	TG_STRUCTURE_TYPE_MESH,
	TG_STRUCTURE_TYPE_RENDER_COMMAND,
	TG_STRUCTURE_TYPE_RENDER_TARGET,
	TG_STRUCTURE_TYPE_RENDERER,
	TG_STRUCTURE_TYPE_STORAGE_BUFFER,
	TG_STRUCTURE_TYPE_STORAGE_IMAGE_3D,
	TG_STRUCTURE_TYPE_UNIFORM_BUFFER,
	TG_STRUCTURE_TYPE_VERTEX_SHADER
} tg_structure_type;

typedef enum tg_vertex_input_attribute_format
{
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_INVALID                = 0,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT             = 100,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT               = 99,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT               = 98,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT          = 103,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT            = 102,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT            = 101,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT       = 106,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT         = 105,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT         = 104,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT    = 109,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT      = 108,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT      = 107
} tg_vertex_input_attribute_format;



typedef struct tg_bounds // TODO: this should be part of physics
{
	v3    min;
	v3    max;
} tg_bounds;

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
            f32       l, r, b, t, n, f;
		} ortho;
		struct
		{
			f32       fov_y_in_radians;
			f32       aspect;
			f32       n, f;
		} persp;
	};
} tg_camera;

typedef struct tg_sampler_create_info
{
	tg_image_filter          min_filter;
	tg_image_filter          mag_filter;
	tg_image_address_mode    address_mode_u;
	tg_image_address_mode    address_mode_v;
	tg_image_address_mode    address_mode_w;
} tg_sampler_create_info;





void                     tg_graphics_init(void);
void                     tg_graphics_on_window_resize(u32 width, u32 height);
void                     tg_graphics_shutdown(void);
void                     tg_graphics_wait_idle(void);



void                     tg_atmosphere_precompute(void);

tg_color_image_h         tg_color_image_create(u32 width, u32 height, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
tg_color_image_h         tg_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info);
void                     tg_color_image_destroy(tg_color_image_h h_color_image);

u32                      tg_color_image_format_channels(tg_color_image_format format);
u32                      tg_color_image_format_size(tg_color_image_format format);

tg_storage_image_3d_h    tg_storage_image_3d_create(u32 width, u32 height, u32 depth, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                     tg_storage_image_3d_destroy(tg_storage_image_3d_h h_storage_image_3d);
void                     tg_storage_image_3d_set_data(tg_storage_image_3d_h h_storage_image_3d, void* p_data);

void                     tg_compute_shader_bind_input(tg_compute_shader_h h_compute_shader, u32 first_handle_index, u32 handle_count, tg_handle* p_handles);
tg_compute_shader_h      tg_compute_shader_create(const char* filename);
void                     tg_compute_shader_dispatch(tg_compute_shader_h h_compute_shader, u32 group_count_x, u32 group_count_y, u32 group_count_z);
void                     tg_compute_shader_destroy(tg_compute_shader_h h_compute_shader);
tg_compute_shader_h      tg_compute_shader_get(const char* filename);

tg_cube_map_h            tg_cube_map_create(u32 dimension, tg_color_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                     tg_cube_map_destroy(tg_cube_map_h h_cube_map);
void                     tg_cube_map_set_data(tg_cube_map_h h_cube_map, void* p_data);

tg_depth_image_h         tg_depth_image_create(u32 width, u32 height, tg_depth_image_format format, const tg_sampler_create_info* p_sampler_create_info);
void                     tg_depth_image_destroy(tg_depth_image_h h_depth_image);

tg_fragment_shader_h     tg_fragment_shader_create(const char* p_filename);
void                     tg_fragment_shader_destroy(tg_fragment_shader_h h_fragment_shader);
tg_fragment_shader_h     tg_fragment_shader_get(const char* p_filename);

tg_material_h            tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
tg_material_h            tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader);
void                     tg_material_destroy(tg_material_h h_material);
b32                      tg_material_is_deferred(tg_material_h h_material);
b32                      tg_material_is_forward(tg_material_h h_material);

tg_mesh_h                tg_mesh_create(void);
tg_mesh_h                tg_mesh_create2(const char* p_filename, v3 scale); // TODO: scale is temporary
tg_mesh_h                tg_mesh_create_sphere(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents);
tg_mesh_h                tg_mesh_create_sphere_flat(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents);
tg_bounds                tg_mesh_get_bounds(tg_mesh_h h_mesh); // TODO: could i separate tg_mesh and tg_mesh_internal, such that i dont need these getters?
u32                      tg_mesh_get_index_count(tg_mesh_h h_mesh);
u32                      tg_mesh_get_vertex_count(tg_mesh_h h_mesh);
void                     tg_mesh_copy_indices(tg_mesh_h h_mesh, u32 first, u32 count, u16* p_buffer);
void                     tg_mesh_copy_positions(tg_mesh_h h_mesh, u32 first, u32 count, v3* p_buffer);
void                     tg_mesh_copy_normals(tg_mesh_h h_mesh, u32 first, u32 count, v3* p_buffer);
void                     tg_mesh_copy_uvs(tg_mesh_h h_mesh, u32 first, u32 count, v2* p_buffer);
void                     tg_mesh_copy_tangents(tg_mesh_h h_mesh, u32 first, u32 count, v3* p_buffer);
void                     tg_mesh_copy_bitangents(tg_mesh_h h_mesh, u32 first, u32 count, v3* p_buffer);
void                     tg_mesh_set_indices(tg_mesh_h h_mesh, u32 count, const u16* p_indices);
void                     tg_mesh_set_positions(tg_mesh_h h_mesh, u32 count, const v3* p_positions);
void                     tg_mesh_set_positions2(tg_mesh_h h_mesh, u32 count, tg_storage_buffer_h h_storage_buffer);
void                     tg_mesh_set_normals(tg_mesh_h h_mesh, u32 count, const v3* p_normals);
void                     tg_mesh_set_normals2(tg_mesh_h h_mesh, u32 count, tg_storage_buffer_h h_storage_buffer);
void                     tg_mesh_set_uvs(tg_mesh_h h_mesh, u32 count, const v2* p_uvs);
void                     tg_mesh_set_tangents(tg_mesh_h h_mesh, u32 count, const v3* p_tangents);
void                     tg_mesh_set_bitangents(tg_mesh_h h_mesh, u32 count, const v3* p_bitangents);
void                     tg_mesh_regenerate_normals(tg_mesh_h h_mesh);
void                     tg_mesh_regenerate_tangents_bitangents(tg_mesh_h h_mesh);
void                     tg_mesh_destroy(tg_mesh_h h_mesh);

tg_render_command_h      tg_render_command_create(tg_mesh_h h_mesh, tg_material_h h_material, v3 position, u32 global_resource_count, tg_handle* p_global_resources);
void                     tg_render_command_destroy(tg_render_command_h h_render_command);
tg_mesh_h                tg_render_command_get_mesh(tg_render_command_h h_render_command);
void                     tg_render_command_set_position(tg_render_command_h h_render_command, v3 position);

u32                      tg_render_target_get_width(tg_render_target_h h_render_target);
u32                      tg_render_target_get_height(tg_render_target_h h_render_target);
tg_color_image_format    tg_render_target_get_color_format(tg_render_target_h h_render_target);
void                     tg_render_target_get_color_data_copy(tg_render_target_h h_render_target, TG_INOUT u32* p_buffer_size, TG_OUT void* p_buffer);

void                     tg_renderer_init_shared_resources(void);
void                     tg_renderer_shutdown_shared_resources(void);
tg_renderer_h            tg_renderer_create(tg_camera* p_camera);
void                     tg_renderer_destroy(tg_renderer_h h_renderer);
void                     tg_renderer_enable_shadows(tg_renderer_h h_renderer, b32 enable);
void                     tg_renderer_begin(tg_renderer_h h_renderer);
void                     tg_renderer_push_directional_light(tg_renderer_h h_renderer, v3 direction, v3 color);
void                     tg_renderer_push_point_light(tg_renderer_h h_renderer, v3 position, v3 color);
void                     tg_renderer_exec(tg_renderer_h h_renderer, tg_render_command_h h_render_command);
void                     tg_renderer_end(tg_renderer_h h_renderer, f32 dt, b32 present);
void                     tg_renderer_clear(tg_renderer_h h_renderer);
tg_render_target_h       tg_renderer_get_render_target(tg_renderer_h h_renderer);
v3                       tg_renderer_screen_to_world(tg_renderer_h h_renderer, u32 x, u32 y);
void                     tg_renderer_screenshot(tg_renderer_h h_renderer, const char* p_filename);
#if TG_ENABLE_DEBUG_TOOLS == 1
void                     tg_renderer_draw_cube_DEBUG(tg_renderer_h h_renderer, v3 position, v3 scale, v4 color);
#else
#define                  tg_renderer_draw_cube_DEBUG(h_renderer, position, scale, color)
#endif

tg_storage_buffer_h      tg_storage_buffer_create(u64 size, b32 visible);
u64                      tg_storage_buffer_size(tg_storage_buffer_h h_storage_buffer);
void*                    tg_storage_buffer_data(tg_storage_buffer_h h_storage_buffer);
void                     tg_storage_buffer_destroy(tg_storage_buffer_h h_storage_buffer);

tg_uniform_buffer_h      tg_uniform_buffer_create(u64 size);
u64                      tg_uniform_buffer_size(tg_uniform_buffer_h h_uniform_buffer);
void*                    tg_uniform_buffer_data(tg_uniform_buffer_h h_uniform_buffer);
void                     tg_uniform_buffer_destroy(tg_uniform_buffer_h h_uniform_buffer);

u32                      tg_vertex_input_attribute_format_get_alignment(tg_vertex_input_attribute_format format);
u32                      tg_vertex_input_attribute_format_get_size(tg_vertex_input_attribute_format format);

tg_vertex_shader_h       tg_vertex_shader_create(const char* p_filename);
void                     tg_vertex_shader_destroy(tg_vertex_shader_h h_vertex_shader);
tg_vertex_shader_h       tg_vertex_shader_get(const char* p_filename);

#endif
