#version 330

uniform sampler2D color_texture;
uniform sampler2D normal_texture;
uniform sampler2D position_texture;

uniform vec4 kLight;
uniform vec4 kColor;

in vec4 int_Pos;

out vec4 out_Color;

void main()
{
    vec2 pos  = int_Pos.xy/int_Pos.w;
    vec2 uv =  pos *0.5+0.5;

    vec3 albedo = texture(color_texture, uv).rgb;
    vec3 normal = texture(normal_texture, uv).rgb;
    vec3 world_pos = texture(position_texture, uv).rgb;

    normal *= 2.0f;
    normal -= 1.0f;
    normal = normalize(normal);

    vec3 light_pos = kLight.xyz;
    float dist = distance(world_pos, light_pos);
	    
    vec3 light_dir = world_pos - light_pos;
    light_dir = normalize(-light_dir);
    float n_l = dot(light_dir, normal);
    float attenuation = 1 - pow( clamp(dist/kLight.w, 0.0f, 1.0f), 2);
    out_Color = vec4(albedo * kColor.rgb * clamp(n_l, 0.0f, 1.0f) * attenuation, 1.0f);

}
