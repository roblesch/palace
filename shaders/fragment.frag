#version 450

#extension GL_KHR_vulkan_glsl : enable

const vec3 specColor = vec3(1.0);
const float shininess = 120.0;
const float Ka = 0.4;
const float Kd = 0.6;
const float Ks = 0.2;
const float ambient = 0.5;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vertexNormal;
layout(location = 4) in float useNormalTexture;
layout(location = 5) in vec3 lightDir;
layout(location = 6) in vec4 inShadowCoord;

layout(location = 0) out vec4 outColor;

float textureProj(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture( shadowMap, shadowCoord.st + off ).r;
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
        {
            shadow = ambient;
        }
    }
    return shadow;
}

vec3 getNormal()
{
	if (useNormalTexture < 0.5)
		return vertexNormal;

	vec3 tangentNormal = texture(normalSampler, uv).xyz * 2.0 - 1.0;

	// Transform normals by perturbation
	// http://www.thetenthplanet.de/archives/1180
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

	vec3 texColor = tex.rgb;
	vec3 ambient = texColor;
	vec3 normal = getNormal();
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * texColor;
    vec3 viewDir = normalize(pos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    vec3 specular = specColor * spec;
    
//    float shadow = textureProj(inShadowCoord, vec2(0.0));

//    outColor = shadow * vec4(Ka * ambient + Kd * diffuse + Ks * specular, 1.0);
    outColor = texture(shadowMap, inShadowCoord.st);
//    outColor = vec4(vec3(shadow), 1.0);
}
