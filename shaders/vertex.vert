#version 450

layout(binding = 0) uniform UniformBuffer {
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
	vec3 lightPos;
} uniforms;

layout(push_constant) uniform PushConstants {
    mat4 model;
    float useNormalTexture;
} constants;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragUv;
layout(location = 3) out vec3 vertNormal;
layout(location = 4) out float useNormalTexture;

void main() {
    vec4 vertPos = uniforms.view * constants.model * vec4(pos, 1.0);
    gl_Position = uniforms.proj * vertPos;
    fragPos = vec3(vertPos) / vertPos.w;
    fragColor = color;
    fragUv = uv;
    vertNormal = normalize(transpose(inverse(mat3(constants.model))) * normal);
    useNormalTexture = constants.useNormalTexture;
}