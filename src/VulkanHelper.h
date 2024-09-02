#pragma once

#include <deque>
#include <functional>
#include "spdlog/spdlog.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vk_enum_string_helper.h"

// From https://vkguide.dev
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             spdlog::error("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

namespace vkutil {
  struct DeletionQueue
  {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
      deletors.push_back(function);
    }

    void flush() {
      // reverse iterate the deletion queue to execute all the functions
      for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
        (*it)(); //call functors
      }

      deletors.clear();
    }
  };


  VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
  VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
  VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
  VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);

  VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
  VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
  VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

  VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
  VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
  VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

  void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
  void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
}
