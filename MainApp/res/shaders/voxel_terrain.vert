#version 430

layout(location = 0) in vec3 vertexPosition;

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

void main()
{
    gl_Position = cameraViewProjection * vec4(vertexPosition, 1.0);
}