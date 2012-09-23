#version 330

#define kDirectionalLight 0.0f
#define kPointLight 1.0f
#define kSpotLight 2.0f

struct Light 
{
    vec3    pos;
    float   size;
    vec3    dir;
    float   type;
    vec3    color;
    float   inner_cos;
    float   outer_cos;
};

layout(std140) uniform LightBuffer
{
    Light kLight[511];
    int  kNumLights;
    int  _padding[3];
};

uniform sampler2D kAlbedoTex;
uniform sampler2D kNormalTex;
uniform sampler2D kSpecularTex;

uniform vec3  kSpecularColor;
uniform float kSpecularCoefficient;
uniform float kSpecularExponent;

uniform vec3 kCameraPosition;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;
in vec3 int_TangentWS;

out vec4 out_Color;


vec3 phong( vec3 light_dir, vec3 light_color,
            vec3 normal, vec3 albedo,
            vec3 dir_to_cam, float attenuation,
            vec3 spec_color, float spec_power, float spec_coefficient)
{
    float n_dot_l = clamp(dot(light_dir, normal), 0.0f, 1.0f);
    vec3 reflection = reflect(dir_to_cam, normal);
    float r_dot_l = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
    
    vec3 specular = vec3(min(1.0f, pow(r_dot_l, spec_power))) * light_color * spec_color * spec_coefficient;
    vec3 diffuse = albedo * light_color * n_dot_l;

    return attenuation * (diffuse + specular);
}

void main()
{
    vec2 flipped_tex = vec2(int_TexCoord.x, -int_TexCoord.y); // Flip the tex coords on the y
    vec3 albedo = texture(kAlbedoTex, flipped_tex).rgb;
    vec3 normal = normalize(texture(kNormalTex, flipped_tex).rgb*2.0 - 1.0f);
    vec3 specular = texture(kSpecularTex, flipped_tex).rgb + kSpecularColor;
    vec3 world_pos = int_WorldPos;
    vec3 dir_to_cam = normalize(kCameraPosition - world_pos);

    vec3 N = normalize(int_Normal);
    vec3 T = normalize(int_TangentWS - dot(int_TangentWS, N)*N);
    vec3 B = cross(N,T);

    mat3 TBN = mat3(T, B, N);
    normal = normalize(TBN*normal);

    for(int ii=0;ii<kNumLights;++ii) {
        Light current_light = kLight[ii];
        vec3 light_color = current_light.color.xyz;
        vec3 light_dir;
        float light_type = current_light.type;
        float attenuation = 1.0f;

        // Dirctional lights and point lights are handled a little bit differently.
        // It might be more efficient to make two separate shaders rather than have
        // the different cases.
        if(light_type == kDirectionalLight) {
            light_dir = normalize(-current_light.dir.xyz);
        } else {
            light_dir = current_light.pos.xyz - world_pos.xyz;
            float dist = length(light_dir);
            float size = current_light.size;
            if(dist > size) {
                continue;
            }
            light_dir = normalize(light_dir);
            attenuation = 1 - pow( clamp(dist/size, 0.0f, 1.0f), 2);
            if(light_type == kSpotLight) {
                vec3 V = normalize(world_pos.xyz - current_light.pos.xyz);
                float inner_cos = current_light.inner_cos;
                float outer_cos = current_light.outer_cos;
                float cos_dir = dot(V, normalize(current_light.dir));
                float spot_effect = smoothstep(outer_cos, inner_cos, cos_dir);
                attenuation *= spot_effect;
            }
        }

        //
        // Perform the actual shading
        //
        vec3 color = phong(light_dir, light_color,
                           normal, albedo,
                           dir_to_cam, attenuation,
                           specular, kSpecularExponent, kSpecularCoefficient);

        // Only add ambient to directional lights
        // In the future, AO/GI/better lighting will handle the ambient
        if(light_type == kDirectionalLight) {
            color = color*0.9f + albedo*0.1f*light_color;
        }
        out_Color += vec4(color, 1.0f);
    }
}
