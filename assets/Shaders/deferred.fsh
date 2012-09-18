#version 330

uniform sampler2D kDiffuseTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec2 int_Depth;

out vec4 GBuffer[3];

void main()
{
    GBuffer[0] = vec4(texture(kDiffuseTex, int_TexCoord).rgb, 1.0f);
    vec3 norm = normalize(int_Normal);
    norm += 1.0f;
    norm *= 0.5f;
    GBuffer[1] = vec4(norm, 1.0f);
    GBuffer[2] = vec4(int_WorldPos, 1.0f);
}
