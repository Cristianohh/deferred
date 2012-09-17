#version 330

uniform sampler2D diffuseTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

out vec4 GBuffer[3];

void main()
{
    GBuffer[0] = texture(diffuseTex, int_TexCoord);
    vec3 norm = normalize(int_Normal);
    norm += 1.0f;
    norm /= 2.0f;
    GBuffer[1] = vec4(norm, 1.0f);
    GBuffer[2] = vec4(int_WorldPos, 1.0f);
}
