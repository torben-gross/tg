#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "tg/tg_common.h"
#include "tg/math/tg_math.h"

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

typedef struct tg_vertex
{
	tgm_vec3f position;
	tgm_vec3f color;
	tgm_vec2f uv;
} tg_vertex;

typedef struct tg_uniform_buffer_object
{
	tgm_mat4f model;
	tgm_mat4f view;
	tgm_mat4f projection;
} tg_uniform_buffer_object;

typedef enum tg_image_format
{
	TG_IMAGE_FORMAT_A8B8G8R8,
	TG_IMAGE_FORMAT_A8R8G8B8,
	TG_IMAGE_FORMAT_B8G8R8A8,
	TG_IMAGE_FORMAT_R8,
	TG_IMAGE_FORMAT_R8G8,
	TG_IMAGE_FORMAT_R8G8B8,
	TG_IMAGE_FORMAT_R8G8B8A8,
	TG_IMAGE_FORMAT_COUNT
} tg_image_format;

typedef enum tg_vertex_shader_layout_element_type
{
	TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32_SFLOAT,
	TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32_SFLOAT,
	TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32B32_SFLOAT,
	TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32B32A32_SFLOAT
} tg_vertex_shader_layout_element_type;

typedef struct tg_vertex_shader_layout_element
{
	ui32 binding;
	ui32 location;
	tg_vertex_shader_layout_element_type element_type;
	ui32 offset;
} tg_vertex_shader_layout_element;



void tg_graphics_init();
void tg_graphics_render();
void tg_graphics_on_window_resize(ui32 width, ui32 height);
void tg_graphics_shutdown();

void tg_graphics_image_create(const char* filename, tg_image_h* p_image_h);
void tg_graphics_image_destroy(tg_image_h image_h);

void tg_graphics_vertex_buffer_create(ui32 size, ui32 layout_element_count, const tg_vertex_shader_layout_element* layout, void* vertices, tg_vertex_buffer_h* p_vertex_buffer_h);
void tg_graphics_vertex_buffer_destroy(tg_vertex_buffer_h vertex_buffer_h);

void tg_graphics_index_buffer_create(ui16 count, ui16* indices, tg_index_buffer_h* p_index_buffer_h);
void tg_graphics_index_buffer_destroy(tg_index_buffer_h index_buffer_h);

void tg_graphics_uniform_buffer_create(ui32 size, tg_uniform_buffer_h* p_uniform_buffer_h);
void tg_graphics_uniform_buffer_destroy(tg_uniform_buffer_h uniform_buffer_h);

void tg_graphics_vertex_shader_create(const char* filename, ui32 layout_element_count, const tg_vertex_shader_layout_element* layout, tg_vertex_shader_h* p_shader_h);
void tg_graphics_vertex_shader_destroy(tg_vertex_shader_h shader_h);

void tg_graphics_fragment_shader_create(const char* filename, tg_fragment_shader_h* p_fragment_shader_h);
void tg_graphics_fragment_shader_destroy(tg_fragment_shader_h p_fragment_shader_h);

/* --------
renderer 2d
-------- */
void tg_graphics_renderer_2d_init();
void tg_graphics_renderer_2d_begin();
void tg_graphics_renderer_2d_draw_sprite(f32 x, f32 y, f32 z, f32 w, f32 h, tg_image_h image);
void tg_graphics_renderer_2d_end();
void tg_graphics_renderer_2d_shutdown();
void tg_graphics_renderer_2d_on_window_resize(ui32 w, ui32 h);

#endif
