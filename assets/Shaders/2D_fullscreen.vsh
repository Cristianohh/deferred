#version 330

layout(location=0) in vec4 in_Position;
layout(location=1) in vec2 in_TexCoord;

out vec2 int_TexCoord;

void main()
{
    gl_Position = in_Position;
    int_TexCoord = in_TexCoord;
}
