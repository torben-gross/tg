#ifndef TG_MATH_FUNCTIONAL
#define TG_MATH_FUNCTIONAL

#include "tg/tg_common.h"

#define TG_PI 3.14159265358979323846
#define TG_FLOAT_TO_DEGREES(radians) (radians * (360.0f / ((float)TG_PI * 2.0f)))
#define TG_FLOAT_TO_RADIANS(degrees) (degrees * (((float)TG_PI * 2.0f) / 360.0f))

uint32 tg_uint32_min(uint32 a, uint32 b);
uint32 tg_uint32_max(uint32 a, uint32 b);
uint32 tg_uint32_clamp(uint32 value, uint32 low, uint32 high);

#endif
