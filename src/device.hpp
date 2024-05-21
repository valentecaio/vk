#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept> // std::runtime_error
#include <vector>
#include <set>

// create a logical device and a graphics queue
void createLogicalDevice(VkPhysicalDevice physicalDevice, VkDevice& device,
                         QueueFamilyIndices indices, VkQueue *graphicsQueue,
                         VkQueue *presentQueue) {
  // contains a bool for every feature in Vulkan
  // enable the desired features here
  VkPhysicalDeviceFeatures deviceFeatures{};
  // deviceFeatures.samplerAnisotropy = VK_TRUE;

  // create queues for each queue family in indices
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f; // [0.0, 1.0] range, mandatory
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // logical device parameters
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
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

  // retrieve the queue handles
  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(),  0, presentQueue);
}

