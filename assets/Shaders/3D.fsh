#version 330

struct Light 
{
        vec4 pos;
        vec4 color;
};

layout(std140) uniform LightBuffer
{
    Light kLight[255];
    int  kNumLights;
    int  _padding[3];
};

uniform sampler2D kDiffuseTex;

uniform vec3 kCameraPosition;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

out vec4 out_Color;

void main()
{
    out_Color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 albedo = texture(kDiffuseTex, int_TexCoord);
    vec3 normal = normalize(int_Normal);
    
    for(int ii=0;ii<kNumLights;++ii) {
        vec3 light_pos = kLight[ii].pos.xyz;
        vec3 light_dir = light_pos - int_WorldPos;
        float dist = length(light_dir);

		if(dist > kLight[ii].pos.w) {
           // continue; // TODO: This causes rendering artifacts on Intel
        }
        
        light_dir = normalize(light_dir);
        float n_l = dot(light_dir, normal);
        float attenuation = 1 - pow( clamp(dist/kLight[ii].pos.w, 0.0f, 1.0f), 2);

        vec3 dir_to_cam = normalize(kCameraPosition - int_WorldPos);
        vec3 reflection = reflect(dir_to_cam, normal);
        float rDotL     = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
        vec3 spec       = vec3(min(1.0f, pow(rDotL, 128.0f)));

        out_Color += albedo * kLight[ii].color * clamp(n_l, 0.0f, 1.0f) * attenuation + vec4(spec*kLight[ii].color.rgb,1.0f);
    }
}
