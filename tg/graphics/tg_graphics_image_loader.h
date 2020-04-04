#ifndef TG_IMAGE_H
#define TG_IMAGE_H

#include "tg/graphics/tg_graphics.h"
#include "tg/math/tg_math.h"
#include "tg/tg_common.h"

#define TG_IMAGE_MAX_MIP_LEVELS(w, h) ((ui32)tgm_f32_log2((f32)tgm_ui32_max((ui32)w, (ui32)h)) + 1)

void tg_image_load(const char* p_filename, ui32* p_width, ui32* p_height, tg_image_format* p_format, ui32** pp_data);
void tg_image_free(ui32* p_data);

void tg_image_convert_format(ui32* p_data, ui32 width, ui32 height, tg_image_format old_format, tg_image_format new_format);

#endif
