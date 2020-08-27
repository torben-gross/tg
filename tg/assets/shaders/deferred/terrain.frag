#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4    v_position;
layout(location = 1) in vec3    v_normal;

layout(location = 0) out vec4    out_position;
layout(location = 1) out vec4    out_normal;
layout(location = 2) out vec4    out_albedo;
layout(location = 3) out vec4    out_metallic_roughness_ao;

void main()
{
    out_position    = v_position;
    out_normal      = vec4(normalize(v_normal), 1.0);

    float d         = dot(out_normal.xyz, vec3(0.0, 1.0, 0.0));
    
    out_albedo      = vec4(204.0/255.0, 255.0/255.0, 0.0/255.0, 1.0);
    if (d < 0.7)
    {
        out_albedo = vec4(0.7, 0.5, 0.5, 1.0);
    }
    out_metallic_roughness_ao = vec4(0.0, 1.0, 1.0, 1.0);

    //out_albedo      = vec4(0.3f, 0.6f, 0.1f, 1.0);
    //out_metallic_roughness_ao = vec4(0.2, 0.8, 1.0, 1.0);
    //
    //if (d < 0.7)
    //{
    //    //out_albedo = vec4(0.7, 0.5, 0.5, 1.0);
    //    out_albedo = vec4(0.5, 0.5, 0.5, 1.0);
    //}
    //else if (d > 0.9 && v_position.y > 128.0)
    //{
    //    out_albedo = vec4(2.0, 2.0, 2.5, 1.0);
    //    out_metallic_roughness_ao.y = 0.4;
    //}
}