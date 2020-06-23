#ifndef TG_QSORT_H
#define TG_QSORT_H

#include "tg_common.h"

#define TG_QSORT(type, count, p_elements)                       tg_qsort_impl(sizeof(type), count, p_elements, tg_##type##_compare_fn)
#define TG_QSORT_CUSTOM(type, count, p_elements, compare_fn)    tg_qsort_impl(sizeof(type), count, p_elements, compare_fn)

typedef b32(*tg_qsort_compare_fn)(const void* p_v0, const void* p_v1);

void tg_qsort_impl(u32 element_size, u32 element_count, void* p_elements, tg_qsort_compare_fn compare_fn);

#endif
