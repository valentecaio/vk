
#pragma once

// #include "../utils/utils.hpp"
// #include "buffer.hpp"
// #include "command.hpp"
// #include "debug.hpp"
// #include "depth.hpp"
// #include "descriptor.hpp"
// #include "device.hpp"
// #include "framebuffer.hpp"
// #include "model.hpp"
// #include "physical_device.hpp"
// #include "pipeline.hpp"
// #include "queue_family.hpp"
// #include "render_pass.hpp"
// #include "swap_chain.hpp"
// #include "texture.hpp"
// #include "vertex.hpp"

#include "utils/common.hpp"
#include "vk/instance.hpp"

#include <base/VulkanInitializers.hpp>
#include <base/VulkanDevice.h>
#include <base/VulkanSwapChainGLFW.hpp>
#include <base/camera.hpp>
#include <base/VulkanglTFModel.h>

namespace vk {

class ShadowMapping {
  public:
    // constructors
    ShadowMapping() = default;
    ShadowMapping(GLFWwindow* window) : window(window) {};

    uint32_t gpu_id = 0; // change gpu here
    uint32_t width = WIDTH;
    uint32_t height = HEIGHT;
    bool displayShadowMap = false;

  	std::vector<vkglTF::Model> scene;

    bool prepared = false;
	  uint32_t currentBuffer = 0;

    // Keep depth range as small as possible
    // for better shadow map precision
    float zNear = 1.0f;
    float zFar = 96.0f;

    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

    glm::vec3 lightPos = glm::vec3();
    float lightFOV = 45.0f;

    VkClearColorValue defaultClearColor = { { 0.01f, 0.01f, 0.01f, 1.0f } };

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;

    Camera camera;




    // pitoco
    GLFWwindow* window;
    VkInstance instance;                // connection between application and vulkan library
    VkDevice device;                    // logical device, used to interface with the GPU
    VkPhysicalDevice physicalDevice;    // GPU handle
    VkDebugUtilsMessengerEXT debugMsgr; // used to report validation layer errors
    VkRenderPass renderPass;            // render pass, collection of attachments, subpasses, and dependencies
    VkDescriptorPool descriptorPool;                   // pool for submitting descriptor sets (uniform buffers)

  	// vkglTF::Model scene;
    vks::VulkanDevice *vulkanDevice; // wrapper
  	VkPhysicalDeviceFeatures enabledFeatures{};
	  std::vector<const char*> enabledDeviceExtensions;
	  void* deviceCreatepNextChain = nullptr;
  	VkQueue queue{ VK_NULL_HANDLE };
	  VkFormat depthFormat;
  	VulkanSwapChainGLFW swapChain; // wrapper
    struct {
      VkSemaphore presentComplete; // Swap chain image presentation
      VkSemaphore renderComplete;  // Command buffer submission and execution
    } semaphores;
  	VkSubmitInfo submitInfo;
  	VkCommandPool cmdPool{ VK_NULL_HANDLE };
  	std::vector<VkCommandBuffer> drawCmdBuffers;
    std::vector<VkFence> waitFences;
    struct {
      VkImage image;
      VkDeviceMemory memory;
      VkImageView view;
    } depthStencil{}; // Default depth stencil attachment used by the default render pass
    VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
    std::vector<VkFramebuffer>frameBuffers;

    // shadow mapping
    // Framebuffer for offscreen rendering
    struct FrameBufferAttachment {
      VkImage image;
      VkDeviceMemory mem;
      VkImageView view;
    };
    struct OffscreenPass {
      int32_t width, height;
      VkFramebuffer frameBuffer;
      FrameBufferAttachment depth;
      VkRenderPass renderPass;
      VkSampler depthSampler;
      VkDescriptorImageInfo descriptor;
    } offscreenPass{};
	  uint32_t shadowMapize{ 2048 };
	  VkFormat offscreenDepthFormat{ VK_FORMAT_D16_UNORM };

    struct {
      vks::Buffer scene;
      vks::Buffer offscreen;
    } uniformBuffers;

    struct UniformDataOffscreen {
      glm::mat4 depthMVP;
    } uniformDataOffscreen;

    struct UniformDataScene {
      glm::mat4 projection;
      glm::mat4 view;
      glm::mat4 model;
      glm::mat4 depthBiasMVP;
      glm::vec4 lightPos;
      // Used for depth map visualization
      float zNear;
      float zFar;
    } uniformDataScene;

    struct {
      VkPipeline offscreen{ VK_NULL_HANDLE };
      VkPipeline sceneShadow{ VK_NULL_HANDLE };
      VkPipeline debug{ VK_NULL_HANDLE };
    } pipelines;
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

    struct {
      VkDescriptorSet offscreen{ VK_NULL_HANDLE };
      VkDescriptorSet scene{ VK_NULL_HANDLE };
      VkDescriptorSet debug{ VK_NULL_HANDLE };
    } descriptorSets;
    VkDescriptorSetLayout descriptorSetLayout; // describe the layout of a descriptor set

    std::vector<VkShaderModule> shaderModules;
	  VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;


    void init() {
      camera.type = Camera::CameraType::lookat;
      camera.setPosition(glm::vec3(0.0f, 0.0f, -14.5f));
      camera.setRotation(glm::vec3(-25.0f, -390.0f, 0.0f));
      camera.setPerspective(70.0f, (float)width / (float)height, 1.0f, 256.0f);
      timerSpeed *= 0.5f;


      // instance
      createInstance(instance);


      // optional debug messenger
      setupDebugMessenger(instance, debugMsgr);


      // Physical device
      uint32_t gpuCount = 0;
      VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
      std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
      VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));
      physicalDevice = physicalDevices[gpu_id];


      // Vulkan device wrapper
      vulkanDevice = new vks::VulkanDevice(physicalDevice);


      // Logical device
      VK_CHECK_RESULT(vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain));
    	device = vulkanDevice->logicalDevice;


      // Graphics queue
    	vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);


      // Depth format
		  vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);


      // Swap chain
    	swapChain.setContext(instance, physicalDevice, device);


      // Semaphores (they stay the same during application time)
      VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
      VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.presentComplete));
      VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphores.renderComplete));

      submitInfo = vks::initializers::submitInfo();
      submitInfo.pWaitDstStageMask = &submitPipelineStages;
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = &semaphores.presentComplete;
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = &semaphores.renderComplete;


      // Surface
      swapChain.initSurface(window);


      // Command pool
      VkCommandPoolCreateInfo cmdPoolInfo = {};
      cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
      cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));


      // Setup Swap Chain
    	swapChain.create(&width, &height);


      createCommandBuffers();


      createFences();


      setupDepthStencil();


      setupRenderPass();


      // Pipeline cache
      VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vks::initializers::pipelineCacheCreateInfo();
      VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));


      setupFrameBuffers();


      // load scene
      const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
  		scene.resize(1);
      scene[0].loadFromFile("models/samplescene.gltf", vulkanDevice, queue, glTFLoadingFlags);


  		prepareOffscreenFramebuffer();


      // Uniform buffers
      // Offscreen vertex shader uniform buffer block
      VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers.offscreen, sizeof(UniformDataOffscreen)));
      // Scene vertex shader uniform buffer block
      VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers.scene, sizeof(UniformDataScene)));
      // Map persistent
      VK_CHECK_RESULT(uniformBuffers.offscreen.map());
      VK_CHECK_RESULT(uniformBuffers.scene.map());
      updateLight();
      updateUniformBufferOffscreen();
      updateUniformBuffers();


      setupDescriptors();


      preparePipelines();


  		buildCommandBuffers();


      prepared = true;
    }



    void createCommandBuffers() {
      // Command buffers (one command buffer for each swap chain image)
      drawCmdBuffers.resize(swapChain.imageCount);
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
    }

    void createFences() {
      // Wait fences (one for each command buffer)
      VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
      waitFences.resize(drawCmdBuffers.size());
      for (auto& fence : waitFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
      }
    }

    void setupDepthStencil() {
      VkImageCreateInfo imageCI{};
      imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageCI.imageType = VK_IMAGE_TYPE_2D;
      imageCI.format = depthFormat;
      imageCI.extent = { width, height, 1 };
      imageCI.mipLevels = 1;
      imageCI.arrayLayers = 1;
      imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

      VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));
      VkMemoryRequirements memReqs{};
      vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

      VkMemoryAllocateInfo memAllloc{};
      memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAllloc.allocationSize = memReqs.size;
      memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.memory));
      VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

      VkImageViewCreateInfo imageViewCI{};
      imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageViewCI.image = depthStencil.image;
      imageViewCI.format = depthFormat;
      imageViewCI.subresourceRange.baseMipLevel = 0;
      imageViewCI.subresourceRange.levelCount = 1;
      imageViewCI.subresourceRange.baseArrayLayer = 0;
      imageViewCI.subresourceRange.layerCount = 1;
      imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
      if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
      VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
    }

    void setupRenderPass() {
      std::array<VkAttachmentDescription, 2> attachments = {};
      // Color attachment
      attachments[0].format = swapChain.colorFormat;
      attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      // Depth attachment
      attachments[1].format = depthFormat;
      attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference colorReference = {};
      colorReference.attachment = 0;
      colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthReference = {};
      depthReference.attachment = 1;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpassDescription = {};
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.colorAttachmentCount = 1;
      subpassDescription.pColorAttachments = &colorReference;
      subpassDescription.pDepthStencilAttachment = &depthReference;
      subpassDescription.inputAttachmentCount = 0;
      subpassDescription.pInputAttachments = nullptr;
      subpassDescription.preserveAttachmentCount = 0;
      subpassDescription.pPreserveAttachments = nullptr;
      subpassDescription.pResolveAttachments = nullptr;

      // Subpass dependencies for layout transitions
      std::array<VkSubpassDependency, 2> dependencies;

      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
      dependencies[0].dependencyFlags = 0;

      dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].dstSubpass = 0;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].srcAccessMask = 0;
      dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
      dependencies[1].dependencyFlags = 0;

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpassDescription;
      renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
      renderPassInfo.pDependencies = dependencies.data();

      VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    }

    void setupFrameBuffers() {
      VkImageView attachments[2];

      // Depth/Stencil attachment is the same for all frame buffers
      attachments[1] = depthStencil.view;

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferCreateInfo.pNext = NULL;
      frameBufferCreateInfo.renderPass = renderPass;
      frameBufferCreateInfo.attachmentCount = 2;
      frameBufferCreateInfo.pAttachments = attachments;
      frameBufferCreateInfo.width = width;
      frameBufferCreateInfo.height = height;
      frameBufferCreateInfo.layers = 1;

      // Create frame buffers for every swap chain image
      frameBuffers.resize(swapChain.imageCount);
      for (uint32_t i = 0; i < frameBuffers.size(); i++)
      {
        attachments[0] = swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
      }
    }


    // Set up a separate render pass for the offscreen frame buffer
    // This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
    void prepareOffscreenRenderpass() {
      VkAttachmentDescription attachmentDescription{};
      attachmentDescription.format = offscreenDepthFormat;
      attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
      attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
      attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
      attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

      VkAttachmentReference depthReference = {};
      depthReference.attachment = 0;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 0;													// No color attachments
      subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

      // Use subpass dependencies for layout transitions
      std::array<VkSubpassDependency, 2> dependencies;

      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      VkRenderPassCreateInfo renderPassCreateInfo = vks::initializers::renderPassCreateInfo();
      renderPassCreateInfo.attachmentCount = 1;
      renderPassCreateInfo.pAttachments = &attachmentDescription;
      renderPassCreateInfo.subpassCount = 1;
      renderPassCreateInfo.pSubpasses = &subpass;
      renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
      renderPassCreateInfo.pDependencies = dependencies.data();

      VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &offscreenPass.renderPass));
    }


    // Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
    // The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
    void prepareOffscreenFramebuffer() {
      offscreenPass.width = shadowMapize;
      offscreenPass.height = shadowMapize;

      // For shadow mapping we only need a depth attachment
      VkImageCreateInfo image = vks::initializers::imageCreateInfo();
      image.imageType = VK_IMAGE_TYPE_2D;
      image.extent.width = offscreenPass.width;
      image.extent.height = offscreenPass.height;
      image.extent.depth = 1;
      image.mipLevels = 1;
      image.arrayLayers = 1;
      image.samples = VK_SAMPLE_COUNT_1_BIT;
      image.tiling = VK_IMAGE_TILING_OPTIMAL;
      image.format = offscreenDepthFormat;																// Depth stencil attachment
      image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
      VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.depth.image));

      VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.mem));
      VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.mem, 0));

      VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
      depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      depthStencilView.format = offscreenDepthFormat;
      depthStencilView.subresourceRange = {};
      depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      depthStencilView.subresourceRange.baseMipLevel = 0;
      depthStencilView.subresourceRange.levelCount = 1;
      depthStencilView.subresourceRange.baseArrayLayer = 0;
      depthStencilView.subresourceRange.layerCount = 1;
      depthStencilView.image = offscreenPass.depth.image;
      VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view));

      // Create sampler to sample from to depth attachment
      // Used to sample in the fragment shader for shadowed rendering
      VkFilter shadowmap_filter = vks::tools::formatIsFilterable(physicalDevice, offscreenDepthFormat, VK_IMAGE_TILING_OPTIMAL) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
      VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
      sampler.magFilter = shadowmap_filter;
      sampler.minFilter = shadowmap_filter;
      sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      sampler.addressModeV = sampler.addressModeU;
      sampler.addressModeW = sampler.addressModeU;
      sampler.mipLodBias = 0.0f;
      sampler.maxAnisotropy = 1.0f;
      sampler.minLod = 0.0f;
      sampler.maxLod = 1.0f;
      sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
      VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &offscreenPass.depthSampler));

      prepareOffscreenRenderpass();

      // Create frame buffer
      VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
      fbufCreateInfo.renderPass = offscreenPass.renderPass;
      fbufCreateInfo.attachmentCount = 1;
      fbufCreateInfo.pAttachments = &offscreenPass.depth.view;
      fbufCreateInfo.width = offscreenPass.width;
      fbufCreateInfo.height = offscreenPass.height;
      fbufCreateInfo.layers = 1;

      VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer));
    }



    void setupDescriptors() {
      // Pool
      std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
      };
      VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
      VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

      // Layout
      std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        // Binding 1 : Fragment shader image sampler (shadow map)
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
      };
      VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
      VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

      // Sets
      std::vector<VkWriteDescriptorSet> writeDescriptorSets;

      // Image descriptor for the shadow map attachment
      VkDescriptorImageInfo shadowMapDescriptor =
        vks::initializers::descriptorImageInfo(
          offscreenPass.depthSampler,
          offscreenPass.depth.view,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

      // Debug display
      VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.debug));
      writeDescriptorSets = {
        // Binding 0 : Parameters uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
        // Binding 1 : Fragment shader texture sampler
        vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

      // Offscreen shadow map generation
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen));
      writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.offscreen, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.offscreen.descriptor),
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

      // Scene rendering with shadow map applied
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));
      writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
        // Binding 1 : Fragment shader shadow sampler
        vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    void preparePipelines() {
      // Layout
      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
      VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

      // Pipelines
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
      VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
      VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
      VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
      VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
      VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
      VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
      std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
      VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

      VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
      pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
      pipelineCI.pRasterizationState = &rasterizationStateCI;
      pipelineCI.pColorBlendState = &colorBlendStateCI;
      pipelineCI.pMultisampleState = &multisampleStateCI;
      pipelineCI.pViewportState = &viewportStateCI;
      pipelineCI.pDepthStencilState = &depthStencilStateCI;
      pipelineCI.pDynamicState = &dynamicStateCI;
      pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
      pipelineCI.pStages = shaderStages.data();

      // Shadow mapping debug quad display
      rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
      shaderStages[0] = loadShader("build/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
      shaderStages[1] = loadShader("build/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
      // Empty vertex input state
      VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
      pipelineCI.pVertexInputState = &emptyInputState;
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.debug));

      // Scene rendering with shadows applied
      pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal});
      rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
      shaderStages[0] = loadShader("build/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
      shaderStages[1] = loadShader("build/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
      // Use specialization constants to select between horizontal and vertical blur
      uint32_t enablePCF = 0;
      VkSpecializationMapEntry specializationMapEntry = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
      VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(1, &specializationMapEntry, sizeof(uint32_t), &enablePCF);
      shaderStages[1].pSpecializationInfo = &specializationInfo;
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.sceneShadow));

      // Offscreen pipeline (vertex shader only)
      shaderStages[0] = loadShader("build/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
      pipelineCI.stageCount = 1;
      // No blend attachment states (no color attachments used)
      colorBlendStateCI.attachmentCount = 0;
      // Disable culling, so all faces contribute to shadows
      rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
      depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      // Enable depth bias
      rasterizationStateCI.depthBiasEnable = VK_TRUE;
      // Add depth bias to dynamic state, so we can change it at runtime
      dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
      dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

      pipelineCI.renderPass = offscreenPass.renderPass;
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));
    }

    void buildCommandBuffers() {
      VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

      VkClearValue clearValues[2];
      VkViewport viewport;
      VkRect2D scissor;

      for (size_t i = 0; i < drawCmdBuffers.size(); ++i)
      {
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

        /*
          First render pass: Generate shadow map by rendering the scene from light's POV
        */
        {
          clearValues[0].depthStencil = { 1.0f, 0 };

          VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
          renderPassBeginInfo.renderPass = offscreenPass.renderPass;
          renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
          renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
          renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
          renderPassBeginInfo.clearValueCount = 1;
          renderPassBeginInfo.pClearValues = clearValues;

          vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

          viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
          vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

          scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
          vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

          // Set depth bias (aka "Polygon offset")
          // Required to avoid shadow mapping artifacts
          vkCmdSetDepthBias(
            drawCmdBuffers[i],
            depthBiasConstant,
            0.0f,
            depthBiasSlope);

          vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
          vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreen, 0, nullptr);
          scene[0].draw(drawCmdBuffers[i]);

          vkCmdEndRenderPass(drawCmdBuffers[i]);
        }

        /*
          Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
        */

        /*
          Second pass: Scene rendering with applied shadow map
        */

        {
          clearValues[0].color = defaultClearColor;
          clearValues[1].depthStencil = { 1.0f, 0 };

          VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
          renderPassBeginInfo.renderPass = renderPass;
          renderPassBeginInfo.framebuffer = frameBuffers[i];
          renderPassBeginInfo.renderArea.extent.width = width;
          renderPassBeginInfo.renderArea.extent.height = height;
          renderPassBeginInfo.clearValueCount = 2;
          renderPassBeginInfo.pClearValues = clearValues;

          vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

          viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
          vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

          scissor = vks::initializers::rect2D(width, height, 0, 0);
          vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

          // Visualize shadow map
          if (displayShadowMap) {
            vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.debug, 0, nullptr);
            vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
            vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
          } else {
            // Render the shadows scene
            vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, nullptr);
            vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sceneShadow);
            scene[0].draw(drawCmdBuffers[i]);
          }

          vkCmdEndRenderPass(drawCmdBuffers[i]);
        }

        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
      }
    }

    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage) {
      VkPipelineShaderStageCreateInfo shaderStage = {};
      shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStage.stage = stage;
      shaderStage.module = vks::tools::loadShader(fileName.c_str(), device);
      shaderStage.pName = "main";
      assert(shaderStage.module != VK_NULL_HANDLE);
      shaderModules.push_back(shaderStage.module);
      return shaderStage;
    }














    void updateLight() {
      // Animate the light source
      lightPos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
      lightPos.y = -50.0f + sin(glm::radians(timer * 360.0f)) * 20.0f;
      lightPos.z = 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f;
    }

    void updateUniformBuffers() {
      uniformDataScene.projection = camera.matrices.perspective;
      uniformDataScene.view = camera.matrices.view;
      uniformDataScene.model = glm::mat4(1.0f);
      uniformDataScene.lightPos = glm::vec4(lightPos, 1.0f);
      uniformDataScene.depthBiasMVP = uniformDataOffscreen.depthMVP;
      uniformDataScene.zNear = zNear;
      uniformDataScene.zFar = zFar;
      memcpy(uniformBuffers.scene.mapped, &uniformDataScene, sizeof(uniformDataScene));
    }

    void updateUniformBufferOffscreen() {
      // Matrix from light's point of view
      glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
      glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
      glm::mat4 depthModelMatrix = glm::mat4(1.0f);
      uniformDataOffscreen.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
      memcpy(uniformBuffers.offscreen.mapped, &uniformDataOffscreen, sizeof(uniformDataOffscreen));
    }


    void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto tStart = std::chrono::high_resolution_clock::now();
        render();
        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        auto frameTimer = tDiff / 1000.0f;
        timer += timerSpeed * frameTimer;
        if (timer > 1.0) {
          timer -= 1.0f;
        }
        camera.update(frameTimer);
      }
    }

    void render() {
      if (!prepared)
        return;

      updateLight();
      updateUniformBufferOffscreen();
      updateUniformBuffers();

      // draw frame
      prepareFrame();
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
      VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
      submitFrame();
    }


    void prepareFrame()
    {
      VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
      // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
      // SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until submitFrame() in case number of swapchain images will change on resize
      switch (result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
          windowResize();
          return;
        case VK_SUBOPTIMAL_KHR:
          return;
        default:
          VK_CHECK_RESULT(result);
      }
    }

    void submitFrame() {
      VkResult result = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
      // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
      switch (result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
          windowResize();
          break;
        default:
          VK_CHECK_RESULT(result);
      }
      VK_CHECK_RESULT(vkQueueWaitIdle(queue));
    }


    void windowResize() {
      if (!prepared) {
        return;
      }
      prepared = false;

      // Ensure all operations on the device have been finished before destroying resources
      vkDeviceWaitIdle(device);

      // Recreate swap chain
      int w = 0, h = 0;
      glfwGetFramebufferSize(window, &w, &h);
      while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
      }
      width =  (uint32_t)w;
      height = (uint32_t)h;

      swapChain.create(&width, &height);

      // Recreate the frame buffers
      vkDestroyImageView(device, depthStencil.view, nullptr);
      vkDestroyImage(device, depthStencil.image, nullptr);
      vkFreeMemory(device, depthStencil.memory, nullptr);
      setupDepthStencil();
      for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
      }
      setupFrameBuffers();

      // Command buffers need to be recreated as they may store
      // references to the recreated frame buffer
      vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
      createCommandBuffers();
      buildCommandBuffers();

      // SRS - Recreate fences in case number of swapchain images has changed on resize
      for (auto& fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
      }
      createFences();

      vkDeviceWaitIdle(device);

      if ((width > 0.0f) && (height > 0.0f)) {
        camera.updateAspectRatio((float)width / (float)height);
      }

      prepared = true;
    }

};

} // namespace vk
