#version 430 core

//1bit zId
//1bit yId
//1bit xId

//4bit BlockPositionZ
//8bit BlockPositionY
//4bit BlockPositionX

//4bit chunkIdX
//4bit chunkIdZ

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

layout(location = 0) in uint vertex;

out vec4 out_color;

void main()
{
    uint vert = vertex;
    ivec3 lPos = ivec3((vert & uint(4)) >> 2, (vert & uint(2)) >> 1, vert & uint(1));
    vert = vert >> 3;
    ivec3 blockPos = ivec3((vert & uint(0xf000)) >> 12, (vert & uint(0x0ff0)) >> 4, vert & uint(0x000f));
    vert = vert >> 16;

    out_color = vec4(lPos, 1.0);
    gl_Position = cameraViewProjection * vec4(blockPos + lPos, 1.0);
}