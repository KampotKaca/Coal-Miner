#version 430

//16bit X 16bit Y
layout(location = 0) in uint vertex;

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

uniform float u_axis_size;
uniform float u_axis_offset;

void main()
{
    vec3 position = vec3(0, 0, 0);

    position.x = u_axis_offset + float((vertex & uint(0xffff0000)) >> 16) * u_axis_size;
    position.z = u_axis_offset + float((vertex & uint(0x0000ffff))) * u_axis_size;
    position.x -= int(cameraPosition.x / u_axis_size);
    position.z += int(cameraPosition.z / u_axis_size);

    gl_Position = cameraViewProjection * vec4(position, 1.0);
}