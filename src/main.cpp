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

// local
#include "instance.hpp"
#include "debug.hpp"
#include "physical_device.hpp"
#include "queue_family.hpp"
#include "device.hpp"


// constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;


// application class //

class Application {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow* window;    // window handle
  VkInstance instance;   // connection between application and vulkan library
  VkDevice device;       // logical device, used to interface with the GPU
  VkSurfaceKHR surface;  // surface to present images to
  VkQueue graphicsQueue; // handle to the graphics queue
  VkQueue presentQueue;  // handle to the presentation queue
  VkDebugUtilsMessengerEXT debugMessenger; // used to report validation layer errors
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // GPU handle
  QueueFamilyIndices queueFamilies; // queue families supported by the GPU

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
    createInstance(instance, debugMessenger);
    setupDebugMessenger(instance, debugMessenger);
    createSurface();
    pickPhysicalDevice(instance, surface, physicalDevice, queueFamilies);
    createLogicalDevice(physicalDevice, device, queueFamilies, &graphicsQueue, &presentQueue);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    // vulkan
    #ifndef NDEBUG
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    #endif
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    // glfw
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
    }
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