
#include <base/VulkanInitializers.hpp>
#include <base/VulkanDevice.h>
#include <base/VulkanSwapChainGLFW.hpp>
#include <base/camera.hpp>
#include <base/VulkanglTFModel.h>

#include "utils/common.hpp"
#include "vk/instance.hpp"
#include "vk/physical_device.hpp"
#include "vk/command.hpp"

class ShadowMapping {
  public:
    /************************ constructors ************************/

    ShadowMapping(GLFWwindow* window, Camera camera) : window(window), camera(camera) {};


    /************************ public settings ************************/

    bool displayShadowMap = false;   // display the shadow map (debug)
    uint32_t shadowMapize = 1024;    // size of the shadow map buffer
    uint32_t gpu_id = 0;             // change gpu here
    float zNear = 1.0f;              // near plane for the shadow map
    float zFar = 96.0f;              // far plane for the shadow map
    float lightFOV = 45.0f;          // field of view for the light source
    uint32_t width = 800;            // surface window width
    uint32_t height = 600;           // surface window height
    float timerSpeed = 0.20f;        // multiplier to control the speed of animations
    VkClearColorValue bgColor = {0.01f, 0.01f, 0.21f, 1.0f}; // background color

    // depth bias used to avoid shadowing artifacts
    float depthBiasConstant = 1.25f; // constant factor (always applied)
    float depthBiasSlope = 1.75f;    // slope factor (applied depending on polygon's slope)



    /************************ private state ************************/
  private:
    GLFWwindow* window;                 // window handle
    Camera camera;                      // camera handle
    vkglTF::Model scene;                // model scene handle
    std::vector<vkglTF::Model> scenes;
    bool paused = false;                // flag to pause animations (movement still allowed)
    bool prepared = false;              // flag to indicate if the scene is ready to be rendered
    uint32_t currentBuffer = 0;         // index of the current swap chain buffer
    glm::vec3 lightPos = glm::vec3();   // light position
    float timer = 0.0f;                 // frame rate independent timer, clamped from [0, 1]

    // constants
    struct {
      std::string sceneVert = "build/scene.vert.spv";
      std::string sceneFrag = "build/scene.frag.spv";
      std::string debugVert = "build/debug.vert.spv";
      std::string debugFrag = "build/debug.frag.spv";
      std::string offscVert = "build/offscreen.vert.spv";
      std::string model = "models/samplescene.gltf";
    } paths;

    // input (WASD or right click to translate, left click to rotate)
    struct Input {
      struct {
        bool buttons[8] = {false};  // mouse buttons state
        float x, y;                 // last mouse position
      } mouse;
      bool keys[1024] = {false};    // keyboard keys state
    } input;


    /************************ vulkan objects ************************/

    VkInstance instance;                // connection between application and vulkan library
    VkDebugUtilsMessengerEXT debugMsgr; // used to report validation layer errors
    vks::VulkanDevice *vulkanDevice;    // wrapper for vulkan device (logical and physical)
    VkDevice device;                    // pointer to vulkanDevice->logicalDevice
    VkPhysicalDevice physicalDevice;    // pointer to vulkanDevice->physicalDevice
    VkCommandPool commandPool;          // pool for submitting command buffers
    VkQueue queue;                      // graphics queue
    VulkanSwapChainGLFW swapChain;      // wrapper for swap chain
    VkSemaphore semaphPresentComplete;  // semaphore for swap chain image presentation
    VkSemaphore semaphRenderComplete;   // semaphore for command buffer submission and execution

    std::vector<VkCommandBuffer> drawCmdBuffers; // command buffers (one per swap chain image)
    std::vector<VkFence> waitFences;             // wait fences     (one per swap chain image)
    std::vector<VkShaderModule> shaderModules;   // shader modules  (one per shader)

    // information about a queue submit operation
    struct {
      VkSubmitInfo info;
      VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } submit;

    // framebuffer attachment used in render passes
    struct FrameBufferAttachment {
      VkImage image;
      VkDeviceMemory mem;
      VkImageView view;
    };

    // render pass of main scene
    struct ScenePass {
      std::vector<VkFramebuffer> frameBuffers;    // frame buffers for the scene rendering (one per swap chain image)
      FrameBufferAttachment color, depth;         // color and depth attachments
      VkFormat depthFormat;
      vks::Buffer uniformBuffer;                  // uniform buffer for the scene rendering
      VkRenderPass renderPass;
    } scenePass{};

    // offscreen pass for shadow map rendering
    struct OffscreenPass {
      int32_t width, height;                      // fixed size equal to shadowMapize
      VkFramebuffer frameBuffer;                  // only one because we render to the whole image
      FrameBufferAttachment depth;                // depth attachment (shadow map)
      VkFormat depthFormat = VK_FORMAT_D16_UNORM; // 16 bits is enough for the shadow map
      VkSampler depthSampler;                     // we use this sampler in the fragment shader of the scene
      vks::Buffer uniformBuffer;                  // uniform buffer for the shadow map rendering
      VkRenderPass renderPass;
    } offscreenPass{};

    // uniform buffer data for the offscreen shadow map rendering (offscreen.vert)
    struct UniformDataOffscreen {
      glm::mat4 depthMVP;
    } uniformDataOffscreen;

    // uniform buffer data for the scene rendering or shadow map visualization (scene.frag & debug.frag)
    struct UniformDataScene {
      // variables for scene rendering (scene.frag)
      glm::mat4 projection;    // projection matrix
      glm::mat4 view;          // view matrix
      glm::mat4 model;         // model matrix
      glm::mat4 lightSpace;    // MVP matrix from light's point of view
      glm::vec4 lightPos;      // light position in view space

      // variables for shadow map visualization (debug.frag)
      float zNear;             // near plane for the shadow map
      float zFar;              //  far plane for the shadow map
    } uniformDataScene;

    // pipelines for each render
    struct Pipelines {
      VkPipeline offscreen;    // pipeline for the offscreen rendering (create the shadow map)
      VkPipeline sceneShadow;  // pipeline for the scene rendering (uses the shadow map)
      VkPipeline debug;        // pipeline for the shadow map visualization (debug)
      VkPipelineLayout layout; // common uniform layout for all pipelines
      VkPipelineCache cache;   // common cache for the pipelines
    } pipelines;

    // descriptor sets for each render
    struct DescriptorSets {
      VkDescriptorSet offscreen;    // descriptor set for the offscreen rendering
      VkDescriptorSet scene;        // descriptor set for the scene rendering
      VkDescriptorSet debug;        // descriptor set for the shadow map visualization
      VkDescriptorSetLayout layout; // common layout for all descriptor sets
      VkDescriptorPool pool;        // common pool for submitting descriptor sets (uniform buffers)
    } descriptorSets;




    /************************ public methods ************************/
  public:
    void init() {
      // initial vulkan setup (instance, device, graphics queue, model, semaphores, fences, surface, swap chain)
      vk::createInstance(instance);
      vk::setupDebugMessenger(instance, debugMsgr);
      vk::getPhysicalDevice(instance, gpu_id, physicalDevice);
      createDevice(); // init vulkanDevice and device
      vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue); // Graphics queue
      loadModel();
      createSemaphores();
      createFences();
      createSwapChain(); // init swap chain and surface

      // offscreen pass setup
      setupOffscreenRenderPassAndFramebuffer();

      // scene pass setup
      setupSceneDepthStencil();
      setupSceneRenderPass();
      setupSceneFrameBuffers();

      // common setup to both passes
      setupUniformBuffers();
      setupDescriptorSets();
      setupPipelines();
      vk::createCommandPool(device, swapChain.queueNodeIndex, commandPool);
      setupCommandBuffers();

      prepared = true;
    }

    void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto tStart = std::chrono::high_resolution_clock::now();

        updateScene();
        render();

        auto tEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<double, std::milli>(tEnd - tStart).count() / 1000.0f;
        camera.update(frameDuration);

        // update timer for next frame animation
        if (!paused) {
          timer += timerSpeed * frameDuration;
          if (timer > 1.0)
            timer -= 1.0f;
        }
      }
    }

    void cleanup() {

    }

    /************************ static input callbacks ************************/

    // WASD translation
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
      ShadowMapping* me = static_cast<ShadowMapping*>(glfwGetWindowUserPointer(window));
      auto keys = me->input.keys;
      if (action == GLFW_PRESS) {
        keys[key] = true;
      } else if (action == GLFW_RELEASE) {
        keys[key] = false;
        if (key == GLFW_KEY_SPACE) {
          me->paused = !me->paused;
        }
      }
      me->camera.keys.up = keys[GLFW_KEY_W];
      me->camera.keys.down = keys[GLFW_KEY_S];
      me->camera.keys.left = keys[GLFW_KEY_A];
      me->camera.keys.right = keys[GLFW_KEY_D];
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
      ShadowMapping* me = static_cast<ShadowMapping*>(glfwGetWindowUserPointer(window));
      me->input.mouse.buttons[button] = (action == GLFW_PRESS);
    }

    // left click to rotate, right click to translate
    static void cursorPositionCallback(GLFWwindow* window, double x, double y) {
      ShadowMapping* me = static_cast<ShadowMapping*>(glfwGetWindowUserPointer(window));
      auto mouse = &me->input.mouse;
      float dx = static_cast<float>(x - mouse->x);
      float dy = static_cast<float>(y - mouse->y);
      if (mouse->buttons[GLFW_MOUSE_BUTTON_LEFT]) {
        me->camera.rotate(glm::vec3(0.15f * dy, 0.15f * dx, 0.0f));
      } if (mouse->buttons[GLFW_MOUSE_BUTTON_RIGHT]) {
        // vertical
        if (dy != 0) me->camera.translate(glm::vec3(0.0f, 0.05f * dy, 0.0f));

        // horizontal (dx>0 move left, dx<0 move right)
        me->camera.keys.left = true;
        me->camera.update(dx / 100.0f);
        me->camera.keys = {false}; // reset keys
      }
      mouse->x = static_cast<float>(x);
      mouse->y = static_cast<float>(y);
    }




    /************************ private methods ************************/
  private:

    // Vulkan device wrapper and logical device
    void createDevice() {
      VkPhysicalDeviceFeatures enabledFeatures{};
      std::vector<const char*> enabledDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
      vulkanDevice = new vks::VulkanDevice(physicalDevice);
      VK_CHECK_RESULT(vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, nullptr));
      device = vulkanDevice->logicalDevice;
    }

    void loadModel() {
      const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
      scenes.resize(1);
      scenes[0].loadFromFile(paths.model, vulkanDevice, queue, glTFLoadingFlags);
      scene = scenes[0];
    }

    // Swap chain and surface
    void createSwapChain() {
      swapChain.setContext(instance, physicalDevice, device);
      swapChain.initSurface(window);
      swapChain.create(&width, &height);
    }

    // Semaphores (they stay the same during application time)
    void createSemaphores() {
      VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
      VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphPresentComplete));
      VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphRenderComplete));
      submit.info = vks::initializers::submitInfo();
      submit.info.pWaitDstStageMask = &submit.stageMask;
      submit.info.waitSemaphoreCount = 1;
      submit.info.pWaitSemaphores = &semaphPresentComplete;
      submit.info.signalSemaphoreCount = 1;
      submit.info.pSignalSemaphores = &semaphRenderComplete;
      submit.info.commandBufferCount = 1;
    }

    // TODO: unused
    // Wait fences (one for each command buffer)
    void createFences() {
      VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
      waitFences.resize(drawCmdBuffers.size());
      for (auto& fence : waitFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
      }
    }

    void setupSceneDepthStencil() {
      // init depth format
      if (!scenePass.depthFormat) {
        vks::tools::getSupportedDepthFormat(physicalDevice, &scenePass.depthFormat);
      }

      VkImageCreateInfo imageCI{};
      imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageCI.imageType = VK_IMAGE_TYPE_2D;
      imageCI.format = scenePass.depthFormat;
      imageCI.extent = { width, height, 1 };
      imageCI.mipLevels = 1;
      imageCI.arrayLayers = 1;
      imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

      VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &scenePass.depth.image));
      VkMemoryRequirements memReqs{};
      vkGetImageMemoryRequirements(device, scenePass.depth.image, &memReqs);

      VkMemoryAllocateInfo memAllloc{};
      memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAllloc.allocationSize = memReqs.size;
      memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &scenePass.depth.mem));
      VK_CHECK_RESULT(vkBindImageMemory(device, scenePass.depth.image, scenePass.depth.mem, 0));

      VkImageViewCreateInfo imageViewCI{};
      imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
      imageViewCI.image = scenePass.depth.image;
      imageViewCI.format = scenePass.depthFormat;
      imageViewCI.subresourceRange.baseMipLevel = 0;
      imageViewCI.subresourceRange.levelCount = 1;
      imageViewCI.subresourceRange.baseArrayLayer = 0;
      imageViewCI.subresourceRange.layerCount = 1;
      imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
      if (scenePass.depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
      }
      VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &scenePass.depth.view));
    }

    void setupSceneRenderPass() {
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
      attachments[1].format = scenePass.depthFormat;
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

      VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &scenePass.renderPass));
    }

    void setupSceneFrameBuffers() {
      VkImageView attachments[2];

      // Depth/Stencil attachment is the same for all frame buffers
      attachments[1] = scenePass.depth.view;

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferCreateInfo.pNext = NULL;
      frameBufferCreateInfo.renderPass = scenePass.renderPass;
      frameBufferCreateInfo.attachmentCount = 2;
      frameBufferCreateInfo.pAttachments = attachments;
      frameBufferCreateInfo.width = width;
      frameBufferCreateInfo.height = height;
      frameBufferCreateInfo.layers = 1;

      // Create frame buffers for every swap chain image
      scenePass.frameBuffers.resize(swapChain.imageCount);
      for (uint32_t i = 0; i < scenePass.frameBuffers.size(); i++) {
        attachments[0] = swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &scenePass.frameBuffers[i]));
      }
    }

    // Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
    // The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
    void setupOffscreenRenderPassAndFramebuffer() {
      offscreenPass.width = offscreenPass.height = shadowMapize;

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
      image.format = offscreenPass.depthFormat;                                // Depth stencil attachment
      image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;    // We will sample directly from the depth attachment for the shadow mapping
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
      depthStencilView.format = offscreenPass.depthFormat;
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
      VkFilter shadowmap_filter = vks::tools::formatIsFilterable(physicalDevice, offscreenPass.depthFormat, VK_IMAGE_TILING_OPTIMAL) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
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

      // Create render pass
      VkAttachmentDescription attachmentDescription{};
      attachmentDescription.format = offscreenPass.depthFormat;
      attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
      attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // Clear depth at beginning of the render pass
      attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // We will read from depth, so it's important to store the depth attachment results
      attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // We don't care about initial layout of the attachment
      attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

      VkAttachmentReference depthReference = {};
      depthReference.attachment = 0;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;      // Attachment will be used as depth/stencil during render pass

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 0;                          // No color attachments
      subpass.pDepthStencilAttachment = &depthReference;         // Reference to our depth attachment

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

    void setupUniformBuffers() {
      // uniform buffer block for offscreen vertex shader
      VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &offscreenPass.uniformBuffer, sizeof(UniformDataOffscreen)));
      // uniform buffer block for scene vertex shader
      VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scenePass.uniformBuffer, sizeof(UniformDataScene)));
      // map the memory and update
      VK_CHECK_RESULT(offscreenPass.uniformBuffer.map());
      VK_CHECK_RESULT(scenePass.uniformBuffer.map());
      updateScene();
    }

    void setupDescriptorSets() {
      // Pool
      std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
      };
      VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
      VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorSets.pool));

      // Common layout
      std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        // Binding 1 : Fragment shader image sampler (shadow map)
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
      };
      VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
      VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSets.layout));

      // Sets
      std::vector<VkWriteDescriptorSet> writeDescriptorSets;

      // Image descriptor for the shadow map attachment
      VkDescriptorImageInfo shadowMapDescriptor =
        vks::initializers::descriptorImageInfo(
          offscreenPass.depthSampler,
          offscreenPass.depth.view,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

      // Debug display
      VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorSets.pool, &descriptorSets.layout, 1);
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.debug));
      writeDescriptorSets = {
        // Binding 0 : Parameters uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scenePass.uniformBuffer.descriptor),
        // Binding 1 : Fragment shader texture sampler
        vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

      // Offscreen shadow map generation
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen));
      writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.offscreen, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &offscreenPass.uniformBuffer.descriptor),
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

      // Scene rendering with shadow map applied
      VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));
      writeDescriptorSets = {
        // Binding 0 : Vertex shader uniform buffer
        vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &scenePass.uniformBuffer.descriptor),
        // Binding 1 : Fragment shader shadow sampler
        vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
      };
      vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    void setupPipelines() {
      // Pipeline cache
      VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vks::initializers::pipelineCacheCreateInfo();
      VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelines.cache));

      // Layout
      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSets.layout, 1);
      VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelines.layout));

      // Pipelines
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
      VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
      VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
      VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
      VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
      VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
      VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
      std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
      VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

      VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelines.layout, scenePass.renderPass, 0);
      pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
      pipelineCI.pRasterizationState = &rasterizationStateCI;
      pipelineCI.pColorBlendState = &colorBlendStateCI;
      pipelineCI.pMultisampleState = &multisampleStateCI;
      pipelineCI.pViewportState = &viewportStateCI;
      pipelineCI.pDepthStencilState = &depthStencilStateCI;
      pipelineCI.pDynamicState = &dynamicStateCI;
      pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
      pipelineCI.pStages = shaderStages.data();

      // Shadow mapping visualization (debug)
      rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
      shaderStages[0] = loadShader(paths.debugVert, VK_SHADER_STAGE_VERTEX_BIT);
      shaderStages[1] = loadShader(paths.debugFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
      VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
      pipelineCI.pVertexInputState = &emptyInputState; // no vertex input
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelines.cache, 1, &pipelineCI, nullptr, &pipelines.debug));

      // Scene rendering with shadows applied
      rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
      shaderStages[0] = loadShader(paths.sceneVert, VK_SHADER_STAGE_VERTEX_BIT);
      shaderStages[1] = loadShader(paths.sceneFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
      pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal});
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelines.cache, 1, &pipelineCI, nullptr, &pipelines.sceneShadow));

      // Offscreen pipeline (vertex shader only)
      shaderStages[0] = loadShader(paths.offscVert, VK_SHADER_STAGE_VERTEX_BIT);
      pipelineCI.stageCount = 1;
      pipelineCI.renderPass = offscreenPass.renderPass;
      colorBlendStateCI.attachmentCount = 0;                      // no color attachments used
      rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;          // disable culling, all faces contribute to shadows
      rasterizationStateCI.depthBiasEnable = VK_TRUE;             // enable depth bias
      dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS); // enable changing depth bias at runtime
      dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
      depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelines.cache, 1, &pipelineCI, nullptr, &pipelines.offscreen));
    }

    void setupCommandBuffers() {
      // create command buffers (one command buffer for each swap chain image)
      drawCmdBuffers.resize(swapChain.imageCount);
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(
            commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));

      // setup command buffers
      VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
      for (size_t i = 0; i < drawCmdBuffers.size(); ++i) {
        VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
        {
          // First pass: Generate shadow map by rendering the scene from light's POV
          {
            VkClearValue clearValues[1];
            clearValues[0].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
            renderPassBeginInfo.renderPass = offscreenPass.renderPass;
            renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
            renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
            renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
              VkViewport viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
              vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

              VkRect2D scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
              vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

              // Set depth bias (aka "Polygon offset") to avoid shadow mapping artifacts
              vkCmdSetDepthBias(drawCmdBuffers[i], depthBiasConstant, 0.0f, depthBiasSlope);

              vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
              vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.layout, 0, 1, &descriptorSets.offscreen, 0, nullptr);
              scene.draw(drawCmdBuffers[i]);
            }
            vkCmdEndRenderPass(drawCmdBuffers[i]);
          } // end of first pass

          // Second pass: Scene rendering with applied shadow map
          {
            VkClearValue clearValues[2];
            clearValues[0].color = bgColor;
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
            renderPassBeginInfo.renderPass = scenePass.renderPass;
            renderPassBeginInfo.framebuffer = scenePass.frameBuffers[i];
            renderPassBeginInfo.renderArea.extent.width = width;
            renderPassBeginInfo.renderArea.extent.height = height;
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
              VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
              vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

              VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
              vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

              // Visualize shadow map
              if (displayShadowMap) {
                vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.layout, 0, 1, &descriptorSets.debug, 0, nullptr);
                vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
                vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
              } else {
                // Render the shadows scene
                vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.layout, 0, 1, &descriptorSets.scene, 0, nullptr);
                vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.sceneShadow);
                scene.draw(drawCmdBuffers[i]);
              }
            }
          } // end of second pass
          vkCmdEndRenderPass(drawCmdBuffers[i]);
        } // end of command buffer
        VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
      }
    }

    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage) {
      auto module = vks::tools::loadShader(fileName.c_str(), device);
      assert(module != VK_NULL_HANDLE);
      shaderModules.push_back(module);
      return vks::initializers::pipelineShaderStageCreateInfo(stage, module);
    }

    // update position of objects in the scene
    void updateScene() {
      // animate the light source
      if (!paused) {
        lightPos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
        lightPos.y = -50.0f + sin(glm::radians(timer * 360.0f)) * 20.0f;
        lightPos.z = 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f;
      }

      // scene uniform buffer
      uniformDataScene.projection = camera.matrices.perspective;
      uniformDataScene.view = camera.matrices.view;
      uniformDataScene.model = glm::mat4(1.0f);
      uniformDataScene.lightPos = glm::vec4(lightPos, 1.0f);
      uniformDataScene.lightSpace = uniformDataOffscreen.depthMVP;
      uniformDataScene.zNear = zNear;
      uniformDataScene.zFar = zFar;
      memcpy(scenePass.uniformBuffer.mapped, &uniformDataScene, sizeof(uniformDataScene));

      // offscren uniform buffer
      // Matrix from light's point of view
      glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
      glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
      glm::mat4 depthModelMatrix = glm::mat4(1.0f);
      uniformDataOffscreen.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
      memcpy(offscreenPass.uniformBuffer.mapped, &uniformDataOffscreen, sizeof(uniformDataOffscreen));
    }

    // render frame
    void render() {
      if (!prepared)
        return;

      // prepare frame
      VkResult result = swapChain.acquireNextImage(semaphPresentComplete, &currentBuffer);
      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
        recreateSwapChain();
      } else if (result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
      }

      // submit frame to queue
      submit.info.pCommandBuffers = &drawCmdBuffers[currentBuffer];
      VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit.info, VK_NULL_HANDLE));

      // present frame and wait until the queue is idle
      result = swapChain.queuePresent(queue, currentBuffer, semaphRenderComplete);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
      } else {
        VK_CHECK_RESULT(result);
      }
      VK_CHECK_RESULT(vkQueueWaitIdle(queue));
    }

    // called by render() on windows resize
    void recreateSwapChain() {
      if (!prepared) {
        return;
      }
      prepared = false;

      // ensure all operations on the device have been finished before destroying resources
      vkDeviceWaitIdle(device);

      // update surface dimensions
      int w = 0, h = 0;
      glfwGetFramebufferSize(window, &w, &h);
      while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
      }
      width =  (uint32_t)w;
      height = (uint32_t)h;

      // recreate swap chain
      swapChain.create(&width, &height);

      // recreate frame buffers attachments
      vkDestroyImageView(device, scenePass.depth.view, nullptr);
      vkDestroyImage(device, scenePass.depth.image, nullptr);
      vkFreeMemory(device, scenePass.depth.mem, nullptr);
      setupSceneDepthStencil();

      // recreate frame buffers
      for (uint32_t i = 0; i < scenePass.frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, scenePass.frameBuffers[i], nullptr);
      }
      setupSceneFrameBuffers();

      // recreate command buffers (they store references to the old frame buffers)
      vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
      setupCommandBuffers();

      // recreate fences (number of swapchain images may have changed on resize)
      for (auto& fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
      }
      createFences();

      vkDeviceWaitIdle(device);

      // update camera aspect ratio
      if ((width > 0.0f) && (height > 0.0f)) {
        camera.updateAspectRatio((float)width / (float)height);
      }

      prepared = true;
    }

};



int main() {
  uint32_t w = 800, h = 600;

  // init glfw
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable openGL context
  auto window = glfwCreateWindow(w, h, "Shadow Mapping", nullptr, nullptr);

  // init camera
  auto camera = Camera();
  camera.type = Camera::CameraType::firstperson;
  camera.setMovementSpeed(5.0f);
  camera.setPosition(glm::vec3(-0.6f, 9.5f, -14.0f));
  camera.setRotation(glm::vec3(-30.0f, 0.0f, 0.0f));
  camera.setPerspective(70.0f, (float)w / (float)h, 1.0f, 256.0f);

  // init vulkan object
  ShadowMapping shadowMapping = ShadowMapping(window, camera);
  shadowMapping.gpu_id = 0;
  shadowMapping.width = w;
  shadowMapping.height = h;
  shadowMapping.init();

  // input callbacks
  glfwSetWindowUserPointer(window, &shadowMapping);
  glfwSetKeyCallback(window, shadowMapping.keyCallback);
  glfwSetMouseButtonCallback(window, shadowMapping.mouseButtonCallback);
  glfwSetCursorPosCallback(window, shadowMapping.cursorPositionCallback);

  // main loop
  shadowMapping.mainLoop();

  // cleanup
  shadowMapping.cleanup();
  glfwDestroyWindow(window);
  glfwTerminate();
}
