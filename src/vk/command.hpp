#pragma once

#include "../utils/common.hpp"
#include "queue_family.hpp"
#include "vertex.hpp"

namespace vk {

void createCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                       QueueFamilyIndices queueFamilyIndices, VkCommandPool& commandPool) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  // two possible flags:
  // TRANSIENT_BIT: hint that command buffers are rerecorded with new commands
  //   very often (may change memory allocation strategy).
  // RESET_COMMAND_BUFFER_BIT: allow command buffers to be rerecorded individually,
  //   without this flag they all have to be reset together.
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void createCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT; // number of command buffers to allocate

  // PRIMARY: can be submitted to a queue for execution, but cannot be called from other command buffers
  // SECONDARY: cannot be submitted directly, but can be called from primary command buffers
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}


// record the command buffer for each frame
void recordCommandBuffer(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkExtent2D swapChainExtent,
                         std::vector<VkFramebuffer>& swapChainFramebuffers, uint32_t imageIndex,
                         VkPipeline graphicsPipeline, bool useDynamicStates, VkBuffer vertexBuffer,
                         VkBuffer indexBuffer, uint32_t indexBufferSize, VkPipelineLayout pipelineLayout,
                         VkDescriptorSet& descriptorSet) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pInheritanceInfo = nullptr; // only relevant for secondary command buffers

  // flags:
  // ONE_TIME_SUBMIT_BIT: the command buffer will be rerecorded right after executing it once
  // RENDER_PASS_CONTINUE_BIT: this is a secondary command buffer that will be entirely
  //   within a single render pass
  // SIMULTANEOUS_USE_BIT: the command buffer can be resubmitted while it is also already
  //   pending execution
  beginInfo.flags = 0; // optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }


  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  // clear color is gray
  // clear depth is white (1.0f)
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  // start the render pass
  // INLINE: the render pass commands will be embedded in the primary command
  // buffer itself and no secondary command buffers will be executed
  // SECONDARY_COMMAND_BUFFERS: the render pass commands will be executed
  // from secondary command buffers
  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // bind the pipeline - GRAPHICS or COMPUTE
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  // uncomment if viewport and scissor were declared as dynamic states on creation (pipeline.hpp),
  // so they can be changed without recreating the pipeline
  if (useDynamicStates) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  }

  // bind vertex buffers to the command buffer
  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  // bind index buffer (UINT16 or UINT32)
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  // bind the descriptor set
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  // draw !!
  vkCmdDrawIndexed(commandBuffer, indexBufferSize, 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

} // namespace vk
