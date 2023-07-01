#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUv;

layout(push_constant) uniform constants {
    mat4 transform;
} consts;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * consts.transform * vec4(pos, 1.0);
    fragColor = color;
    fragUv = uv;
}