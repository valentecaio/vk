#pragma once

// This file contains the debug messenger setup and callback
// The debug messenger is used to report validation layer errors
// It is not necessary for the application to run, but it is useful for debugging

#include "../utils/common.hpp"

namespace vk {

// The two functions below are extension functions, not part of the core.
// They are not automatically loaded, so we need to load manually

// create a debug messenger
VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
  #ifdef NDEBUG
    return VK_SUCCESS;
  #endif

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  return func ? func(instance, pCreateInfo, pAllocator, pDebugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

// destroy a debug messenger
void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMsgr,
                                   const VkAllocationCallbacks* pAllocator) {
  #ifdef NDEBUG
    return VK_SUCCESS;
  #endif

  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(instance, debugMsgr, pAllocator);
}



// validation layer callback, static to avoid the this pointer
// VKAPI_ATTR and VKAPI_CALL are necessary for the function to be called by vulkan
// simple print to stderr
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {

  std::string severity = "";
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      severity = "VERBOSE";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      severity = "INFO";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      severity = "WARNING";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      severity = "ERROR";
      break;
    default:
      severity = "UNKNOWN";
      break;
  }

  std::cerr << "VALIDATION [" << severity << "]: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  // enable some message severities, all except info
  createInfo.messageSeverity = 0
                            //  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                            //  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  // enable all message types  
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                          | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // optional
}

// create and setup a debug messenger
void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debugMsgr) {
  #ifdef NDEBUG
    return;
  #endif

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);
  if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMsgr) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

} // namespace vk
