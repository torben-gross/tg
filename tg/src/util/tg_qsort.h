#ifndef TG_QSORT_H
#define TG_QSORT_H

#include "tg_common.h"

#define TG_QSORT(count, p_elements, p_compare_fn, p_user_data)    tg_qsort_impl(sizeof(*(p_elements)), count, p_elements, p_compare_fn, p_user_data)

typedef i32 tg_qsort_compare_fn(const void* p_v0, const void* p_v1, void* p_user_data);

void tg_qsort_impl(tg_size element_size, u32 element_count, void* p_elements, tg_qsort_compare_fn* p_compare_fn, void* p_user_data);

#endif
