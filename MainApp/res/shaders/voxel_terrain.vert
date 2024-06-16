#version 430

//3bits vIndex
layout(location = 0) in uint vertex;

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

out vec4 out_color;

void main()
{
//    vec3 position = vec3(float((vertex & uint(4)) >> 2), float((vertex & uint(2)) >> 1), float(vertex & uint(1)));
    vec3 position = vec3(float((vertex & uint(4)) >> 2), float((vertex & uint(2)) >> 1), float(vertex & uint(1)));

    out_color = vec4(position, 1.0);
    gl_Position = cameraViewProjection * vec4(position, 1.0);
}