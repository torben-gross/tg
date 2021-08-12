#ifndef TG_IMAGE_IO_H
#define TG_IMAGE_IO_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"

void    tg_image_load(const char* p_filename, TG_OUT u32* p_width, TG_OUT u32* p_height, TG_OUT tg_color_image_format* p_format, TG_OUT u32** pp_data);
void    tg_image_free(u32* p_data);
void    tg_image_convert_format(TG_INOUT u32* p_data, u32 width, u32 height, tg_color_image_format old_format, tg_color_image_format new_format);
b32     tg_image_store_to_disc(const char* p_filename, u32 width, u32 height, tg_color_image_format format, const u8* p_data, b32 force_alpha_one, b32 replace_existing);

#endif
