#ifndef TG_FONT_H
#define TG_FONT_H

#include "tg_common.h"

typedef struct tg_open_type_point
{
	i16    x;
	i16    y;
	u8     flags;
} tg_open_type_point;

typedef struct tg_open_type_contour
{
	u32                    point_count;
	tg_open_type_point*    p_points;
} tg_open_type_contour;

typedef struct tg_open_type_glyph
{
	i16                      x_min;
	i16                      y_min;
	i16                      x_max;
	i16                      y_max;
	u32                      contour_count;
	tg_open_type_contour*    p_contours;
} tg_open_type_glyph;

typedef struct tg_open_type_font
{
	u16                    p_character_to_glyph[256];
	u32                    glyph_count;
	tg_open_type_glyph*    p_glyphs;
} tg_open_type_font;

void    tg_font_load(const char* p_filename, TG_OUT tg_open_type_font* p_font);
void    tg_font_free(tg_open_type_font* p_font);
void    tg_font_rasterize(const tg_open_type_font* p_font, unsigned char character, u32 width, u32 height, TG_OUT u8* p_image_data);

#endif
