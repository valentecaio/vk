#version 450

layout (binding = 0) uniform UBO {
	mat4 depthMVP;
} ubo;

layout (location = 0) in vec3 inPos;

out gl_PerVertex {
  vec4 gl_Position;   
};


void main() {
	gl_Position =  ubo.depthMVP * vec4(inPos, 1.0);
}