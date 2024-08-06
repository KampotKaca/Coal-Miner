#version 430 core
precision highp float;

const uint TERRAIN_CHUNK_SIZE = 64;
const uint TERRAIN_CHUNK_VOXEL_COUNT = TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE;
const uint TERRAIN_CHUNK_VOXEL_COUNT_SPLIT = TERRAIN_CHUNK_VOXEL_COUNT / 4;

const uint UV_SCALE = 16;
const float UV_STEP = 1.0 / UV_SCALE;

//layout(std140, binding = 33) uniform Light
//{
//    vec3 lightPosition;
//    vec3 lightColor;
//    float lightLuminosity;
//};

const float faceColors[6] =
{
    0.8f, 0.8f, 0.8f, 0.8f, 1.0f, 0.6f
};

layout(std430, binding = 48) buffer VoxelBuffer
{
    uint[] voxels;
};

in flat uint out_faceId;
in vec3 out_lPos;
in vec2 out_facePos;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];
uniform uvec4 u_chunkIndex;

void main()
{
    ivec3 voxelPos = ivec3(out_lPos - vec3(out_faceId == 2, out_faceId == 4, out_faceId == 0));
    uint id = voxelPos.y * TERRAIN_CHUNK_SIZE * TERRAIN_CHUNK_SIZE + voxelPos.x * TERRAIN_CHUNK_SIZE + voxelPos.z;
    uint offset = (id % 4) * 8;
    uint bufferIndex = u_chunkIndex.w * TERRAIN_CHUNK_VOXEL_COUNT_SPLIT + uint(id * 0.25);
    uint voxel = (voxels[bufferIndex] & (0xff << offset)) >> offset;

    voxel--;
    vec2 uv = vec2((voxel / UV_SCALE) * UV_STEP, (voxel % UV_SCALE) * UV_STEP) +
                    mod(out_facePos, 1.0f) * UV_STEP;

    uv.y = 1 - uv.y;
    finalColor = texture(u_surfaceTex[out_faceId], uv);
    finalColor.rgb *= faceColors[out_faceId];
}
