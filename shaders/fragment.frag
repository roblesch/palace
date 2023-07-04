#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in float useNormalTexture;

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 texColor = texture(texSampler, uv);
	if (texColor.a < 0.5)
		discard;
	outColor = vec4(vec3(texColor), 1.0);
}