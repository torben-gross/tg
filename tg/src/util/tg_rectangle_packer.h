#ifndef TG_RECTANGLE_PACKER_H
#define TG_RECTANGLE_PACKER_H

#include "tg_common.h"



typedef struct tg_rectangle_packer_rect
{
	u32    id;
	u16    left;
	u16    bottom;
	u16    width;
	u16    height;
} tg_rectangle_packer_rect;



void tg_rectangle_packer_pack(u32 rect_count, tg_rectangle_packer_rect* p_rects, u32* p_total_width, u32* p_total_height);

#endif
