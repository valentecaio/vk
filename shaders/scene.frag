#version 450

layout (binding = 1) uniform sampler2D shadowMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec4 inShadowCoord;

layout (location = 0) out vec4 outFragColor;

// ambient light intensity
#define ambient 0.1

// samples the shadow map to determine if the fragment is in shadow
float textureProj(vec4 shadowCoord) {
	float shadow = 1.0;
	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
		float dist = texture(shadowMap, shadowCoord.st).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z) {
			shadow = ambient;
		}
	}
	return shadow;
}


void main() {	
	float shadow = textureProj(inShadowCoord / inShadowCoord.w);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 diffuse = max(dot(N, L), ambient) * inColor;

	outFragColor = vec4(diffuse * shadow, 1.0);
}
