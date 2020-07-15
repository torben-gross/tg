#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_ssao;

layout(location = 0) out vec4 out_color;

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(u_ssao, 0));
    float result = 0.0;
    for (int x = -2; x < 2; x++)
    {
        for (int y = -2; y < 2; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(u_ssao, clamp(v_uv + offset, 0.0, 1.0)).x;
        }
    }
    out_color.x = clamp(result / 16.0, 0.0, 1.0);
}
