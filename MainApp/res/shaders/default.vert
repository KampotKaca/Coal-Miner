#version 430

// Input vertex attributes
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 uvPosition;

//layout(std140, binding = 32) uniform Camera
//{
//    mat4 cameraView;
//    mat4 cameraProjection;
//    mat4 cameraViewProjection;
//    vec3 cameraPosition;
//    vec3 cameraDirection;
//};

//uniform mat4 cameraViewProjection;

out vec4 out_color;

void main()
{
    // Calculate final vertex position
    out_color = vec4(uvPosition, 0, 1);
    gl_Position = vec4(vertexPosition, 1.0);
}