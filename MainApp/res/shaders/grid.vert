#version 430

//16bit X 16bit Y
layout(location = 0) in uint vertex;
layout(location = 1) in uint colorId;

layout(std140, binding = 8) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

uniform float u_axis_size;
uniform float u_axis_offset;

out vec4 out_color;

const vec3 COLORS[3] = { {0, 0, 0}, {0, 0, 1}, {1, 0, 0} };

void main()
{
    vec3 position = vec3(0, 0, 0);

    position.x = u_axis_offset * u_axis_size + float((vertex & uint(0xffff0000)) >> 16) * u_axis_size;
    position.z = u_axis_offset * u_axis_size + float((vertex & uint(0x0000ffff))) * u_axis_size;
    position.x -= int(cameraPosition.x / u_axis_size);
    position.z += int(cameraPosition.z / u_axis_size);

    out_color = vec4(COLORS[colorId], 1);
    gl_Position = cameraViewProjection * vec4(position, 1.0);
}