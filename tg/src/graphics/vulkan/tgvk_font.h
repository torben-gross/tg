#ifndef TGVK_FONT_H
#define TGVK_FONT_H

#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

typedef struct tgvk_kerning
{
    u8     right_glyph_idx;
    f32    kerning;
} tgvk_kerning;

typedef struct tgvk_glyph
{
    v2               size;
    v2               uv_min;
    v2               uv_max;
    f32              advance_width;
    f32              left_side_bearing;
    f32              bottom_side_bearing;
    u16              kerning_count;
    tgvk_kerning*    p_kernings;
} tgvk_glyph;

typedef struct tgvk_font
{
    f32            max_glyph_height;
    u16            glyph_count;
    u8             p_char_to_glyph[256];
    tgvk_image     texture_atlas;
    tgvk_glyph*    p_glyphs;
} tgvk_font;

void    tgvk_font_create(const char* p_filename, TG_OUT tgvk_font* p_font);
void    tgvk_font_destroy(tgvk_font* p_font);

#endif

#endif
