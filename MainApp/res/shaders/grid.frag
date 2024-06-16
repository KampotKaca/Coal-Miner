#version 430

uniform vec4 u_color = vec4(1, 1, 1, 1);
out vec4 finalColor;

void main()
{
    finalColor = u_color;
}
