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

#include "instance.hpp"
#include "debug.hpp"
#include "physical_device.hpp"
#include "queue_family.hpp"
#include "device.hpp"
#include "swap_chain.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "framebuffer.hpp"
#include "command_pool.hpp"
#include "utils.hpp"


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

  // vulkan
  VkInstance instance;   // connection between application and vulkan library
  VkDevice device;       // logical device, used to interface with the GPU
  VkSurfaceKHR surface;  // surface to present images to
  VkDebugUtilsMessengerEXT debugMessenger; // used to report validation layer errors
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // GPU handle

  // queues
  QueueFamilyIndices queueFamilies; // queue families indices in the physical device
  VkQueue graphicsQueue;            // handle to the graphics queue
  VkQueue presentQueue;             // handle to the presentation queue
  
  // swap chain
  VkSwapchainKHR swapChain;             // handle to the swap chain
  VkFormat swapChainImageFormat;        // format of the swap chain images
  VkExtent2D swapChainExtent;           // resolution of the swap chain images
  std::vector<VkImage> swapChainImages; // handles to the swap chain images
  std::vector<VkImageView> swapChainImageViews;

  // pipeline
  VkPipeline graphicsPipeline;     // handle to the graphics pipeline
  VkPipelineLayout pipelineLayout; // uniform values for shaders
  VkRenderPass renderPass;         // render pass, collection of attachments, subpasses, and dependencies

  // framebuffers
  std::vector<VkFramebuffer> swapChainFramebuffers;

  // commands
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  // sync objects
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;


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
    createSwapChain(physicalDevice, device, surface, window, swapChain, swapChainImages, swapChainImageFormat, swapChainExtent);
    createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
    createRenderPass(device, swapChainImageFormat, renderPass);
    createGraphicsPipeline(device, swapChainExtent, renderPass, pipelineLayout, graphicsPipeline);
    createFramebuffers(device, swapChainImageViews, swapChainFramebuffers, swapChainExtent, renderPass);
    createCommandPool(device, physicalDevice, surface, queueFamilies, commandPool);
    createCommandBuffer(device, commandPool, commandBuffer);
    createSyncObjects();
  }


  void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start in signaled state for the first frame

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores or fences!");
    }
  }


  void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(commandBuffer, 0); // 0 flags
    recordCommandBuffer(commandBuffer, renderPass, swapChainExtent, swapChainFramebuffers, imageIndex, graphicsPipeline);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {swapChain};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr; // Optional
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(device);
  }

  void cleanup() {
    // vulkan
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
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