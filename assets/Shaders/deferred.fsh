#version 330

uniform sampler2D kDiffuseTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec2 int_Depth;

out vec4 GBuffer[3];

void main()
{
    float spec_intensity = 0.8f;
    float spec_power = 0.5f;

    GBuffer[0] = vec4(texture(kDiffuseTex, int_TexCoord).rgb, spec_intensity);
    vec3 norm = normalize(int_Normal);
    norm += 1.0f;
    norm *= 0.5f;
    GBuffer[1] = vec4(norm, spec_power);
    GBuffer[2] = vec4(int_Depth.x/int_Depth.y);
}
