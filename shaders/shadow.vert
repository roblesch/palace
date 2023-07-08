#version 450

layout(binding = 0) uniform UBO 
{
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
	vec3 lightPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    float useNormalTexture;
} constants;

layout(location = 0) in vec3 pos;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main()
{
	gl_Position =  ubo.lightProj * ubo.lightView * constants.model * vec4(pos, 1.0);
}