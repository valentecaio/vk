#version 450

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 lightSpace;
	vec4 lightPos;
} ubo;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec4 outShadowCoord;

// matrix to convert coordinates from [-1, 1] to [0, 1]
// we need this because the shadow map is in [0, 1] range
// but the shadow coord is in [-1, 1] range
const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

void main() {
  vec4 pos = ubo.model * vec4(inPos, 1.0);

  outColor = inColor;
  outNormal = mat3(ubo.model) * inNormal;
  outViewVec = -pos.xyz;
  outLightVec = normalize(ubo.lightPos.xyz - inPos);
  outShadowCoord = (biasMat * ubo.lightSpace * ubo.model) * vec4(inPos, 1.0);
  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
}

