#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in float useNormalTexture;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(1.0, 1.0, -1.0);
const vec3 specColor = vec3(1.0);
const float shininess = 120.0;
const float Ka = 0.3;
const float Kd = 0.5;
const float Ks = 0.2;

vec3 getNormal()
{
	if (useNormalTexture < 0.5)
		return vertexNormal;

	// http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalSampler, uv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(pos);
	vec3 q2 = dFdy(pos);
	vec2 st1 = dFdx(uv);
	vec2 st2 = dFdy(uv);

	vec3 N = normalize(vertexNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main() {
	vec4 tex = texture(texSampler, uv);
	if (tex.a < 0.5)
		discard;

	// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
	vec3 texColor = tex.rgb;
	vec3 ambient = texColor;
	vec3 normal = getNormal();
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * texColor;
    vec3 viewDir = normalize(pos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);

    vec3 specular = specColor * spec;

    outColor = vec4(Ka * ambient + Kd * diffuse + Ks * specular, 1.0);
}