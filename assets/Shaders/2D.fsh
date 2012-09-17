#version 330

uniform sampler2D diffuseTex;

in vec2 int_TexCoord;

out vec4 out_Color;

void main()
{
	out_Color = texture(diffuseTex, int_TexCoord);
}
