#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 lightSpace;
	vec4 lightPos;
	float zNear;
	float zFar;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;


void main() {
	float depth = texture(samplerColor, inUV).r;

  float n = ubo.zNear;
  float f = ubo.zFar;
  float z = depth;
  float linearDepth = (2.0 * n) / (f + n - z * (f - n));

	outFragColor = vec4(vec3(1.0-linearDepth), 1.0);
}