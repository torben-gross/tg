layout(location = 0) out v4u    out_position_3xu32_normal_3xu8_metallic_1xu8;
layout(location = 1) out v4     out_albedo_3xf8_roughness_1xf8;

void tg_out(v3 p, v3 n, v3 a, f32 m, f32 r)
{
    v4u v0;
    v0.xyz = floatBitsToUint(p);
    v0.w = uint(n.x * 127.5 + 127.5) | (uint(n.y * 127.5 + 127.5) << 8) | (uint(n.z * 127.5 + 127.5) << 16) | (uint(m * 255.0) << 24);

    v4 v1 = v4(a, r);

    out_position_3xu32_normal_3xu8_metallic_1xu8 = v0;
    out_albedo_3xf8_roughness_1xf8 = v1;
}
