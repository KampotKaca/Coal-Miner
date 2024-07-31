#version 430 core

const uint CHUNK_SIZE = 64;
const uint CHUNK_CUBE_COUNT = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

const int WORLD_EDGE = 100000 * int(CHUNK_SIZE);

const uint MAX_AXIS_SURFACE_TYPE = 16;
const float AXIS_SURFACE_OFFSET = 1.0f / (MAX_AXIS_SURFACE_TYPE);
const uint MAX_SUFACE_TYPE = MAX_AXIS_SURFACE_TYPE * MAX_AXIS_SURFACE_TYPE;

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

//6bit BlockPositionZ
//6bit BlockPositionY
//6bit BlockPositionX

//3bit FaceId

//4bit SizeX
//4bit SizeY

//2bit Id
layout(location = 0) in uint vertex;

//xyz index, w id
uniform uvec4 u_chunkIndex;

out flat uint out_faceId;
out vec3 out_lPos;
out vec2 out_facePos;

void main()
{
    uint vert = vertex;
    ivec2 lPos = ivec2((vert & 2u) >> 1, vert & 1u);
    vert >>= 2;

    ivec2 size = ivec2((vert & 240u) >> 4, (vert & 15u));
    vert >>= 8;

    ivec3 vertexPos;

    out_faceId = vert & 7u;
    vert >>= 3;
    ivec3 blockPos = ivec3((vert & 258048u) >> 12, (vert & 4032u) >> 6, vert & 63u);
    vert >>= 18;

    switch(out_faceId)
    {
        case 0: //front
        vertexPos = ivec3(lPos.x, 1 - lPos.y, 1);
//        vertexPos.xy *= size;
        out_facePos = vertexPos.xy;
        break;
        case 1: //back
        vertexPos = ivec3(lPos, 0);
//        vertexPos.xy *= size;
        out_facePos = vertexPos.xy;
        break;
        case 2: //right
        vertexPos = ivec3(1, lPos.y, lPos.x);
//        vertexPos.zy *= size;
        out_facePos = vertexPos.zy;
        break;
        case 3: //left
        vertexPos = ivec3(0, lPos.y, 1 - lPos.x);
//        vertexPos.zy *= size;
        out_facePos = vertexPos.zy;
        break;
        case 4: //top
        vertexPos = ivec3(lPos.x, 1, lPos.y);
//        vertexPos.xz *= size;
        out_facePos = vertexPos.xz;
        break;
        case 5: //bottom
        vertexPos = ivec3(lPos.x, 0, 1 - lPos.y);
//        vertexPos.xz *= size;
        out_facePos = vertexPos.xz;
        break;
    }

    out_lPos = blockPos + vertexPos;
    vec3 pos = u_chunkIndex.xyz * CHUNK_SIZE - vec3(WORLD_EDGE, 0, WORLD_EDGE) + out_lPos;
    gl_Position = cameraViewProjection * vec4(pos, 1.0);
}