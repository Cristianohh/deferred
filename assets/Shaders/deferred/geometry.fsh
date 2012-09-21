#version 330

uniform sampler2D kDiffuseTex;
uniform sampler2D kNormalTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec2 int_Depth;

in vec3 int_NormalCam;
in vec3 int_TangentCam;
in vec3 int_BitangentCam;

out vec4 GBuffer[3];

void main()
{
    float spec_intensity = 0.8f;
    float spec_power = 128.0f;

    vec3 norm = normalize(texture(kNormalTex, int_TexCoord).rgb*2.0 - 1.0f);    
    mat3 TBN = transpose(mat3(int_TangentCam, int_BitangentCam, int_NormalCam));

    GBuffer[0] = vec4(texture(kDiffuseTex, int_TexCoord).rgb, spec_intensity);
    norm = TBN * -norm;
    norm = normalize(norm);
    norm += 1.0f;
    norm *= 0.5f;
    GBuffer[1] = vec4(norm, spec_power);
    GBuffer[2] = vec4(int_Depth.x/int_Depth.y);
}
