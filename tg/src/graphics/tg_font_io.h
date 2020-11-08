#ifndef TG_FONT_IO_H
#define TG_FONT_IO_H

#include "tg_common.h"

typedef struct tg_font
{
	int placeholder;
} tg_font;

void    tg_font_load(const char* p_filename, TG_OUT tg_font* p_font);
void    tg_font_free(tg_font* p_font);

#endif
