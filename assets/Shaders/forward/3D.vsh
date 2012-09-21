#version 330

uniform mat4 kViewProj;
uniform mat4 kWorld;

layout(location=0) in vec4 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec3 in_Tangent;
layout(location=3) in vec3 in_BiTangent;
layout(location=4) in vec2 in_TexCoord;

out vec3 int_WorldPos;
out vec3 int_Normal;
out vec2 int_TexCoord;
out vec3 int_TangentWS;

void main()
{
    mat3 world3 = mat3(kWorld);
    vec4 world_pos = kWorld * in_Position;
    gl_Position = kViewProj * world_pos;

    int_WorldPos    = vec3(world_pos);
    int_Normal      = world3 * in_Normal;
    int_TangentWS   = world3 * in_Tangent;
    int_TexCoord    = in_TexCoord;
}
