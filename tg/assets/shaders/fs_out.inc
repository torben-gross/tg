layout(location = 0) out v4    out_normal_3xf8_metallic_1xf8;
layout(location = 1) out v4    out_albedo_3xf8_roughness_1xf8;

void tg_out(v3 n, v3 a, f32 m, f32 r)
{
    out_normal_3xf8_metallic_1xf8 = v4(n, m);
    out_albedo_3xf8_roughness_1xf8 = v4(a, r);
}
