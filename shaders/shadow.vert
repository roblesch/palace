#version 450

layout(binding = 0) uniform UniformBuffer {
    mat4 cameraView;
    mat4 cameraProj;
    mat4 lightView;
    mat4 lightProj;
    vec4 lightPos;
} uniforms;

layout(push_constant) uniform PushConstants {
    mat4 model;
    float useNormalTexture;
} constants;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

void main() {
    vec4 vertPos = uniforms.lightView * constants.model * vec4(pos, 1.0);
    gl_Position = uniforms.lightProj * vertPos;
}
