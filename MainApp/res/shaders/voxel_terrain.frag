#version 430 core

//layout(std140, binding = 33) uniform Light
//{
//    vec3 lightPosition;
//    vec3 lightColor;
//    float lightLuminosity;
//};

in flat uint out_faceId;
in vec2 out_uv;

out vec4 finalColor;

uniform sampler2D u_surfaceTex[6];

void main()
{
    finalColor = texture(u_surfaceTex[out_faceId], vec2(out_uv.x, 1 - out_uv.y));
}
