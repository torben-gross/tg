#include "util/tg_rectangle_packer.h"

#include "util/tg_qsort.h"

b32 tg_rectangle_packer_internal_rect_compare(const tg_rect* p_r0, const tg_rect* p_r1)
{
    b32 result = p_r0->height < p_r1->height;
    return result;
}

void tg_rectangle_packer_pack(u32 rect_count, tg_rect* p_rects)
{
    TG_QSORT_CUSTOM(tg_rect, rect_count, p_rects, tg_rectangle_packer_internal_rect_compare);
}
