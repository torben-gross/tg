#ifndef TG_FONT_H
#define TG_FONT_H

#include "tg_common.h"

#define TG_FONT_GRID2PX(font, size_pt, grid_pt)    ((f32)(grid_pt) * ((f32)(size_pt) * (f32)tgp_get_window_dpi() / 72.0f) / (f32)(font).units_per_em)

typedef struct tg_open_type_point
{
	f32    x;
	f32    y;
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
	u16                      advance_width;
	i16                      left_side_bearing;
	u32                      contour_count;
	tg_open_type_contour*    p_contours;
} tg_open_type_glyph;

typedef struct tg_open_type_font
{
	u16                    p_character_to_glyph[256];
	i16                    x_min;
	i16                    x_max;
	i16                    y_min;
	i16                    y_max;
	u16                    units_per_em;
	u16                    glyph_count;
	tg_open_type_glyph*    p_glyphs;
} tg_open_type_font;

void                         tg_font_load(const char* p_filename, TG_OUT tg_open_type_font* p_font);
void                         tg_font_free(tg_open_type_font* p_font);
const tg_open_type_glyph*    tg_font_get_glyph(const tg_open_type_font* p_font, unsigned char character);
void                         tg_font_rasterize_pt(const tg_open_type_font* p_font, const tg_open_type_glyph* p_glyph, u32 glyph_off_x, u32 glyph_off_y, u32 size_pt, u32 img_w, u32 img_h, TG_OUT u8* p_image_data);
void                         tg_font_rasterize_wh(const tg_open_type_glyph* p_glyph, u32 glyph_off_x, u32 glyph_off_y, u32 glyph_w, u32 glyph_h, u32 img_w, u32 img_h, TG_OUT u8* p_image_data);

#endif
