#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "tg/math/tg_math.h"
#include "tg/tg_common.h"



typedef struct tg_image tg_image;
typedef tg_image* tg_image_h;

typedef struct tg_vertex_buffer tg_vertex_buffer;
typedef tg_vertex_buffer* tg_vertex_buffer_h;

typedef struct tg_index_buffer tg_index_buffer;
typedef tg_index_buffer* tg_index_buffer_h;

typedef struct tg_uniform_buffer tg_uniform_buffer;
typedef tg_uniform_buffer* tg_uniform_buffer_h;

typedef struct tg_vertex_shader tg_vertex_shader;
typedef tg_vertex_shader* tg_vertex_shader_h;

typedef struct tg_fragment_shader tg_fragment_shader;
typedef tg_fragment_shader* tg_fragment_shader_h;

typedef struct tg_mesh tg_mesh;
typedef tg_mesh* tg_mesh_h;
typedef struct tg_material tg_material;
typedef tg_material* tg_material_h;
typedef struct tg_model tg_model;
typedef tg_model* tg_model_h;



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

typedef struct tg_camera
{
	tgm_mat4f view;
	tgm_mat4f projection;
} tg_camera;

typedef struct tg_vertex
{
	tgm_vec3f    position;
	tgm_vec3f    normal;
	tgm_vec2f    uv;
	tgm_vec3f    tangent;
	tgm_vec3f    bitangent;
} tg_vertex;

typedef struct tg_uniform_buffer_object
{
	tgm_mat4f    model;
	tgm_mat4f    view;
	tgm_mat4f    projection;
} tg_uniform_buffer_object;



void tg_graphics_init();
void tg_graphics_on_window_resize(ui32 width, ui32 height);
void tg_graphics_shutdown();

void tg_graphics_fragment_shader_create(const char* p_filename, tg_fragment_shader_h* p_fragment_shader_h);
void tg_graphics_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h);
void tg_graphics_image_create(const char* p_filename, tg_image_h* p_image_h);
void tg_graphics_image_destroy(tg_image_h image_h);
void tg_graphics_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, tg_material_h* p_material_h);
void tg_graphics_material_destroy(tg_material_h material_h);
void tg_graphics_mesh_create(ui32 vertex_count, const tgm_vec3f* p_positions, const tgm_vec3f* p_normals, const tgm_vec2f* p_uvs, const tgm_vec3f* p_tangents, ui32 index_count, const ui16* p_indices, tg_mesh_h* p_mesh_h);
void tg_graphics_mesh_destroy(tg_mesh_h mesh_h);
void tg_graphics_model_create(tg_mesh_h mesh_h, tg_material_h material_h, tg_model_h* p_model_h);
void tg_graphics_model_destroy(tg_model_h model_h);
void tg_graphics_vertex_shader_create(const char* p_filename, tg_vertex_shader_h* p_vertex_shader_h);
void tg_graphics_vertex_shader_destroy(tg_vertex_shader_h p_vertex_shader_h);



/*------------------------------------------------------------+
| 2D Renderer                                                 |
+------------------------------------------------------------*/

void tg_graphics_renderer_2d_init();
void tg_graphics_renderer_2d_begin();
void tg_graphics_renderer_2d_draw_sprite(f32 x, f32 y, f32 z, f32 w, f32 h, tg_image_h image);
void tg_graphics_renderer_2d_end();
void tg_graphics_renderer_2d_present();
void tg_graphics_renderer_2d_shutdown();
void tg_graphics_renderer_2d_on_window_resize(ui32 w, ui32 h);

#ifdef TG_DEBUG
void tg_graphics_renderer_2d_draw_call_count(ui32* draw_call_count);
#endif



/*------------------------------------------------------------+
| 3D Renderer                                                 |
+------------------------------------------------------------*/

void tg_graphics_renderer_3d_init(const tg_camera* p_camera);
void tg_graphics_renderer_3d_draw(const tg_model_h model_h);
void tg_graphics_renderer_3d_present();
void tg_graphics_renderer_3d_shutdown();
void tg_graphics_renderer_3d_on_window_resize(ui32 w, ui32 h);

#endif
