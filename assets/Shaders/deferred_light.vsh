#version 330

uniform mat4 kViewProj;
uniform mat4 kWorld;

layout(location=0) in vec4 in_Position;

out vec4 int_Pos;

void main()
{
    vec4 world_pos = kWorld * in_Position;
    gl_Position = kViewProj * world_pos;

    int_Pos = gl_Position;
}
