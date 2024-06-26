#pragma once

#include "../utils/common.hpp"
#include "../utils/utils.hpp"

#include "buffer.hpp"
#include "command.hpp"
#include "debug.hpp"
#include "depth.hpp"
#include "descriptor.hpp"
#include "device.hpp"
#include "framebuffer.hpp"
#include "instance.hpp"
#include "model.hpp"
#include "physical_device.hpp"
#include "pipeline.hpp"
#include "queue_family.hpp"
#include "render_pass.hpp"
#include "swap_chain.hpp"
#include "texture.hpp"
#include "vertex.hpp"


namespace vk {

// kee·lau·ay·uh
class Kilauea {
  public:
    // constructors
    Kilauea() = default;
    Kilauea(GLFWwindow* window) : window(window) {};


    void init() {
      loadModel(vertices, indices);
      createInstance(instance);
      setupDebugMessenger(instance, debugMsgr);
      createSurface(instance, surface);
      pickPhysicalDevice(instance, surface, physicalDevice, queueFamilies);
      createLogicalDevice(physicalDevice, device, queueFamilies, &graphicsQueue, &presentQueue);
      createSwapChain(physicalDevice, device, surface, window, swapChain, swapChainImages, swapChainImageFormat, swapChainExtent);
      createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
      createRenderPass(device, swapChainImageFormat, findDepthFormat(physicalDevice), renderPass);
      createDescriptorSetLayout(device, descriptorSetLayout);
      createGraphicsPipeline(device, swapChainExtent, renderPass, useDynamicStates, descriptorSetLayout, pipelineLayout, graphicsPipeline);
      createCommandPool(device, physicalDevice, surface, queueFamilies, commandPool);
      createDepthResources(device, physicalDevice, swapChainExtent, depthImage, depthImageMemory, depthImageView);
      createFramebuffers(device, swapChainExtent, renderPass, swapChainImageViews, depthImageView, swapChainFramebuffers);
      createTextureImage(device, physicalDevice, commandPool, graphicsQueue, textureImage, textureImageMemory);
      createTextureImageView(device, textureImage, textureImageView);
      createTextureSampler(device, physicalDevice, textureSampler);
      createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue, vertices, vertexBuffer, vertexBufferMemory);
      createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue, indices, indexBuffer, indexBufferMemory);
      createUniformBuffers(device, physicalDevice, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped);
      createDescriptorPool(device, descriptorPool);
      createDescriptorSets(device, descriptorPool, descriptorSetLayout, textureImageView, textureSampler, uniformBuffers, descriptorSets);
      createCommandBuffers(device, commandPool, commandBuffers);
      createSyncObjects();
    }


    void cleanup() {
      cleanupSwapChain();

      // texture
      vkDestroySampler(device, textureSampler, nullptr);
      vkDestroyImageView(device, textureImageView, nullptr);
      vkDestroyImage(device, textureImage, nullptr);
      vkFreeMemory(device, textureImageMemory, nullptr);

      // uniform buffers and descriptor sets
      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
      }
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

      // graphics pipeline
      vkDestroyPipeline(device, graphicsPipeline, nullptr);
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
      vkDestroyRenderPass(device, renderPass, nullptr);

      // vertex and index buffers
      vkDestroyBuffer(device, vertexBuffer, nullptr);
      vkFreeMemory(device, vertexBufferMemory, nullptr);
      vkDestroyBuffer(device, indexBuffer, nullptr);
      vkFreeMemory(device, indexBufferMemory, nullptr);

      // semaphores and fences
      for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
      }

      vkDestroyCommandPool(device, commandPool, nullptr);
      vkDestroyDevice(device, nullptr);

      destroyDebugUtilsMessengerEXT(instance, debugMsgr, nullptr);

      vkDestroySurfaceKHR(instance, surface, nullptr);
      vkDestroyInstance(instance, nullptr);
    }

    void drawFrame() {
      // wait for the last frame to be finished (the fence is signaled by the present queue)
      vkWaitForFences(device, 1, &inFlightFences[curFrame], VK_TRUE, UINT64_MAX);

      // acquire an image from the swap chain
      uint32_t imageIndex;
      auto result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[curFrame], VK_NULL_HANDLE, &imageIndex);
      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // window was resized
        framebufferResized = false;
        recreateSwapChain();
        return;
      } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
      }

      // now that we have the image, we can reset the fence to block the next frame
      vkResetFences(device, 1, &inFlightFences[curFrame]);

      // update the uniform buffer
      updateUniformBuffer();

      // record the command buffer
      vkResetCommandBuffer(commandBuffers[curFrame], 0); // 0 flags
      recordCommandBuffer(commandBuffers[curFrame], renderPass, swapChainExtent,
                          swapChainFramebuffers, imageIndex, graphicsPipeline,
                          useDynamicStates, vertexBuffer, indexBuffer,
                          (uint32_t)indices.size(), pipelineLayout, descriptorSets[curFrame]);

      // semaphores used to signal that the image is ready
      VkSemaphore waitSemaphores[]   = {imageAvailableSemaphores[curFrame]};
      VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[curFrame]};
      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

      // submit the command buffer
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = waitSemaphores;
      submitInfo.pWaitDstStageMask = waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffers[curFrame];
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = signalSemaphores;
      if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[curFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
      }

      // present the image
      VkSwapchainKHR swapChains[] = {swapChain};
      VkPresentInfoKHR presentInfo{};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = signalSemaphores;
      presentInfo.pResults = nullptr; // Optional
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = swapChains;
      presentInfo.pImageIndices = &imageIndex;

      result = vkQueuePresentKHR(presentQueue, &presentInfo);
      if (framebufferResized || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // window was resized
        framebufferResized = false;
        recreateSwapChain();
      } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
      }

      // advance to the next frame
      curFrame = (curFrame+1) % MAX_FRAMES_IN_FLIGHT;
    }

    void waitIdle() {
      vkDeviceWaitIdle(device);
    }

    // change the flag and wait for the current frame to be finished before resizing
    // static because GLFW does not know how to properly call a member function with the right this pointer
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
      auto kilauea = reinterpret_cast<Kilauea*>(glfwGetWindowUserPointer(window));
      kilauea->framebufferResized = true;
    }


  private:
    GLFWwindow* window;

    // vulkan
    VkInstance instance;                // connection between application and vulkan library
    VkDevice device;                    // logical device, used to interface with the GPU
    VkSurfaceKHR surface;               // surface to present images to
    VkCommandPool commandPool;          // command pool for submitting command buffers
    VkPhysicalDevice physicalDevice;    // GPU handle
    VkDebugUtilsMessengerEXT debugMsgr; // used to report validation layer errors

    // queues
    QueueFamilyIndices queueFamilies; // queue families indices in the physical device
    VkQueue graphicsQueue;            // handle to the graphics queue
    VkQueue presentQueue;             // handle to the presentation queue

    // graphics pipeline
    VkPipeline graphicsPipeline;      // handle to the graphics pipeline
    VkRenderPass renderPass;          // render pass, collection of attachments, subpasses, and dependencies
    VkPipelineLayout pipelineLayout;  // uniform values for shaders

    // swap chain
    VkSwapchainKHR swapChain;         // handle to the swap chain
    VkFormat swapChainImageFormat;    // format of the swap chain images
    VkExtent2D swapChainExtent;       // resolution of the swap chain images

    // per-swap-chain objects
    std::vector<VkImage> swapChainImages;             // handles to the swap chain images
    std::vector<VkImageView> swapChainImageViews;     // handles to the swap chain image views
    std::vector<VkFramebuffer> swapChainFramebuffers; // handles to the swap chain framebuffers

    // per-frame objects
    std::vector<VkCommandBuffer> commandBuffers;       // submit commands to the GPU
    std::vector<VkSemaphore> imageAvailableSemaphores; // signal that an image is available
    std::vector<VkSemaphore> renderFinishedSemaphores; // signal that rendering has finished
    std::vector<VkFence> inFlightFences;               // block the next frame until the current one is finished

    // uniform buffer
    VkDescriptorPool descriptorPool;                   // pool for submitting descriptor sets (uniform buffers)
    VkDescriptorSetLayout descriptorSetLayout;         // describe the layout of a descriptor set
    std::vector<VkDescriptorSet> descriptorSets;       // descriptor sets for the uniform buffers
    std::vector<VkBuffer> uniformBuffers;              // uniform buffers
    std::vector<VkDeviceMemory> uniformBuffersMemory;  // memory for the uniform buffers
    std::vector<void*> uniformBuffersMapped;           // pointer to the mapped uniform buffers

    // vertex buffer
    std::vector<Vertex> vertices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // index buffer
    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // depth buffer
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // texture
    VkImage textureImage;              // handle to the texture image
    VkDeviceMemory textureImageMemory; // memory for the texture image
    VkImageView textureImageView;      // handle to the texture image view
    VkSampler textureSampler;          // handle to the texture sampler

    // state
    uint32_t curFrame = 0;           // index of the current frame (used in buffers and semaphores)
    bool useDynamicStates = true;    // whether to use dynamic states in the pipeline (viewport, scissor)
    bool framebufferResized = false; // flag to recreate the swap chain after a resize



    void createSurface(VkInstance instance, VkSurfaceKHR& surface) {
      if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
      }
    }

    void createSyncObjects() {
      imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

      VkSemaphoreCreateInfo semaphoreInfo{};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceInfo{};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start in signaled state for the first frame

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create semaphores or fences!");
        }
      }
    }

    void cleanupSwapChain() {
      // depth buffer
      vkDestroyImageView(device, depthImageView, nullptr);
      vkDestroyImage(device, depthImage, nullptr);
      vkFreeMemory(device, depthImageMemory, nullptr);

      for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
      }
      for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
      }
      vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void recreateSwapChain() {
      // TODO: code below never happens, window size is always at least 1
      // pause the application until the window is shown again (minimized window)
      int width = 0, height = 0;
      glfwGetFramebufferSize(window, &width, &height);
      while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
      }

      // wait for the frame to be finished before recreating the swap chain
      vkDeviceWaitIdle(device);

      // recreate the swap chain
      cleanupSwapChain();
      createSwapChain(physicalDevice, device, surface, window, swapChain, swapChainImages, swapChainImageFormat, swapChainExtent);
      createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
      createDepthResources(device, physicalDevice, swapChainExtent, depthImage, depthImageMemory, depthImageView);
      createFramebuffers(device, swapChainExtent, renderPass, swapChainImageViews, depthImageView, swapChainFramebuffers);
    }


    void updateUniformBuffer() {
      // used to spin the scene
      static auto startTime = std::chrono::high_resolution_clock::now();
      auto currentTime = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

      // update the uniform buffer
      UniformBufferObject ubo{};
      ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                            glm::vec3(0.0f, 0.0f, 0.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));

      float aspect = swapChainExtent.width / (float) swapChainExtent.height;
      ubo.proj = glm::perspective(glm::radians(40.0f), aspect, 0.1f, 10.0f);

      // the Y coordinate of the clip coordinates is inverted in Vulkan (contrary to OpenGL)
      ubo.proj[1][1] *= -1;

      // copy the updated data to the mapped memory (visible to the GPU)
      memcpy(uniformBuffersMapped[curFrame], &ubo, sizeof(ubo));
    }


};

} // namespace vk
