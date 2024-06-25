#version 430 core

in flat uint out_faceId;
in vec2 out_uv;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];

void main()
{
    finalColor = texture(u_surfaceTex[out_faceId], out_uv);
}
