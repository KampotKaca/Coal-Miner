#version 430 core
precision highp float;

const uint CHUNK_SIZE = 64;
const uint CHUNK_CUBE_COUNT = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

const int WORLD_EDGE = 100000 * int(CHUNK_SIZE);

const uint MAX_AXIS_SURFACE_TYPE = 16;
const float AXIS_SURFACE_OFFSET = 1.0f / (MAX_AXIS_SURFACE_TYPE);
const uint MAX_SUFACE_TYPE = MAX_AXIS_SURFACE_TYPE * MAX_AXIS_SURFACE_TYPE;

const vec3 AO_FOOTPRINT[4] =
{
    { 1.0f, 1.0f, 1.0f },
    { 0.75f, 0.75f, 0.75f },
    { 0.5f, 0.5f, 0.5f },
    { 0.25f, 0.25f, 0.25f },
};

const vec3 NORMAL_FOOTPRINT[6] =
{
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f },
    { 1.0f, 0.0f, 0.0f },
    { -1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
};

layout(std140, binding = 8) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

//X
//18bit BlockPosition
//12bit Size
//2bit AO

//Y
//3bit FaceId
//2bit Id
layout(location = 0) in uvec2 vertex;

//xyz index, w id
uniform uvec4 u_chunkIndex;

out flat uint out_faceId;
out flat uvec3 out_blockPos;
out vec3 out_lPos;
out vec2 out_facePos;
out vec3 out_ao_footprint;

out vec3 out_position;
out flat vec3 out_normal;

void main()
{
    uint vertX = vertex.x;
    uint vertY = vertex.y;

    uint aoPrint = vertX & 3u;
    vertX >>= 2;
    out_ao_footprint = AO_FOOTPRINT[aoPrint];

    ivec2 size = ivec2((vertX & 4032u) >> 6u, (vertX & 63u));
    size.x++;
    size.y++;
    vertX >>= 12;

    out_blockPos = uvec3((vertX & 258048u) >> 12u, (vertX & 4032u) >> 6u, vertX & 63u);
    vertX >>= 18;

    ivec2 lPos = ivec2((vertY & 2u) >> 1u, vertY & 1u);
    vertY >>= 2;

    out_faceId = vertY & 7u;
    vertY >>= 3;

    ivec3 vertexPos;

    switch(out_faceId)
    {
        case 0: //front
        vertexPos = ivec3(lPos.x, 1 - lPos.y, 1);
        vertexPos.xy *= size;
        out_facePos = vertexPos.xy;
        break;
        case 1: //back
        vertexPos = ivec3(lPos, 0);
        vertexPos.xy *= size;
        out_facePos = vertexPos.xy;
        break;
        case 2: //right
        vertexPos = ivec3(1, lPos.y, lPos.x);
        vertexPos.yz *= size;
        out_facePos = vertexPos.zy;
        break;
        case 3: //left
        vertexPos = ivec3(0, lPos.y, 1 - lPos.x);
        vertexPos.yz *= size;
        out_facePos = vertexPos.zy;
        break;
        case 4: //top
        vertexPos = ivec3(lPos.x, 1, lPos.y);
        vertexPos.zx *= size;
        out_facePos = vertexPos.zx;
        break;
        case 5: //bottom
        vertexPos = ivec3(lPos.x, 0, 1 - lPos.y);
        vertexPos.zx *= size;
        out_facePos = vertexPos.zx;
        break;
    }

    out_normal = NORMAL_FOOTPRINT[out_faceId];
    out_lPos = vec3(vertexPos);
    out_position = u_chunkIndex.xyz * CHUNK_SIZE - vec3(WORLD_EDGE, 0, WORLD_EDGE) + vertexPos + out_blockPos;
    gl_Position = cameraViewProjection * vec4(out_position, 1.0);
}