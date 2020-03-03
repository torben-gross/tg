#ifndef TG_IMAGE_H
#define TG_IMAGE_H

#include "tg/tg_common.h"
#include <math.h>

#define TG_IMAGE_MAX_MIP_LEVELS(w, h) ((ui32)log2f(max(w, h)) + 1)

typedef struct tg_image tg_image;
typedef tg_image* tg_image_h;

typedef enum tg_image_format
{
	TG_IMAGE_FORMAT_NONE = 0x00000000,
	TG_IMAGE_FORMAT_A8B8G8R8,
	TG_IMAGE_FORMAT_A8R8G8B8,
	TG_IMAGE_FORMAT_B8G8R8A8,
	TG_IMAGE_FORMAT_R8,
	TG_IMAGE_FORMAT_R8G8,
	TG_IMAGE_FORMAT_R8G8B8,
	TG_IMAGE_FORMAT_R8G8B8A8
} tg_image_format;



void tg_image_create(tg_image_h* p_handle, const char* filename);
void tg_image_destroy(tg_image_h handle);

void tg_image_get_width(tg_image_h handle, ui32* p_width);
void tg_image_get_height(tg_image_h handle, ui32* p_height);
void tg_image_get_dimensions(tg_image_h handle, ui32* p_width, ui32* p_height);
void tg_image_get_format(tg_image_h handle, tg_image_format* p_format);
void tg_image_get_data_size(tg_image_h handle, ui64* p_size);
void tg_image_get_data(tg_image_h handle, ui32** p_data);

void tg_image_convert_to_format(tg_image_h handle, tg_image_format format);

#endif
