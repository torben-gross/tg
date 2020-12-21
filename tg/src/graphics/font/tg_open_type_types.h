#ifndef TG_OPEN_TYPE_TYPES_H
#define TG_OPEN_TYPE_TYPES_H

#include "tg_common.h"

// https://people.ece.cornell.edu/land/courses/ece4760/PIC32/index_fixed_point.html#:~:text=A%20narrower%202.14%20format%20is,%3D6*10-5.
#define TG_OPEN_TYPE_F2DOT14_2_F32(v)    ((f32)(v)/16384.0f)

typedef struct u24 { u8 p_values[3]; } u24;
typedef f32 tg_fixed;
typedef i16 tg_fword;
typedef u16 tg_ufword;
typedef i16 tg_f2dot14;
typedef i64 tg_longdatetime;
typedef u16 tg_offset16;
typedef u32 tg_offset32;

#endif
