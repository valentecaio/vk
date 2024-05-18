#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept> // std::runtime_error
#include <vector>

void createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices, VkDevice& device) {
  // contains a bool for every feature in Vulkan
  // enable the desired features
  VkPhysicalDeviceFeatures deviceFeatures{};

  // create a single queue from the graphics queue family for this logical device
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f; // [0.0, 1.0] range, mandatory
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // logical device parameters
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;

  // logical device extensions
  createInfo.enabledExtensionCount = 0;

  // the validation layers per device are deprecated
  // recent versions of Vulkan ignore the following parameters
  // we set them here to be compatible with older versions
  #ifndef NDEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  #else
    createInfo.enabledLayerCount = 0;
  #endif

  // logical device creation
  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
}

