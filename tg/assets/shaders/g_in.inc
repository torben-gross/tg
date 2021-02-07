layout(location = 0) out v4u    u_position_3xu32_normal_3xu8_metallic_1xu8;
layout(location = 1) out v4     u_albedo_3xf8_roughness_1xf8;

void tg_in(out v3 p, out v3 n, out v3 a, out f32 m, out f32 r)
{
    v4u v0 = texture(u_position_3xu32_normal_3xu8_metallic_1xu8, v_uv);
    v4 v1 = texture(u_albedo_3xf8_roughness_1xf8, v_uv);

    p = uintBitsToFloat(v0.xyz);
    n = v3(f32(v0.w & 0xff), f32((v0.w >> 8) & 0xff), f32((v0.w >> 16) & 0xff)) / v3(127.5) - v3(1.0);
    a = v1.xyz;
    m = f32((v0.w >> 24) & 0xff) / 255.0;
    r = v1.w;
}
