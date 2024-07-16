#version 430 core

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

//layout(std430, binding = 48) VoxelBuffer
//{
//    uint[] voxels;
//};

in flat uint out_faceId;
in vec3 out_lPos;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];
uniform uvec4 u_chunkIndex;

void main()
{
    vec3 rem = mod(out_lPos, 1);

    finalColor = texture(u_surfaceTex[out_faceId], vec2(rem.x, 1 - rem.y));
    finalColor.rgb *= faceColors[out_faceId];
}
