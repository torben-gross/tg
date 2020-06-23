#include "util/tg_qsort.h"

static void tgi_swap(u32 element_size, u8* p_v0, u8* p_v1)
{
    for (u32 i = 0; i < element_size; i++)
    {
        const u8 temp = p_v0[i];
        p_v0[i] = p_v1[i];
        p_v1[i] = temp;
    }
}

static i32 tgi_partition(u32 element_size, u8* p_elements, i32 low, i32 high, tg_qsort_compare_fn compare_fn)
{
    const void* p_pivot = &p_elements[high * element_size];
    i32 i = low - 1;

    for (i32 j = low; j <= high - 1; j++)
    {
        if (compare_fn(&p_elements[j * element_size], p_pivot))
        {
            i++;
            tgi_swap(element_size, &p_elements[i * element_size], &p_elements[j * element_size]);
        }
    }
    tgi_swap(element_size, &p_elements[(i + 1) * element_size], &p_elements[high * element_size]);

    return i + 1;
}

static void tgi_impl(u32 element_size, u8* p_elements, i32 low, i32 high, tg_qsort_compare_fn compare_fn)
{
    if (low < high)
    {
        const i32 pi = tgi_partition(element_size, p_elements, low, high, compare_fn);
        tgi_impl(element_size, p_elements, low, pi - 1, compare_fn);
        tgi_impl(element_size, p_elements, pi + 1, high, compare_fn);
    }
}



void tg_qsort_impl(u32 element_size, u32 element_count, void* p_elements, tg_qsort_compare_fn compare_fn)
{
    TG_ASSERT(element_size && element_count && p_elements && compare_fn);

    tgi_impl(element_size, (u8*)p_elements, 0, element_count - 1, compare_fn);
}
