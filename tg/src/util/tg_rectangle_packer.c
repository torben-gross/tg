#include "util/tg_rectangle_packer.h"

#include "math/tg_math.h"
#include "util/tg_list.h"
#include "util/tg_qsort.h"

static b32 tgi_compare(const tg_rectangle_packer_rect* p_r0, const tg_rectangle_packer_rect* p_r1)
{
    const b32 result = p_r0->height > p_r1->height;
    return result;
}



void tg_rectangle_packer_pack(u32 rect_count, tg_rectangle_packer_rect* p_rects, u32* p_total_width, u32* p_total_height)
{
    *p_total_width = 0;
    *p_total_height = 0;

    u32 area = 0;
    u32 max_width = 0;
    for (u32 i = 0; i < rect_count; i++)
    {
        TG_ASSERT(p_rects[i].width && p_rects[i].height);
        TG_ASSERT(area + p_rects[i].width * p_rects[i].height > area);
        area += p_rects[i].width * p_rects[i].height;
        max_width = tgm_u32_max(max_width, p_rects[i].width);
    }

    TG_QSORT_CUSTOM(tg_rectangle_packer_rect, rect_count, p_rects, tgi_compare);

    tg_list spaces = TG_LIST_CREATE__CAPACITY(tg_rectangle_packer_rect, rect_count * rect_count);

    tg_rectangle_packer_rect start_rect = { 0 };
    start_rect.left = 0;
    start_rect.bottom = 0;
    start_rect.width = tgm_u32_max((u32)tgm_f32_ceil(tgm_f32_sqrt((f32)area / 0.95f)), max_width);;
    start_rect.height = TG_U16_MAX;

    tg_list_insert(&spaces, &start_rect);

    for (u32 i = 0; i < rect_count; i++)
    {
        tg_rectangle_packer_rect* p_rect = &p_rects[i];
        for (u32 j = 0; j < spaces.count; j++)
        {
            tg_rectangle_packer_rect* p_space = TG_LIST_POINTER_TO(spaces, spaces.count - 1 - j);

            if (p_rect->width > p_space->width || p_rect->height > p_space->height)
            {
                continue;
            }

            p_rect->left = p_space->left;
            p_rect->bottom = p_space->bottom;

            *p_total_width = tgm_u32_max(*p_total_width, p_rect->left + p_rect->width);
            *p_total_height = tgm_u32_max(*p_total_height, p_rect->bottom + p_rect->height);

            if (p_rect->width == p_space->width && p_rect->height == p_space->height)
            {
                /*
                 ______________________
                |                      |
                | rect                 |
                |                      |
                |                      |
                |                      |
                |______________________|

                */
                tg_list_remove_at(&spaces, j);
            }
            else if (p_rect->width == p_space->width)
            {
                /*
                 ______________________
                |                      |
                | updated space        |
                |______________________|
                |                      |
                | rect                 |
                |______________________|

                */
                p_space->bottom += p_rect->height;
                p_space->height -= p_rect->height;
            }
            else if (p_rect->height == p_space->height)
            {
                /*
                 ______________________
                |      |               |
                | rect | updates space |
                |      |               |
                |      |               |
                |      |               |
                |______|_______________|

                */
                p_space->left += p_rect->width;
                p_space->width -= p_rect->width;
            }
            else
            {
                /*
                 ______________________
                |                      |
                | updated space        |
                |______________________|
                |      |               |
                | rect | new space     |
                |______|_______________|

                */
                tg_rectangle_packer_rect new_space = { 0 };
                new_space.left = p_space->left + p_rect->width;
                new_space.bottom = p_space->bottom;
                new_space.width = p_space->width - p_rect->width;
                new_space.height = p_rect->height;

                tg_list_insert(&spaces, &new_space);
                p_space->bottom += p_rect->height;
                p_space->height -= p_rect->height;
            }
            break;
        }
    }

    tg_list_destroy(&spaces);
}
