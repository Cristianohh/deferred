#version 330

#define kDirectionalLight 0.0f
#define kPointLight 1.0f

struct Light 
{
    vec4 pos;
    vec4 color;
};

layout(std140) uniform LightBuffer
{
    Light kLight[127];
    int  kNumLights;
    int  _padding[3];
};

uniform sampler2D kDiffuseTex;
uniform sampler2D kNormalTex;

uniform vec3 kCameraPosition;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

in vec3 int_NormalCam;
in vec3 int_TangentCam;
in vec3 int_BitangentCam;

out vec4 out_Color;


vec3 phong( vec3 light_dir, vec3 light_color,
            vec3 normal, vec3 albedo,
            vec3 dir_to_cam, float attenuation,
            float spec_power, float spec_intensity)
{
    float n_dot_l = clamp(dot(light_dir, normal), 0.0f, 1.0f);
    vec3 reflection = reflect(dir_to_cam, normal);
    float r_dot_l = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
    
    vec3 specular = vec3(min(1.0f, pow(r_dot_l, spec_power))) * light_color * spec_intensity;
    vec3 diffuse = albedo * light_color * n_dot_l;

    return attenuation * (diffuse + specular);
}

void main()
{
    vec3 albedo = texture(kDiffuseTex, int_TexCoord).rgb;
    vec3 normal = normalize(texture(kNormalTex, int_TexCoord).rgb*2.0 - 1.0f);
    vec3 world_pos = int_WorldPos;
    vec3 dir_to_cam = normalize(kCameraPosition - world_pos);
    float spec_power = 128.0f;
    float spec_intensity = 0.8f;

    mat3 TBN = transpose(mat3(int_TangentCam, int_BitangentCam, int_NormalCam));

    dir_to_cam = TBN * dir_to_cam;

    for(int ii=0;ii<kNumLights;++ii) {
        Light current_light = kLight[ii];
        vec3 light_color = current_light.color.xyz;
        vec3 light_dir = current_light.pos.xyz;
        float light_type = current_light.color.a;
        float attenuation = 1.0f;

        light_dir = TBN * light_dir;

        // Dirctional lights and point lights are handled a little bit differently.
        // It might be more efficient to make two separate shaders rather than have
        // the different cases.
        if(light_type == kDirectionalLight) {
            light_dir = normalize(light_dir);
        } else if(light_type == kPointLight) {
            light_dir = current_light.pos.xyz - world_pos.xyz;
            float dist = length(light_dir);
            float size = current_light.pos.w;
            if(dist > size) {
                continue;
            }
            light_dir = normalize(light_dir);
            attenuation = 1 - pow( clamp(dist/size, 0.0f, 1.0f), 2);
        }

        //
        // Perform the actual shading
        //
        vec3 color = phong(light_dir, light_color,
                           normal, albedo,
                           dir_to_cam, attenuation,
                           spec_power, spec_intensity);

        // Only add ambient to directional lights
        // In the future, AO/GI/better lighting will handle the ambient
        if(light_type == kDirectionalLight) {
            color = color*0.9f + albedo*0.1f*light_color;
        }
        out_Color += vec4(color, 1.0f);
    }
}