#ifndef TG_BITMAP_H
#define TG_BITMAP_H

#include "tg_common.h"

#define TG_BITMAP32                        u32
#define TG_BITMAP32_CAPACITY               (8 * sizeof(TG_BITMAP32))
#define TG_BITMAP32_SET(bitmap, bit)       ((void)((bitmap) |= (1 << (bit))))
#define TG_BITMAP32_UNSET(bitmap, bit)     ((void)((bitmap) &= (~(1 << (bit)))))
#define TG_BITMAP32_FILL(bitmap)           ((void)((bitmap) = 0xffffffff))
#define TG_BITMAP32_CLEAR(bitmap)          ((void)((bitmap) = 0))
#define TG_BITMAP32_IS_SET(bitmap, bit)    (((bitmap) & (1 << bit)) != 0)

#define TG_ARRAY_BITMAP_TYPE                        u32
#define TG_ARRAY_BITMAP_TYPE_NUM_BITS               (8 * sizeof(TG_ARRAY_BITMAP_TYPE))
#define TG_ARRAY_BITMAP_NUM_ELEMS(bit_count)        (((bit_count) + (TG_ARRAY_BITMAP_TYPE_NUM_BITS - 1)) / TG_ARRAY_BITMAP_TYPE_NUM_BITS)
#define TG_ARRAY_BITMAP_SET_BIT(bitmap, bit)        ((void)((bitmap)[(bit) / TG_ARRAY_BITMAP_TYPE_NUM_BITS] |=   (1 << ((bit) % TG_ARRAY_BITMAP_TYPE_NUM_BITS)))     )
#define TG_ARRAY_BITMAP_CLEAR_BIT(bitmap, bit)      ((void)((bitmap)[(bit) / TG_ARRAY_BITMAP_TYPE_NUM_BITS] &= (~(1 << ((bit) % TG_ARRAY_BITMAP_TYPE_NUM_BITS))))    )
#define TG_ARRAY_BITMAP_IS_BIT_SET(bitmap, bit)     (      ((bitmap)[(bit) / TG_ARRAY_BITMAP_TYPE_NUM_BITS] &    (1 << ((bit) % TG_ARRAY_BITMAP_TYPE_NUM_BITS))) != 0)

#define TG_ARRAY_BITMAP_CLEAR(bitmap, bit_count)                                                 \
	{                                                                                            \
		const u32 _bitmap_num_elems = TG_ARRAY_BITMAP_NUM_ELEMS(bit_count);                      \
		for (u32 _bitmap_elem_idx = 0; _bitmap_elem_idx < _bitmap_num_elems; _bitmap_elem_idx++) \
			(bitmap)[_bitmap_elem_idx] = 0;                                                      \
	}

#endif
