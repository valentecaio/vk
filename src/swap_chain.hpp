#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <cstdint>   // uint32_t
#include <limits>    // std::numeric_limits
#include <algorithm> // std::clamp
#include <stdexcept> // std::runtime_error

#include "queue_family.hpp"

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;      // min/max number of images, min/max width and height of images
  std::vector<VkSurfaceFormatKHR> formats;    // pixel format, color space (srgb, linear, etc)
  std::vector<VkPresentModeKHR> presentModes; // presentation modes (vsync, tearing, etc)
};

// query swap chain support details for a given physical device and surface
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  // capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &details.capabilities);

  // formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, details.formats.data());
  }

  // present modes
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

// choose the surface format for the swap chain
// try to use the srgb format if available, otherwise use the first available format
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

// choose the presentation mode for the swap chain
// there are 4 options here:
// VK_PRESENT_MODE_IMMEDIATE_KHR: images are submitted right away, may cause tearing
// VK_PRESENT_MODE_FIFO_KHR: vsync, waits for the vertical blank
// VK_PRESENT_MODE_FIFO_RELAXED_KHR: vsync, only waits if the queue is empty
// VK_PRESENT_MODE_MAILBOX_KHR: "triple buffer", waits for the vertical blank, discards the previous image
// we will use VK_PRESENT_MODE_MAILBOX_KHR if available, otherwise we will use
// VK_PRESENT_MODE_FIFO_KHR, which is the only mode guaranteed to be available
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

// choose the swap extent for the swap chain
// the swap extent is the resolution of the swap chain images
// glfwGetFramebufferSize is used to get the resolution of the window,
// but it uses "screen coordinates" which may be different from pixels
// on some systems, so we need to choose the resolution that best matches the window
VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
  }
}

// create the swap chain
// the swap chain is a queue of images that are displayed to the screen
// the images are presented to the screen in the order they were acquired
// the swap chain is created with a minimum number of images, a format, a resolution, and a presentation mode
// we return the swap chain, the swap chain images, the format, and the resolution
void createSwapChain(VkPhysicalDevice physDevice, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window,
                     VkSwapchainKHR& swapChain, std::vector<VkImage>& swapChainImages,
                     VkFormat& swapChainImageFormat, VkExtent2D& swapChainExtent) {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDevice, surface);

  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = chooseSwapExtent(window, swapChainSupport.capabilities);

  // number of images in the swap chain
  // one more than the minimum to allow waiting for the driver to complete operations
  // then clamp the number of images to the maximum if it is not 0
  // (0 is a special value that means there is no maximum)
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = swapChainExtent;
  createInfo.imageArrayLayers = 1; // always 1 unless stereoscopic 3D
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly to images

  QueueFamilyIndices indices = findQueueFamilies(physDevice, surface);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  // if the graphics and presentation queues are different, there are two options:
  // VK_SHARING_MODE_CONCURRENT: images can be used across multiple queues without explicit ownership transfers
  // VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and ownership must be explicitly transferred
  // we will use VK_SHARING_MODE_CONCURRENT if the queues are different,
  // otherwise we will use VK_SHARING_MODE_EXCLUSIVE (best performance)
  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // optional
    createInfo.pQueueFamilyIndices = nullptr; // optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // no transform
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // no blending with other windows
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE; // ignore pixels that are hidden by other windows (better performance)

  // no resizing for now
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  // retrieve the swap chain images
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}