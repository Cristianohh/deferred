#version 330

uniform sampler2D kDiffuseTex;
uniform sampler2D kNormalTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec2 int_Depth;

in vec3 int_NormalWS;
in vec3 int_TangentWS;

out vec4 GBuffer[3];

void main()
{
    float spec_intensity = 0.8f;
    float spec_power = 128.0f;

    vec3 norm = normalize(texture(kNormalTex, int_TexCoord).rgb*2.0 - 1.0f);

    vec3 N = normalize(int_NormalWS);
    vec3 T = normalize(int_TangentWS - dot(int_TangentWS, N)*N);
    vec3 B = cross(N,T);

    mat3 TBN = mat3(T, B, N);
    norm = normalize(TBN*norm);

    GBuffer[0] = vec4(texture(kDiffuseTex, int_TexCoord).rgb, spec_intensity);
    norm += 1.0f;
    norm *= 0.5f;
    GBuffer[1] = vec4(norm, spec_power);
    GBuffer[2] = vec4(int_Depth.x/int_Depth.y);
}
