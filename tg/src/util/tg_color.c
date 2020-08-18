#include "util/tg_color.h"

u32 tg_color_pack(v4 rgba)
{
	TG_ASSERT(rgba.x >= 0.0f && rgba.x <= 1.0f && rgba.y >= 0.0f && rgba.y <= 1.0f && rgba.z >= 0.0f && rgba.z <= 1.0f && rgba.w >= 0.0f && rgba.w <= 1.0f);

	const u32 r = (u8)(rgba.x * 255.0f);
	const u32 g = (u8)(rgba.y * 255.0f);
	const u32 b = (u8)(rgba.z * 255.0f);
	const u32 a = (u8)(rgba.w * 255.0f);
	const u32 result = r << 24 | g << 16 | b << 8 | a;
	return result;
}

v4 tg_color_unpack(u32 rgba)
{
	const v4 result = {
		(f32)((rgba >> 24) & 0xff) / 255.0f,
		(f32)((rgba >> 16) & 0xff) / 255.0f,
		(f32)((rgba >> 8) & 0xff) / 255.0f,
		(f32)(rgba & 0xff) / 255.0f
	};
	return result;
}
