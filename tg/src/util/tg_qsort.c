#include "util/tg_qsort.h"

#include "math/tg_math.h"



// source: https://github.com/lattera/glibc/blob/master/stdlib/qsort.c

#define TG_MAX_THRESH                   4
#define TG_STACK_SIZE                   (8 * sizeof(u32))
#define TG_STACK_PUSH(p_low, p_high)    ((void) ((p_top->p_lo = (p_low)), (p_top->p_hi = (p_high)), ++p_top))
#define TG_STACK_POP(p_low, p_high)     ((void) (--p_top, (p_low = p_top->p_lo), (p_high = p_top->p_hi)))
#define TG_STACK_NOT_EMPTY              (p_stack < p_top)

#define TG_SWAP(element_size, p_left, p_right) \
    do                                         \
    {                                          \
        u32 s = (u32)(element_size);           \
        i8* p_l = (i8*)(p_left);               \
        i8* p_r = (i8*)(p_right);              \
        do                                     \
        {                                      \
            const i8 temp = *p_l;              \
            *p_l++ = *p_r;                     \
            *p_r++ = temp;                     \
        }                                      \
        while (--s > 0);                       \
    }                                          \
    while (TG_FALSE)



typedef struct tg_stack_node
{
    const i8*    p_lo;
    const i8*    p_hi;
} tg_stack_node;



void tg_qsort_impl(u32 element_size, u32 element_count, void* p_elements, tg_qsort_compare_fn* p_compare_fn, void* p_user_data)
{
    TG_ASSERT(element_size && element_count && p_elements && p_compare_fn);

    i8* p_base = (i8*)p_elements;
    const u64 max_thresh = TG_MAX_THRESH * (u64)element_size;

    if (element_count > TG_MAX_THRESH)
    {
        const i8* p_lo = p_base;
        const i8* p_hi = &p_lo[element_size * (element_count - 1)];
        tg_stack_node p_stack[TG_STACK_SIZE] = { 0 };
        tg_stack_node* p_top = p_stack;

        TG_STACK_PUSH(TG_NULL, TG_NULL);

        while (TG_STACK_NOT_EMPTY)
        {
            const i8* p_left;
            const i8* p_right;

            const i8* p_mid = p_lo + element_size * ((p_hi - p_lo) / element_size >> 1);

            if ((*p_compare_fn)((void*)p_mid, (void*)p_lo, p_user_data) < 0)
            {
                TG_SWAP(element_size, p_mid, p_lo);
            }
            if ((*p_compare_fn)((void*)p_hi, (void*)p_mid, p_user_data) < 0)
            {
                TG_SWAP(element_size, p_mid, p_hi);
            }
            else
            {
                goto jump_over;
            }
            if ((*p_compare_fn)((void*)p_mid, (void*)p_lo, p_user_data) < 0)
            {
                TG_SWAP(element_size, p_mid, p_lo);
            }
        jump_over:

            p_left = p_lo + element_size;
            p_right = p_hi - element_size;

            do
            {
                while ((*p_compare_fn)((void*)p_left, (void*)p_mid, p_user_data) < 0)
                {
                    p_left += element_size;
                }

                while ((*p_compare_fn)((void*)p_mid, (void*)p_right, p_user_data) < 0)
                {
                    p_right -= element_size;
                }

                if (p_left < p_right)
                {
                    TG_SWAP(element_size, p_left, p_right);
                    if (p_mid == p_left)
                    {
                        p_mid = p_right;
                    }
                    else if (p_mid == p_right)
                    {
                        p_mid = p_left;
                    }
                    p_left += element_size;
                    p_right -= element_size;
                }
                else if (p_left == p_right)
                {
                    p_left += element_size;
                    p_right -= element_size;
                    break;
                }
            }
            while (p_left <= p_right);

            if ((u64)(p_right - p_lo) <= max_thresh)
            {
                if ((u64)(p_hi - p_left) <= max_thresh)
                {
                    TG_STACK_POP(p_lo, p_hi);
                }
                else
                {
                    p_lo = p_left;
                }
            }
            else if ((u64)(p_hi - p_left) <= max_thresh)
            {
                p_hi = p_right;
            }
            else if ((p_right - p_lo) > (p_hi - p_left))
            {
                TG_STACK_PUSH(p_lo, p_right);
                p_lo = p_left;
            }
            else
            {
                TG_STACK_PUSH(p_left, p_hi);
                p_hi = p_right;
            }
        }
    }

    {
        const i8* const p_end = &p_base[element_size * (element_count - 1)];
        i8* p_tmp = p_base;
        const i8* p_thresh = TG_MIN(p_end, p_base + max_thresh);
        i8* p_run;

        for (p_run = p_tmp + element_size; p_run <= p_thresh; p_run += element_size)
        {
            if ((*p_compare_fn)((void*)p_run, (void*)p_tmp, p_user_data) < 0)
            {
                p_tmp = p_run;
            }
        }

        if (p_tmp != p_base)
        {
            TG_SWAP(element_size, p_tmp, p_base);
        }

        p_run = p_base + element_size;
        while ((p_run += element_size) <= p_end)
        {
            p_tmp = p_run - element_size;
            while ((*p_compare_fn)((void*)p_run, (void*)p_tmp, p_user_data) < 0)
            {
                p_tmp -= element_size;
            }
            p_tmp += element_size;
            if (p_tmp != p_run)
            {
                i8* p_trav = p_run + element_size;
                while (--p_trav >= p_run)
                {
                    const i8 c = *p_trav;
                    char* p_hi;
                    char* p_lo;

                    for (p_hi = p_lo = p_trav; (p_lo -= element_size) >= p_tmp; p_hi = p_lo)
                    {
                        *p_hi = *p_lo;
                    }
                    *p_hi = c;
                }
            }
        }
    }
}
