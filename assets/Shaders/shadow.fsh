#version 330

layout(location=0) out float out_Depth;

void main()
{
    out_Depth = gl_FragCoord.z;
}
