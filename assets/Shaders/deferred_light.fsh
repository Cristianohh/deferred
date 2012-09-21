#version 330

uniform sampler2D GBuffer[3];

uniform mat4 kInverseViewProj;
uniform vec4 kLight[2];

uniform vec3 kCameraPosition;

in vec4 int_Pos;

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
    // Calculate screen space UVs
    vec2 pos = int_Pos.xy/int_Pos.w;
    vec2 uv =  pos*0.5+0.5;

    // Read from textures
    vec3 albedo = texture(GBuffer[0], uv).rgb;
    float spec_intensity = texture(GBuffer[0], uv).a;
    vec3 normal = texture(GBuffer[1], uv).rgb;
    float spec_power = texture(GBuffer[1], uv).a;
    float depth = texture(GBuffer[2], uv).r;

    vec4 world_pos = vec4(pos.xy, depth, 1.0f);
    world_pos = kInverseViewProj * world_pos;
    world_pos /= world_pos.w;

    normal *= 2.0f;
    normal -= 1.0f;
    
    vec3 dir_to_cam = normalize(kCameraPosition - world_pos.xyz);
    if(kLight[1].a == 0.0f) {
        vec3 light_dir = kLight[0].xyz;
        vec3 light_color = kLight[1].xyz;
        
        light_dir = normalize(light_dir);
        float n_l = dot(light_dir, normal);

        out_Color = vec4(albedo*0.1f + 0.9f*phong(light_dir, light_color, normal, albedo, dir_to_cam, spec_power, spec_intensity), 1.0f);
    } else if(kLight[1].a == 1.0f) {
        vec3 light_dir = kLight[0].xyz - world_pos.xyz;
        vec3 light_color = kLight[1].xyz;
        float dist = length(light_dir);

        if(dist > kLight[0].w)
            discard;
            
        light_dir = normalize(light_dir);

        float n_l = dot(light_dir, normal);
        
        vec3 reflection = reflect(dir_to_cam, normal);
        float rDotL     = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
        vec3 spec       = vec3(min(1.0f, pow(rDotL, 1024)));
        //vec3 spec       = vec3(0.0f);

        float attenuation = 1 - pow( clamp(dist/kLight[0].w, 0.0f, 1.0f), 2);

        out_Color += vec4(albedo * 0.9f * kLight[1].rgb * clamp(n_l, 0.0f, 1.0f) * attenuation + spec*kLight[1].rgb, 1.0f);

        out_Color = vec4(phong(light_dir, light_color*attenuation, normal, albedo, dir_to_cam, spec_power, spec_intensity), 1.0f);
    }
}
