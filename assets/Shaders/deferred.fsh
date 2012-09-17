#version 330

layout(std140) uniform LightBuffer
{
    vec4 kLights[24];
    int  kNumLights;
    int  __padding[3];
};

uniform sampler2D diffuseTex;

in vec3 int_WorldPos;
in vec3 int_Normal;
in vec2 int_TexCoord;

out vec4 out_Color[2];

void main()
{
    out_Color[0] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 albedo = texture(diffuseTex, int_TexCoord);
    
    for(int ii=0;ii<0;++ii) {
        vec3 light_pos = kLights[ii].xyz;
        float dist = distance(int_WorldPos, light_pos);
        if(dist > kLights[ii].w)
            continue;
        vec3 light_dir = int_WorldPos - light_pos;
        light_dir = normalize(-light_dir);
        float n_l = dot(light_dir, normalize(int_Normal));
        float attenuation = 1 - pow( clamp(dist/kLights[ii].w, 0.0f, 1.0f), 2);
        out_Color[0] += albedo * clamp(n_l, 0.0f, 1.0f) * attenuation;
    }
    vec3 norm = normalize(int_Normal);
    norm += 1.0f;
    norm /= 2.0f;
    out_Color[1] = vec4(norm, 1.0f);
}
