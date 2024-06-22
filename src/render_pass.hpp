#pragma once

#include "utils/common.hpp"

namespace vk {

void createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass& renderPass) {
  // a Color attachment is a framebuffer that contains color values (we have only one)
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling for now

  // what to do with the data in the attachment before rendering
  // load: preserve the existing contents
  // cear: clear the values to a constant at the start
  // dont_care: existing contents are undefined; we don't care about them
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

  // what to do with the data in the attachment after rendering
  // store: store the rendered contents to memory
  // dont_care: the contents of the framebuffer will be undefined after the rendering operation
  // non: the contents of the framebuffer will be preserved after the rendering operation
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // same as above, but for stencil data (we dont have any for now)
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  // textures and framebuffers in Vulkan are represented by VkImage objects
  // the layout of the pixels in memory will depend on how we're using the image
  // the initialLayout specifies the layout the image will have before the render pass begins
  // the finalLayout specifies the layout the image will have after the render pass finishes
  // the most common layouts are:
  // LAYOUT_UNDEFINED: we dont care about the previous layout of the image
  // COLOR_ATTACHMENT_OPTIMAL: the image data can be used as a color attachment
  // PRESENT_SRC_KHR: the image data is ready for presentation
  // TRANSFER_DST_OPTIMAL: the image data can be used as a destination for a memory copy operation
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // the index of the attachment in the attachment descriptions array
  // directly corresponds to the layout(location) in the shader
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0; // we only have one attachment
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // best layout for color attachments

  // subpasses are subsequent rendering operations that depend on the contents of the previous subpass
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // the subpass is a graphics subpass
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // added on chapter 'rendering and presentation'
  // add a subpass dependency to handle the transition of the image layout
  // from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // create render pass
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

} // namespace vk
