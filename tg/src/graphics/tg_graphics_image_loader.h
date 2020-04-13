#ifndef TG_GRAPHICS_IMAGE_LOADER_H
#define TG_GRAPHICS_IMAGE_LOADER_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"

#define TG_IMAGE_MAX_MIP_LEVELS(w, h) ((u32)tgm_f32_log2((f32)tgm_u32_max((u32)w, (u32)h)) + 1)

void    tg_image_load(const char* p_filename, u32* p_width, u32* p_height, tg_image_format* p_format, u32** pp_data);
void    tg_image_free(u32* p_data);
void    tg_image_convert_format(u32* p_data, u32 width, u32 height, tg_image_format old_format, tg_image_format new_format);

#endif
