#version 430 core

const uint CHUNK_SIZE = 32;
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

//4bit uvX
//4bit uvY

//5bit BlockPositionZ
//5bit BlockPositionY
//5bit BlockPositionX

//3bit faceId

layout(std430, binding = 64) buffer VoxelBuffer
{
    uint voxelFaces[];
};

//2bit Id
layout(location = 0) in uint vertex;

//xyz index, w id
uniform uvec4 u_chunkIndex;

out flat uint out_faceId;
out vec2 out_uv;

void main()
{
    uint vert = vertex;
    ivec2 lPos = ivec2((vert & 2u) >> 1, vert & 1u);
    vert = vert >> 2;

    uint face = voxelFaces[u_chunkIndex.w * CHUNK_CUBE_COUNT + gl_InstanceID];
    ivec3 vertexPos;

    out_faceId = face & 7u;
    face = face >> 3;
    ivec3 blockPos = ivec3((face & 31744u) >> 10, (face & 992u) >> 5, face & 31u);
    face = face >> 15;

    ivec2 surfaceId = ivec2((face & 240u) >> 4, (face & 15u));
    face = face >> 8;

    switch(out_faceId)
    {
        case 0: //front
        vertexPos = ivec3(lPos.x, 1 - lPos.y, 1);
        out_uv = ((surfaceId + ivec2(0, 1)) * AXIS_SURFACE_OFFSET) + (ivec2(1, -1) * AXIS_SURFACE_OFFSET * lPos);
        break;
        case 1: //back
        vertexPos = ivec3(lPos, 0);
        out_uv = (surfaceId * AXIS_SURFACE_OFFSET) + (AXIS_SURFACE_OFFSET * lPos);
        break;
        case 2: //right
        vertexPos = ivec3(1, lPos.y, lPos.x);
        out_uv = (surfaceId * AXIS_SURFACE_OFFSET) + (AXIS_SURFACE_OFFSET * lPos);
        break;
        case 3: //left
        vertexPos = ivec3(0, lPos.y, 1 - lPos.x);
        out_uv = ((surfaceId + ivec2(1, 0)) * AXIS_SURFACE_OFFSET) + (ivec2(-1, 1) * AXIS_SURFACE_OFFSET * lPos);
        break;
        case 4: //top
        vertexPos = ivec3(lPos.x, 1, lPos.y);
        out_uv = (surfaceId * AXIS_SURFACE_OFFSET) + (AXIS_SURFACE_OFFSET * lPos);
        break;
        case 5: //bottom
        vertexPos = ivec3(lPos.x, 0, 1 - lPos.y);
        out_uv = (surfaceId * AXIS_SURFACE_OFFSET) + (AXIS_SURFACE_OFFSET * lPos);
        break;
    }

    gl_Position = cameraViewProjection * vec4(u_chunkIndex.xyz * CHUNK_SIZE - vec3(WORLD_EDGE, 0, WORLD_EDGE) + blockPos + vertexPos, 1.0);
}