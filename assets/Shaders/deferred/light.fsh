#version 330

#define kDirectionalLight 0.0f
#define kPointLight 1.0f
#define kSpotLight 2.0f

struct Light 
{
    vec3    pos;
    float   size;
    vec3    dir;
    float   type;
    vec3    color;
    float   inner_cos;
    float   outer_cos;
};

uniform sampler2D GBuffer[4];

uniform mat4 kInverseViewProj;
uniform vec4 kLight[4];

uniform vec3 kCameraPosition;

in vec4 int_Pos;

out vec4 out_Color;

vec3 phong( vec3 light_dir, vec3 light_color,
            vec3 normal, vec3 albedo,
            vec3 dir_to_cam, float attenuation,
            float spec_power, float spec_intensity)
{
    float n_dot_l = clamp(dot(light_dir, normal), 0.0f, 1.0f);
    vec3 reflection = reflect(dir_to_cam, normal);
    float r_dot_l = clamp(dot(reflection, -light_dir), 0.0f, 1.0f);
    
    vec3 specular = vec3(min(1.0f, pow(r_dot_l, spec_power))) * light_color * spec_intensity;
    vec3 diffuse = albedo * light_color * n_dot_l;

    return attenuation * (diffuse + specular);
}

void main()
{
    Light light = Light(kLight[0].xyz, kLight[0].w,
                        kLight[1].xyz, kLight[1].w,
                        kLight[2].xyz, kLight[2].w,
                        kLight[3].x);
    // Calculate screen space UVs
    vec2 screen_pos = int_Pos.xy/int_Pos.w;
    vec2 uv =  screen_pos*0.5+0.5;

    // Read from textures
    vec3 albedo = texture(GBuffer[0], uv).rgb;
    float spec_intensity = texture(GBuffer[0], uv).a;
    vec3 normal = texture(GBuffer[1], uv).rgb;
    float spec_power = texture(GBuffer[1], uv).a;
    float depth = texture(GBuffer[3], uv).r;

    // Calculate the world position from the depth
    vec4 world_pos = vec4(screen_pos.xy, depth, 1.0f);
    world_pos = kInverseViewProj * world_pos;
    world_pos /= world_pos.w;

    normal *= 2.0f;
    normal -= 1.0f;
    
    vec3 dir_to_cam = normalize(kCameraPosition - world_pos.xyz);
    vec3 light_color = light.color.xyz;
    vec3 light_dir = light.dir.xyz;
    vec3 light_pos = light.pos;
    float light_type = light.type;
    float attenuation = 1.0f;

    // Dirctional lights and point lights are handled a little bit differently.
    // It might be more efficient to make two separate shaders rather than have
    // the different cases.
    if(light_type == kDirectionalLight) {
        light_dir = normalize(-light_dir);
    } else {
        light_dir = light_pos - world_pos.xyz;
        float dist = length(light_dir);
        if(dist > light.size)
            discard;
        light_dir = normalize(light_dir);
        attenuation = 1 - pow( clamp(dist/light.size, 0.0f, 1.0f), 2);
        if(light_type == kSpotLight) {
            vec3 V = normalize(world_pos.xyz - light.pos.xyz);
            float inner_cos = light.inner_cos;
            float outer_cos = light.outer_cos;
            float cos_dir = dot(V, normalize(light.dir));
            float spot_effect = smoothstep(outer_cos, inner_cos, cos_dir);
            attenuation *= spot_effect;
        }
    }

    //
    // Perform the actual shading
    //
    vec3 color = phong(light_dir, light_color,
                       normal, albedo,
                       dir_to_cam, attenuation,
                       spec_power, spec_intensity);

    // Only add ambient to directional lights
    // In the future, AO/GI/better lighting will handle the ambient
    if(light_type == kDirectionalLight) {
        color = color*0.9f + albedo*0.1f*light_color;
    }
    out_Color = vec4(color, 1.0f);
}
