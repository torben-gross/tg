#include "tg_math_functional.h"

uint32 tg_uint32_min(uint32 a, uint32 b)
{
	return a < b ? a : b;
}

uint32 tg_uint32_max(uint32 a, uint32 b)
{
	return a > b ? a : b;
}

uint32 tg_uint32_clamp(uint32 value, uint32 low, uint32 high)
{
	return tg_uint32_max(low, tg_uint32_min(high, value));
}
