#version 430 core

//1bit zId
//1bit yId
//1bit xId

//5bit BlockPositionZ
//5bit BlockPositionY
//5bit BlockPositionX

//5bit chunkIdX
//3bit chunkIdY
//5bit chunkIdZ

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
    ivec3 blockPos = ivec3((vert & uint(31744)) >> 10, (vert & uint(992)) >> 5, vert & uint(31));
    vert = vert >> 15;

    out_color = vec4(lPos, 1.0);
    gl_Position = cameraViewProjection * vec4(blockPos + lPos, 1.0);
}