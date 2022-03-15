#define b32                    bool
#define f32                    float
#define i32                    int
#define u32                    uint

#define v2                     vec2
#define v2i                    ivec2
#define v2u                    uvec2
#define v3                     vec3
#define v3i                    ivec3
#define v3u                    uvec3
#define v4                     vec4
#define v4i                    ivec4
#define v4u                    uvec4

#define m2                     mat2
#define m3                     mat3
#define m4                     mat4

#define TG_F32_MAX             3.402823466e+38f
#define TG_F32_MIN             (-TG_F32_MAX)
#define TG_F32_MIN_POSITIVE    1.175494351e-38f
#define TG_F32_EPSILON         1.192092896e-07f
#define TG_I32_MIN             (-2147483647 - 1)
#define TG_I32_MAX             2147483647
#define TG_U32_MAX             0xffffffffu

#define TG_IN(      location_index, content) layout(location = location_index)      in  content
#define TG_IN_FLAT( location_index, content) layout(location = location_index) flat in  content
#define TG_OUT(     location_index, content) layout(location = location_index)      out content
#define TG_OUT_FLAT(location_index, content) layout(location = location_index) flat out content

#define TG_SSBO(     binding_index, content) layout(std430, set = 0, binding = binding_index) buffer  tg_ssbo_ ## binding_index { content }
#define TG_UBO(      binding_index, content) layout(        set = 0, binding = binding_index) uniform tg_ubo_  ## binding_index { content }
#define TG_SAMPLER2D(binding_index, name   ) layout(        set = 0, binding = binding_index) uniform sampler2D name

// Result is not normalized
v3 tg_lerp_corner_ray_directions_nn(v3 ray_bl, v3 ray_br, v3 ray_tr, v3 ray_tl, f32 x, f32 y)
{
	return mix(
        mix(ray_bl, ray_tl, y),
        mix(ray_br, ray_tr, y),
        x);
}

v3 tg_lerp_corner_ray_directions(v3 ray_bl, v3 ray_br, v3 ray_tr, v3 ray_tl, f32 x, f32 y)
{
	return normalize(mix(
        mix(ray_bl, ray_tl, y),
        mix(ray_br, ray_tr, y),
        x));
}
