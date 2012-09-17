#version 330

uniform sampler2D color_texture;
uniform sampler2D normal_texture;
uniform sampler2D position_texture;

uniform vec4 kLight;

in vec4 int_Pos;

out vec4 out_Color;

void main()
{
    vec3 pos  = int_Pos.xyz/int_Pos.w;
    vec2 uv =  pos.xy *0.5+0.5;
    
    vec3 world_pos = texture(position_texture, uv).rgb;
    vec3 albedo = texture(color_texture, uv).rgb;
    vec3 normal = texture(normal_texture, uv).rgb;

    vec3 light_dir = world_pos - kLight.rgb;
    light_dir = normalize(-light_dir);
    
    float dist = distance(world_pos, kLight.rgb);
    
    normal *= 2.0f;
    normal -= 1.0f;
    normal = normalize(normal);
    
    float n_l = dot(light_dir, normal);
    float attenuation = 1 - pow( clamp(dist/kLight.w, 0.0f, 1.0f), 2);
    out_Color = vec4(albedo * clamp(n_l, 0.0f, 1.0f) * attenuation, 1.0f);
}
