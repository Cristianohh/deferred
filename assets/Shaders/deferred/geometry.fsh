#version 330

uniform sampler2D kDiffuseTex;
uniform sampler2D kNormalTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec2 int_Depth;

in vec3 int_TangentWS;

out vec4 GBuffer[4];

// GBuffer format
//  [0] RGB: Albedo    
//  [1] RGB: WS Normal  A: Spec coefficient
//  [2] RGB: Spec Color A: Spec exponent
//  [3] R: Depth

void main()
{
    vec2 flipped_tex = vec2(int_TexCoord.x, -int_TexCoord.y); // Flip the tex coords on the y
    vec3 norm = normalize(texture(kNormalTex, flipped_tex).rgb*2.0 - 1.0f);
    vec3 albedo = texture(kDiffuseTex, flipped_tex).rgb;
    vec3 spec_color = vec3(1.0f);

    float spec_coefficient = 0.8f;
    float spec_exponent = 128.0f;

    vec3 N = normalize(int_Normal);
    vec3 T = normalize(int_TangentWS - dot(int_TangentWS, N)*N);
    vec3 B = cross(N,T);

    mat3 TBN = mat3(T, B, N);
    norm = normalize(TBN*norm);

    GBuffer[0] = vec4(albedo, 1.0f);
    norm += 1.0f;
    norm *= 0.5f;
    GBuffer[1] = vec4(norm, spec_coefficient);
    GBuffer[2] = vec4(spec_color, spec_exponent*(1/256.0f));
    GBuffer[3] = vec4(int_Depth.x/int_Depth.y);
}
