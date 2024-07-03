#version 450

layout(binding = 1) uniform sampler2D texSampler;

// input
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

// output
layout(location = 0) out vec4 outColor;

void main() {
  outColor = texture(texSampler, fragTexCoord);          // 1 possum
  // outColor = texture(texSampler, fragTexCoord * 2.0); // 4 possums
  // outColor = vec4(fragTexCoord, 0.0, 1.0);            // gradient
}