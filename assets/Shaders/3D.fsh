#version 330

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

uniform vec3 kCameraPosition;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

out vec4 out_Color;


vec3 phong( vec3 light_dir, vec3 light_color,
            vec3 normal, vec3 albedo,
            vec3 dir_to_cam, float spec_power, float spec_intensity)
{
    float n_dot_l = clamp(dot(light_dir, normal), 0.0f, 1.0f);
    vec3 reflection = reflect(dir_to_cam, normal);
    float r_dot_l = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
    
    vec3 specular = vec3(min(1.0f, pow(r_dot_l, spec_power))) * light_color * spec_intensity;
    vec3 diffuse = albedo * light_color * n_dot_l;

    return diffuse + specular;
}

void main()
{
    vec4 albedo = texture(kDiffuseTex, int_TexCoord);
    vec3 normal = normalize(int_Normal);
    vec3 dir_to_cam = normalize(kCameraPosition - int_WorldPos);

    out_Color = albedo * 0.1f;
    for(int ii=0;ii<kNumLights;++ii) {
        if(kLight[ii].color.a == 0.0f) {
            vec3 light_dir = kLight[ii].pos.xyz;
            
            light_dir = normalize(light_dir);
            float n_l = dot(light_dir, normal);

            out_Color += albedo * kLight[ii].color * clamp(n_l, 0.0f, 1.0f);
        } else if(kLight[ii].color.a == 1.0f) {
            vec3 light_pos = kLight[ii].pos.xyz;
            vec3 light_dir = light_pos - int_WorldPos;
            float dist = length(light_dir);

            if(dist > kLight[ii].pos.w) {
                continue; // TODO: This causes rendering artifacts on Intel
            }
            
            light_dir = normalize(light_dir);
            float n_l = dot(light_dir, normal);
            float attenuation = 1 - pow( clamp(dist/kLight[ii].pos.w, 0.0f, 1.0f), 2);

            vec3 reflection = reflect(dir_to_cam, normal);
            float rDotL     = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
            vec3 spec       = vec3(min(1.0f, pow(rDotL, 128.0f)));
            //vec3 spec       = vec3(0.0f);

            out_Color += (albedo * kLight[ii].color * clamp(n_l, 0.0f, 1.0f) * attenuation)*0.9f + vec4(spec*kLight[ii].color.rgb,1.0f);
        }
    }
}
