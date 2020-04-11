#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "math/tg_math.h"
#include "tg_common.h"



TG_DECLARE_HANDLE(tg_camera);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_image);
TG_DECLARE_HANDLE(tg_material);
TG_DECLARE_HANDLE(tg_mesh);
TG_DECLARE_HANDLE(tg_model);
TG_DECLARE_HANDLE(tg_index_buffer);
TG_DECLARE_HANDLE(tg_vertex_buffer);
TG_DECLARE_HANDLE(tg_vertex_shader);



typedef enum tg_filter
{
	TG_FILTER_NEAREST    = 0,
	TG_FILTER_LINEAR     = 1
} tg_filter;

typedef enum tg_image_aspect
{
	TG_IMAGE_ASPECT_COLOR_BIT    = 0x00000001,
	TG_IMAGE_ASPECT_DEPTH_BIT    = 0x00000002
} tg_image_aspect;

typedef enum tg_image_format
{
	TG_IMAGE_FORMAT_A8B8G8R8,
	TG_IMAGE_FORMAT_A8R8G8B8,
	TG_IMAGE_FORMAT_B8G8R8A8,
	TG_IMAGE_FORMAT_R8,
	TG_IMAGE_FORMAT_R8G8,
	TG_IMAGE_FORMAT_R8G8B8,
	TG_IMAGE_FORMAT_R8G8B8A8
} tg_image_format;

typedef enum tg_image_type
{
	TG_IMAGE_TYPE_1D,
	TG_IMAGE_TYPE_2D,
	TG_IMAGE_TYPE_3D
} tg_image_type;

typedef enum tg_image_usage
{
	TG_IMAGE_USAGE_TRANSFER_SRC_BIT                = 0x00000001,
	TG_IMAGE_USAGE_TRANSFER_DST_BIT                = 0x00000002,
	TG_IMAGE_USAGE_SAMPLED_BIT                     = 0x00000004,
	TG_IMAGE_USAGE_STORAGE_BIT                     = 0x00000008,
	TG_IMAGE_USAGE_COLOR_ATTACHMENT_BIT            = 0x00000010,
	TG_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT    = 0x00000020,
	TG_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT        = 0x00000040,
	TG_IMAGE_USAGE_INPUT_ATTACHMENT_BIT            = 0x00000080,
	TG_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV       = 0x00000100,
	TG_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT    = 0x00000200
} tg_image_usage;

typedef enum tg_image_view_type {
	TG_IMAGE_VIEW_TYPE_1D            = 0,
	TG_IMAGE_VIEW_TYPE_2D            = 1,
	TG_IMAGE_VIEW_TYPE_3D            = 2,
	TG_IMAGE_VIEW_TYPE_CUBE          = 3,
	TG_IMAGE_VIEW_TYPE_1D_ARRAY      = 4,
	TG_IMAGE_VIEW_TYPE_2D_ARRAY      = 5,
	TG_IMAGE_VIEW_TYPE_CUBE_ARRAY    = 6
} tg_image_view_type;

typedef enum tg_sampler_address_mode {
	TG_SAMPLER_ADDRESS_MODE_REPEAT                  = 0,
	TG_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT         = 1,
	TG_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE           = 2,
	TG_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER         = 3,
	TG_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE    = 4
} tg_sampler_address_mode;

typedef enum tg_sampler_mipmap_mode {
	TG_SAMPLER_MIPMAP_MODE_NEAREST    = 0,
	TG_SAMPLER_MIPMAP_MODE_LINEAR     = 1
} tg_sampler_mipmap_mode;

typedef struct tg_vertex
{
	v3    position;
	v3    normal;
	v2    uv;
	v3    tangent;
	v3    bitangent;
} tg_vertex;

typedef struct tg_uniform_buffer_object
{
	m4    model;
	m4    view;
	m4    projection;
} tg_uniform_buffer_object;



void                    tg_graphics_init();
void                    tg_graphics_on_window_resize(u32 width, u32 height);
void                    tg_graphics_shutdown();



tg_camera_h             tg_graphics_camera_create(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far);
m4                      tg_graphics_camera_get_projection(tg_camera_h camera_h);
m4                      tg_graphics_camera_get_view(tg_camera_h camera_h);
void                    tg_graphics_camera_set_projection(f32 fov_y, f32 near, f32 far, tg_camera_h camera_h);
void                    tg_graphics_camera_set_view(const v3* p_position, f32 pitch, f32 yaw, f32 roll, tg_camera_h camera_h);
void                    tg_graphics_camera_destroy(tg_camera_h camera_h);

tg_fragment_shader_h    tg_graphics_fragment_shader_create(const char* p_filename);
void                    tg_graphics_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h);

tg_image_h              tg_graphics_image_create(const char* p_filename);
void                    tg_graphics_image_destroy(tg_image_h image_h);

tg_material_h           tg_graphics_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h);
void                    tg_graphics_material_destroy(tg_material_h material_h);

tg_mesh_h               tg_graphics_mesh_create(u32 vertex_count, const v3* p_positions, const v3* p_normals, const v2* p_uvs, const v3* p_tangents, u32 index_count, const u16* p_indices);
void                    tg_graphics_mesh_destroy(tg_mesh_h mesh_h);

tg_model_h              tg_graphics_model_create(tg_mesh_h mesh_h, tg_material_h material_h);
void                    tg_graphics_model_destroy(tg_model_h model_h);

tg_vertex_shader_h      tg_graphics_vertex_shader_create(const char* p_filename);
void                    tg_graphics_vertex_shader_destroy(tg_vertex_shader_h p_vertex_shader_h);



/*------------------------------------------------------------+
| 3D Renderer                                                 |
+------------------------------------------------------------*/

void                    tg_graphics_renderer_3d_init(const tg_camera_h camera_h);
void                    tg_graphics_renderer_3d_register(tg_model_h model_h);
void                    tg_graphics_renderer_3d_render();
void                    tg_graphics_renderer_3d_draw();
void                    tg_graphics_renderer_3d_present();
void                    tg_graphics_renderer_3d_shutdown();
void                    tg_graphics_renderer_3d_on_window_resize(u32 w, u32 h);

#endif
