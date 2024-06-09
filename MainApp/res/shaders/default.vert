#version 430

// Input vertex attributes
in vec3 vertexPosition;

//layout(std140, binding = 32) uniform Camera
//{
//    mat4 cameraView;
//    mat4 cameraProjection;
//    mat4 cameraViewProjection;
//    vec3 cameraPosition;
//    vec3 cameraDirection;
//};

void main()
{
    // Calculate final vertex position
    gl_Position = vec4(vertexPosition, 1.0);
}