#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "tg/tg_common.h"

typedef struct tg_image tg_image;
typedef tg_image* tg_image_h;

typedef struct tg_index_buffer tg_index_buffer;
typedef tg_index_buffer* tg_index_buffer_h;

typedef struct tg_vertex_buffer tg_vertex_buffer;
typedef tg_vertex_buffer* tg_vertex_buffer_h;

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



void tg_graphics_init();
void tg_graphics_render();
void tg_graphics_on_window_resize(ui32 width, ui32 height);
void tg_graphics_shutdown();

void tg_graphics_image_create(const char* filename, tg_image_h* p_image_h);
void tg_graphics_image_destroy(tg_image_h image_h);

void tg_graphics_index_buffer_create(ui16 count, ui16* indices, tg_index_buffer_h* p_index_buffer_h);
void tg_graphics_index_buffer_destroy(tg_index_buffer_h index_buffer_h);

void tg_graphics_vertex_buffer_create(ui32 size, void* vertices, tg_vertex_buffer_h* p_vertex_buffer_h);
void tg_graphics_vertex_buffer_destroy(tg_vertex_buffer_h vertex_buffer_h);

#endif
