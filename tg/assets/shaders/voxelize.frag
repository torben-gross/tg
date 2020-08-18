#version 450

layout(location = 0) in vec4 v_position;

layout(set = 0, binding = 2, R8) uniform image3D u_image_3d;

void main()
{
    vec3 image_size = vec3(imageSize(u_image_3d));

    float x = floor(image_size.x * ((v_position.x + 1.0) / 2.0));
    float y = floor(image_size.y * (1.0 - ((v_position.y + 1.0) / 2.0)));
    float z = floor(image_size.z * (1.0 - v_position.z));

    imageStore(u_image_3d, ivec3(x, y, z), vec4(1.0));
}
