#version 330

layout(std140) uniform LightBuffer
{
    struct {
        vec4 pos;
        vec4 color;
    } kLight[255];
    int  kNumLights;
    int  _padding[3];
};

uniform sampler2D kDiffuseTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

out vec4 out_Color;

void main()
{
    out_Color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 albedo = texture(kDiffuseTex, int_TexCoord);
    
    for(int ii=0;ii<kNumLights;++ii) {
        vec3 light_pos = kLight[ii].pos.xyz;
        float dist = distance(int_WorldPos, light_pos);

		if(dist > kLight[ii].pos.w) {
            continue; // TODO: This causes rendering artifacts on Intel
        }
        
        vec3 light_dir = int_WorldPos - light_pos;
        light_dir = normalize(-light_dir);
        float n_l = dot(light_dir, normalize(int_Normal));
        float attenuation = 1 - pow( clamp(dist/kLight[ii].pos.w, 0.0f, 1.0f), 2);
        out_Color += albedo * kLight[ii].color * clamp(n_l, 0.0f, 1.0f) * attenuation;
    }
}
