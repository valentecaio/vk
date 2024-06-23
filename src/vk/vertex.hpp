#pragma once

#include "../utils/common.hpp"

namespace vk {

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // VERTEX or INSTANCE
    return bindingDescription;
  }

  // float:  VK_FORMAT_R32_SFLOAT
  // double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
  // vec2:   VK_FORMAT_R32G32_SFLOAT
  // vec3:   VK_FORMAT_R32G32B32_SFLOAT
  // vec4:   VK_FORMAT_R32G32B32A32_SFLOAT
  // ivec2:  VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
  // uvec4:  VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
  }
};

} // namespace vk
