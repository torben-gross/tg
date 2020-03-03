#ifndef TG_IMAGE_H
#define TG_IMAGE_H

#include "tg/tg_common.h"
#include <math.h>

#define TG_IMAGE_MAX_MIP_LEVELS(w, h) ((ui32)log2f(max(w, h)) + 1)

typedef struct tg_image tg_image;
typedef tg_image* tg_image_h;

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

void tg_image_load(const char* filename, ui32* p_width, ui32* p_height, tg_image_format* p_format, ui32** p_data);
void tg_image_free(ui32* data);

void tg_image_convert_to_format(ui32* data, ui32 width, ui32 height, tg_image_format old_format, tg_image_format new_format);

#endif
