#version 330

uniform mat4 kWorldViewProj;

layout(location=0) in vec4 in_Position;

void main()
{
    gl_Position = kWorldViewProj * in_Position;
}
