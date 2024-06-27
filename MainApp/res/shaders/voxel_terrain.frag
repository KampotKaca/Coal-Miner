#version 430 core

in flat uint out_faceId;
in vec2 out_uv;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];
uniform vec4 u_outlineColor;

void main()
{
//    finalColor = vec4(out_uv, 0, 1);
    finalColor = texture(u_surfaceTex[out_faceId], vec2(out_uv.x, 1 - out_uv.y));
//    if(finalColor.a < .2f) finalColor = vec4(0, 0, 0, 1);
    finalColor.a = 1;
}
