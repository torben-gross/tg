#ifndef TG_FONT_IO_H
#define TG_FONT_IO_H

#include "tg_common.h"

typedef struct tg_open_type_point
{
	i16    x_coordinate;
	i16    y_coordinate;
	u8     flags;
} tg_open_type_point;

typedef struct tg_open_type_glyph
{
	u32                    point_count;
	tg_open_type_point*    p_points;
} tg_open_type_glyph;

typedef struct tg_open_type_font
{
	u16                    p_character_to_glyph[256];
	u32                    glyph_count;
	tg_open_type_glyph*    p_glyphs;
} tg_open_type_font;

void    tg_font_load(const char* p_filename, TG_OUT tg_open_type_font* p_font);
void    tg_font_free(tg_open_type_font* p_font);

#endif
