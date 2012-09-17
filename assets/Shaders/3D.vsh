#version 330

layout(std140) uniform PerFrame
{
    mat4 kViewProj;
};

layout(std140) uniform PerObject
{
    mat4 kWorld;
};

layout(location=0) in vec4 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec2 in_TexCoord;

out vec3 int_WorldPos;
out vec3 int_Normal;
out vec2 int_TexCoord;

void main()
{
    vec4 world_pos = kWorld * in_Position;
    gl_Position = kViewProj * world_pos;

    int_WorldPos = vec3(world_pos);
    int_Normal = mat3(kWorld) * in_Normal;
    int_TexCoord = in_TexCoord;
}
