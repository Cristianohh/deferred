#version 330

uniform sampler2D GBuffer[3];

uniform vec4 kLight[2];

in vec4 int_Pos;

out vec4 out_Color;

void main()
{
    vec2 pos  = int_Pos.xy/int_Pos.w;
    vec2 uv =  pos *0.5+0.5;

    vec3 albedo = texture(GBuffer[0], uv).rgb;
    vec3 normal = texture(GBuffer[1], uv).rgb;
    vec3 world_pos = texture(GBuffer[2], uv).rgb;

    normal *= 2.0f;
    normal -= 1.0f;
    //normal = normalize(normal);

    vec3 light_pos = kLight[0].xyz;
    float dist = distance(world_pos, light_pos);
    
    vec3 light_dir = world_pos - light_pos;
    light_dir = normalize(-light_dir);
    float n_l = dot(light_dir, normal);
    float attenuation = 1 - pow( clamp(dist/kLight[0].w, 0.0f, 1.0f), 2);
    out_Color = vec4(albedo * kLight[1].rgb * clamp(n_l, 0.0f, 1.0f) * attenuation, 1.0f);
}
