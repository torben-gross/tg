#ifndef TG_MATH_FUNCTIONAL
#define TG_MATH_FUNCTIONAL

#include "tg_math_defines.h"
#include "tg/tg_common.h"

#define TG_PI TG_MATH_FLOAT(3.14159265358979323846)
#define TG_FLOAT_TO_DEGREES(radians) (radians * (TG_MATH_FLOAT(360.0) / (TG_PI * TG_MATH_FLOAT(2.0))))
#define TG_FLOAT_TO_RADIANS(degrees) (degrees * ((TG_PI * TG_MATH_FLOAT(2.0)) / TG_MATH_FLOAT(360.0)))

uint32 tg_uint32_min(uint32 a, uint32 b);
uint32 tg_uint32_max(uint32 a, uint32 b);
uint32 tg_uint32_clamp(uint32 value, uint32 low, uint32 high);

#endif
