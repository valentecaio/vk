#pragma once

#include "../utils/common.hpp"
#include "vertex.hpp"

namespace vk {


// more about alignas at https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html#_alignment_requirements
struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};


uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  throw std::runtime_error("failed to find suitable memory type!");
}


// create a temporary command buffer for a single transfer operation
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
  // allocate temporary command buffer for the transfer operation
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  // start recording the command buffer
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}


// submit and end the recording of a single time command buffer
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool,
                           VkQueue graphicsQueue, VkCommandBuffer commandBuffer) {
  // finish recording the command buffer
  vkEndCommandBuffer(commandBuffer);

  // submit the command buffer to the queue and wait for it to finish
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  // cleanup
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}



// create a buffer with a given memory property
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                  VkDeviceMemory& bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}


void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

  // create a temporary command buffer
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

  // copy the buffer
  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  // submit command and cleanup
  endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}


// create a vertex buffer in device local memory
void createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                        VkQueue graphicsQueue, const std::vector<Vertex>& vertices,
                        VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory) {

  // create a temporary buffer in a memory that is accessible by both CPU and GPU
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,     // buffer is used as the source in a memory transfer operation
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // memory is mappable by the host
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // memory is coherent, meaning that writes are visible to the device
               stagingBuffer, stagingBufferMemory);

  // map memory in the CPU to the staging buffer, copy vertices into it, unmap
  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  // create the final vertex buffer in device local memory (not accessible by CPU)
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // local device memory
               vertexBuffer, vertexBufferMemory);

  // copy the staging buffer to the vertex buffer
  copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);

  // cleanup of temporary buffer
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}


// create an index buffer in device local memory
void createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                       VkQueue graphicsQueue, const std::vector<uint32_t>& indices,
                       VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory) {

  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  // create a temporary buffer in a memory that is accessible by both CPU and GPU
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,     // buffer is used as the source in a memory transfer operation
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // memory is mappable by the host
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // memory is coherent, meaning that writes are visible to the device
               stagingBuffer, stagingBufferMemory);

  // map memory in the CPU to the staging buffer, copy indices into it, unmap
  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  // create the final index buffer in device local memory (not accessible by CPU)
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // local device memory
               indexBuffer, indexBufferMemory);

  // copy the staging buffer to the index buffer
  copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

  // cleanup of temporary buffer
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}


// create uniform buffers for each frame in CPU visible memory
void createUniformBuffers(VkDevice device, VkPhysicalDevice physicalDevice,
                          std::vector<VkBuffer>& uniformBuffers,
                          std::vector<VkDeviceMemory>& uniformBuffersMemory,
                          std::vector<void*>& uniformBuffersMapped) {

  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i], uniformBuffersMemory[i]);

    // persistent mapping: map the buffers once and keep them mapped during app lifetime
    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
  }
}

} // namespace vk
