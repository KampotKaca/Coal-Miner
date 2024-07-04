#version 430

in vec4 out_color;

uniform vec4 u_color = vec4(1, 1, 1, 1);
out vec4 finalColor;

void main()
{
    finalColor = out_color * u_color;
}
