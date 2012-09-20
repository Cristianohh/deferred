#version 330

uniform sampler2D GBuffer[3];

uniform mat4 kInverseViewProj;
uniform vec4 kLight[2];

uniform vec3 kCameraPosition;

in vec4 int_Pos;

out vec4 out_Color;

void main()
{
    vec2 pos = int_Pos.xy/int_Pos.w;
    vec2 uv =  pos*0.5+0.5;

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
    
    if(kLight[1].a == 0.0f) {
        vec3 light_dir = kLight[0].xyz;
        
        light_dir = normalize(light_dir);
        float n_l = dot(light_dir, normal);

        out_Color = vec4(albedo * kLight[1].rgb * clamp(n_l, 0.0f, 1.0f), 1.0f);
    } else if(kLight[1].a == 1.0f) {
        vec3 light_dir = kLight[0].xyz - world_pos.xyz;
        float dist = length(light_dir);
            
        light_dir = normalize(light_dir);

        float n_l = dot(light_dir, normal);
        
        vec3 dir_to_cam = normalize(kCameraPosition - world_pos.xyz);
        vec3 reflection = reflect(dir_to_cam, normal);
        float rDotL     = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
        //vec3 spec       = vec3(min(1.0f, pow(rDotL, 1024)));
        vec3 spec       = vec3(0.0f);

        float attenuation = 1 - pow( clamp(dist/kLight[0].w, 0.0f, 1.0f), 2);

        out_Color = vec4(albedo * kLight[1].rgb * clamp(n_l, 0.0f, 1.0f) * attenuation + spec*kLight[1].rgb, 1.0f);
    }
}
