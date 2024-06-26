#pragma once

#include "../utils/common.hpp"
#include "texture.hpp"

namespace vk {

  VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
        return format;
      } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    throw std::runtime_error("failed to find supported format!");
  }


  VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
    return findSupportedFormat(
      physicalDevice,
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
  }


  void createDepthResources(VkDevice device, VkPhysicalDevice physicalDevice,
                            VkExtent2D swapChainExtent, VkImage &depthImage,
                            VkDeviceMemory &depthImageMemory, VkImageView &depthImageView) {
    VkFormat depthFormat = findDepthFormat(physicalDevice);

    createImage(device, physicalDevice, swapChainExtent.width, swapChainExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
  }

} // namespace vk
