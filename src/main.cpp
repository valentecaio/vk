// glfw
#define GLFW_INCLUDE_VULKAN // replace the include <vulkan/vulkan.h>, needs to be before glfw3.h
#include <GLFW/glfw3.h>

// glm
// openGL uses [-1, 1] for depth, vulkan uses [0, 1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// standard
#include <iostream>  // std::cerr, std::endl
#include <stdexcept> // std::exception, std::runtime_error
#include <cstdlib>   // EXIT_SUCCESS, EXIT_FAILURE
#include <vector>
#include <cstring>   // strcmp

// local
#include "debug.hpp"
#include "device.hpp"
#include "queue_family.hpp"

// constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif


// application class //

class Application {
public:
  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
  };

  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow* window;  // window handle
  VkInstance instance; // connection between application and vulkan library
  VkDebugUtilsMessengerEXT debugMessenger; // used to report validation layer errors
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // handle to the GPU
  QueueFamilyIndices queueFamilies;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable openGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // disable resizing

    // 4th argument is monitor (fullscreen), 5th is share (only for openGL)
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    // int c;
    // auto monitors = glfwGetMonitors(&c);
    // window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", monitors[0], nullptr);
  }

  void initVulkan() {
    createInstance();
    setupDebugMessenger(instance, debugMessenger);
    pickPhysicalDevice(instance, physicalDevice);
    findQueueFamilies(physicalDevice, queueFamilies);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    // vulkan
    if (enableValidationLayers)
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);

    // glfw
    glfwDestroyWindow(window);
    glfwTerminate();
  }


  void createInstance() {
    // optional information used for optimization
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // required information, global
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // extensions
    listExtensions();
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // for debug messenger
    if (enableValidationLayers) {
      if (!checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
      }
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      // we need this to cover instance creation and destruction events by the debug messenger
      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr; // no debug messenger
    }

    // create instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
      throw std::runtime_error("failed to create instance!");
    }
  }

  std::vector<const char*> getRequiredExtensions() {
    // glfw extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // extensions used by the validation layers
    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
  }

  // check if all validation layers are available
  bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
      bool layerFound = false;
      for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound)
        return false;
    }
    return true;
  }

};

int main() {
  Application app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}