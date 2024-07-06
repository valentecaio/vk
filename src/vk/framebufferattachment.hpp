#pragma once

#include "../utils/common.hpp"

namespace vk {

// framebuffer attachment used in render passes
class FrameBufferAttachment {
  public:
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;

    // destroy the image, memory, and view
    void destroy(VkDevice device) {
      vkDestroyImage(device, image, nullptr);
      vkFreeMemory(device, mem, nullptr);
      vkDestroyImageView(device, view, nullptr);
    }
};

} // namespace vk
