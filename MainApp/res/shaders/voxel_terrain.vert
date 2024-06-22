#version 430 core

layout(std140, binding = 32) uniform Camera
{
    mat4 cameraView;
    mat4 cameraProjection;
    mat4 cameraViewProjection;
    vec3 cameraPosition;
    vec3 cameraDirection;
};

//5bit BlockPositionZ
//5bit BlockPositionY
//5bit BlockPositionX

//3bit faceId

layout(std430, binding = 64) buffer VoxelBuffer
{
    uint voxelFaces[];
};

//2bit Id
layout(location = 0) in uint vertex;

out vec4 out_color;

void main()
{
    uint vert = vertex;
    ivec2 lPos = ivec2((vert & 2u) >> 1, vert & 1u);
    vert = vert >> 2;

    uint face = voxelFaces[gl_InstanceID];
    ivec3 vertexPos;

    switch(face & 7u)
    {
        case 0: //front
        vertexPos = ivec3(lPos.x, 1 - lPos.y, 1);
        break;
        case 1: //back
        vertexPos = ivec3(lPos, 0);
        break;
        case 2: //right
        vertexPos = ivec3(1, lPos.y, lPos.x);
        break;
        case 3: //left
        vertexPos = ivec3(0, lPos.y, 1 - lPos.x);
        break;
        case 4: //top
        vertexPos = ivec3(lPos.x, 1, lPos.y);
        break;
        case 5: //bottom
        vertexPos = ivec3(lPos.x, 0, 1 - lPos.y);
        break;
    }
    face = face >> 3;

    ivec3 blockPos = ivec3((face & 31744u) >> 10, (face & 992u) >> 5, face & 31u);
    face = face >> 15;

    out_color = vec4(vertexPos, 1.0);
    gl_Position = cameraViewProjection * vec4(blockPos + vertexPos, 1.0);
}