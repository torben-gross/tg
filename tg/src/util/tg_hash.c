#include "util/tg_hash.h"


u32 tg_hash(tg_size size, const void* p_value_ptr)
{
	u32 result = 0;
	for (u32 i = 0; i < size; i++)
	{
		result += (u32)((u8*)p_value_ptr)[i] * 2654435761;
	}
	return result;
}

u32 tg_hash_str(const char* p_string)
{
	u32 result = 0;
	while (*p_string)
	{
		result += (u32)*p_string++ * 2654435761;
	}
	return result;
}
