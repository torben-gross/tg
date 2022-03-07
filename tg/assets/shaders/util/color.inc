u32 tg_color_pack(v4 rgba)
{
	u32 r = u32(rgba.x * 255.0f);
	u32 g = u32(rgba.y * 255.0f);
	u32 b = u32(rgba.z * 255.0f);
	u32 a = u32(rgba.w * 255.0f);
	u32 result = r << 24 | g << 16 | b << 8 | a;
	return result;
}

v4 tg_color_unpack(u32 rgba)
{
	v4 result = v4(
		f32((rgba >> 24) & 0xff) / 255.0f,
		f32((rgba >> 16) & 0xff) / 255.0f,
		f32((rgba >>  8) & 0xff) / 255.0f,
		f32( rgba        & 0xff) / 255.0f
	);
	return result;
}
