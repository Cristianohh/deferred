#version 330

uniform mat4 kView;
uniform mat4 kProj;
uniform mat4 kWorld;

layout(location=0) in vec4 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec3 in_Tangent;
layout(location=3) in vec3 in_BiTangent;
layout(location=4) in vec2 in_TexCoord;

out vec3 int_WorldPos;
out vec3 int_Normal;
out vec2 int_TexCoord;
out vec2 int_Depth;

void main()
{
    vec4 world_pos = kWorld * in_Position;
    vec4 view_pos = kView * world_pos;
    gl_Position = kProj * view_pos;

    int_WorldPos = world_pos.xyz;
    int_Normal = mat3(kWorld) * in_Normal;
    int_TexCoord = in_TexCoord;
    
	int_Depth.x = gl_Position.z;
	int_Depth.y = gl_Position.w;
}
