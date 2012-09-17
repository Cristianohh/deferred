#version 330

uniform sampler2D color_texture;
uniform sampler2D normal_texture;
uniform sampler2D position_texture;

layout(std140) uniform LightBuffer
{
    vec4 kLights[24];
    int  kNumLights;
    int  _padding[3];
};

in vec4 int_Pos;

out vec4 out_Color;

void main()
{
    vec3 pos  = int_Pos.xyz*int_Pos.w;
    vec2 uv =  pos.xy *0.5+0.5;

    vec3 albedo = texture(color_texture, uv).rgb;
    vec3 normal = texture(normal_texture, uv).rgb;
    vec3 world_pos = texture(position_texture, uv).rgb;

    normal *= 2.0f;
    normal -= 1.0f;
    normal = normalize(normal);

    for(int ii=0;ii<kNumLights;++ii) {
        vec3 light_pos = kLights[ii].xyz;
        float dist = distance(world_pos, light_pos);

		if(dist > kLights[ii].w)
		  continue;
        
        vec3 light_dir = world_pos - light_pos;
        light_dir = normalize(-light_dir);
        float n_l = dot(light_dir, normal);
        float attenuation = 1 - pow( clamp(dist/kLights[ii].w, 0.0f, 1.0f), 2);
        out_Color += vec4(albedo * clamp(n_l, 0.0f, 1.0f) * attenuation, 1.0f);
    }
}