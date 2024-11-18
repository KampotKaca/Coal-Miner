#version 430 core
precision highp float;

const float EPSILON = .999999f;
const uint TERRAIN_CHUNK_SIZE = 64;
const uint TERRAIN_CHUNK_VOXEL_COUNT = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;
const uint TERRAIN_CHUNK_VOXEL_COUNT_SPLIT = TERRAIN_CHUNK_VOXEL_COUNT / 4;

const uint UV_SCALE = 16;
const float UV_STEP = 1.0f / UV_SCALE;

layout(std140, binding = 8) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

layout(std140, binding = 9) uniform GlobalLight
{
    vec4 lightColor;
    vec3 lightDirection;
};

const float faceColors[6] =
{
    0.8f, 0.8f, 0.8f, 0.8f, 1.0f, 0.6f
};

layout(std430, binding = 16) buffer VoxelBuffer
{
    uint[] voxels;
};

in flat uint out_faceId;
in flat uvec3 out_blockPos;
in vec3 out_lPos;
in vec2 out_facePos;
in vec3 out_ao_footprint;

in vec3 out_position;
in flat vec3 out_normal;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];
uniform uvec4 u_chunkIndex;

uniform vec3 u_ambientColor;

uvec3 round_vec3(vec3 v3)
{
    uvec3 result;
    if(fract(v3.x) >= EPSILON) result.x = uint(v3.x) + 1u;
    else result.x = uint(v3.x);

    if(fract(v3.y) >= EPSILON) result.y = uint(v3.y) + 1u;
    else result.y = uint(v3.y);

    if(fract(v3.z) >= EPSILON) result.z = uint(v3.z) + 1u;
    else result.z = uint(v3.z);
    return result;
}

vec3 diffuse_global_lighting(vec3 fragPosition, vec3 normal, vec3 ambientColor)
{
    float diffusePower = max(0.0f, dot(-lightDirection, normal));
    vec3 diffuseColor = lightColor.w * diffusePower * lightColor.rgb;

    return ambientColor + diffuseColor;
}

void main()
{
    uvec3 voxelPos = out_blockPos + round_vec3(out_lPos) - uvec3(out_faceId == 2u, out_faceId == 4u, out_faceId == 0u);
    uint id = voxelPos.y * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE + voxelPos.x * TERRAIN_CHUNK_SIZE + voxelPos.z;
    uint offset = (id % 4u) * 8u;
    uint bufferIndex = u_chunkIndex.w * TERRAIN_CHUNK_VOXEL_COUNT_SPLIT + uint(id * 0.25f);
    uint voxel = (voxels[bufferIndex] & (0xff << offset)) >> offset;

    voxel--;
    vec2 uv = vec2(voxel / UV_SCALE, voxel % UV_SCALE) + fract(out_facePos);
    uv *= UV_STEP;

    uv.y = 1.0f - uv.y;
    finalColor = texture(u_surfaceTex[out_faceId], uv);
    finalColor.rgb *= diffuse_global_lighting(out_position, out_normal, u_ambientColor);
    finalColor.rgb *= out_ao_footprint;
//    finalColor.rgb *= faceColors[out_faceId];
    finalColor.a = 1f;
}
