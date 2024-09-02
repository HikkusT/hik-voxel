#include "VulkanHelper.h"

VkCommandPoolCreateInfo vkutil::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.pNext = nullptr;

  info.flags = flags;
  info.queueFamilyIndex = queueFamilyIndex;
  return info;
}

VkCommandBufferAllocateInfo vkutil::command_buffer_allocate_info(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;

  info.commandPool = pool;
  info.commandBufferCount = count;
  info.level = level;
  return info;
}

VkCommandBufferBeginInfo vkutil::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
  return {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = flags,
      .pInheritanceInfo = nullptr
  };
}

VkCommandBufferSubmitInfo vkutil::command_buffer_submit_info(VkCommandBuffer cmd) {
  return {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .pNext = nullptr,
      .commandBuffer = cmd,
      .deviceMask = 0
  };
}

VkFenceCreateInfo vkutil::fence_create_info(VkFenceCreateFlags flags) {
  return {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = flags
  };
}

VkSemaphoreCreateInfo vkutil::semaphore_create_info(VkSemaphoreCreateFlags flags) {
  return {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = nullptr,
    .flags = flags
  };
}

VkSemaphoreSubmitInfo vkutil::semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
  return {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .pNext = nullptr,
    .semaphore = semaphore,
    .value = 1,
    .stageMask = stageMask,
    .deviceIndex = 0
  };
}

VkSubmitInfo2 vkutil::submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
  VkSubmitInfo2 info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  info.pNext = nullptr;

  info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
  info.pWaitSemaphoreInfos = waitSemaphoreInfo;

  info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
  info.pSignalSemaphoreInfos = signalSemaphoreInfo;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = cmd;

  return info;
}

VkImageSubresourceRange vkutil::image_subresource_range(VkImageAspectFlags aspectMask) {
  return {
    .aspectMask = aspectMask,
    .baseMipLevel = 0,
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .baseArrayLayer = 0,
    .layerCount = VK_REMAINING_ARRAY_LAYERS
  };
}

VkImageCreateInfo vkutil::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
  return {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = extent,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = usageFlags
  };
}

VkImageViewCreateInfo vkutil::imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
  return {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .subresourceRange = {
      .aspectMask = aspectFlags,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
      },
  };
}


void vkutil::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
  VkImageMemoryBarrier2 imageBarrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .pNext = nullptr
  };

  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  imageBarrier.oldLayout = currentLayout;
  imageBarrier.newLayout = newLayout;

  VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange = vkutil::image_subresource_range(aspectMask);
  imageBarrier.image = image;

  VkDependencyInfo depInfo {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .pNext = nullptr,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &imageBarrier
  };

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vkutil::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {
  VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

  blitRegion.srcOffsets[1].x = srcSize.width;
  blitRegion.srcOffsets[1].y = srcSize.height;
  blitRegion.srcOffsets[1].z = 1;

  blitRegion.dstOffsets[1].x = dstSize.width;
  blitRegion.dstOffsets[1].y = dstSize.height;
  blitRegion.dstOffsets[1].z = 1;

  blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount = 1;
  blitRegion.srcSubresource.mipLevel = 0;

  blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.dstSubresource.layerCount = 1;
  blitRegion.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
  blitInfo.dstImage = destination;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage = source;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter = VK_FILTER_LINEAR;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &blitRegion;

  vkCmdBlitImage2(cmd, &blitInfo);
}

