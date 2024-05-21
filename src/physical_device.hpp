#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>       // std::multimap
#include <stdexcept> // std::runtime_error
#include <vector>

#include "queue_family.hpp"

int rateDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
  // name, supported vulkan version, memory properties, queue families, extensions,
  // swap chain support, device type, etc
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

  // texture compression, 64-bit floats, multi viewport rendering, geometry shader,
  // tessellation shader, shader storage buffer, 16-bit floats, 8-bit integers, etc
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

  // application cannot function without geometry shaders
  if (!deviceFeatures.geometryShader) {
    return 0;
  }

  // application cannot function without a graphics queue
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
  if (!queueFamilyIndices.isComplete()) {
    return 0;
  }

  int score = 0;

  // discrete GPUs have a significant performance advantage
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }

  // maximum possible size of textures affects graphics quality
  score += deviceProperties.limits.maxImageDimension2D;

  return score;
}

// find a suitable physical device for the application instance
// surface is used to check for presentation support
// physicalDevice is filled with the chosen device
// queueFamilies is filled with the queue families supported by the device
void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                        VkPhysicalDevice& physicalDevice, QueueFamilyIndices& queueFamilies) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // sort candidates by increasing score
  std::multimap<int, VkPhysicalDevice> candidates;
  for (const auto& device : devices) {
    int score = rateDeviceSuitability(device, surface);
    candidates.insert(std::make_pair(score, device));
  }

  // check if the best candidate is suitable
  if (candidates.rbegin()->first > 0) {
    physicalDevice = candidates.rbegin()->second;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  // fill queueFamilies with the queue families supported by the chosen device
  queueFamilies = findQueueFamilies(physicalDevice, surface);
}
