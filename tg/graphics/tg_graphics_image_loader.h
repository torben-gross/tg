#ifndef TG_IMAGE_H
#define TG_IMAGE_H

#include "tg/tg_common.h"
#include "tg/graphics/tg_graphics.h"
#include <math.h>

#define TG_IMAGE_MAX_MIP_LEVELS(w, h) ((ui32)log2f((f32)max((ui32)w, (ui32)h)) + 1)

void tg_image_load(const char* filename, ui32* p_width, ui32* p_height, tg_image_format* p_format, ui32** p_data);
void tg_image_free(ui32* data);

void tg_image_convert_to_format(ui32* data, ui32 width, ui32 height, tg_image_format old_format, tg_image_format new_format);

#endif
