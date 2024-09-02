#include "Renderer.h"
#include "VkBootstrap.h"
#include "spdlog/spdlog.h"
#include "vulkan/vk_enum_string_helper.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// From https://vkguide.dev
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
             spdlog::error("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

namespace cubik {
  Renderer::Renderer(const Window& window)
  : DisplayWindow(window) {
    vkb::InstanceBuilder vulkanBuilder;

    auto vulkanInstanceResult = vulkanBuilder.set_app_name("Example Vulkan Application")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    if (!vulkanInstanceResult) {
      spdlog::error("Failed to create vulkan instance: {}", vulkanInstanceResult.error().message());
      std::abort();
    }
    auto vkb = *vulkanInstanceResult;

    _instance = vkb.instance;
    _debug_messenger = vkb.debug_messenger;

    _surface = DisplayWindow.createVulkanSurface(&_instance);

    // GPU selecting logic
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;


    vkb::PhysicalDeviceSelector selector{ vkb };
    vkb::Result<vkb::PhysicalDevice> physicalDeviceResult = selector
      .set_minimum_version(1, 3)
      .set_required_features_13(features)
      .set_required_features_12(features12)
      .set_surface(_surface)
      .select();

    if (!physicalDeviceResult) {
      spdlog::error("Could not find a fitting GPU: {}", physicalDeviceResult.error().message());
      std::abort();
    }
    vkb::PhysicalDevice vkbPhysicalDevice = *physicalDeviceResult;

    _chosenGPU = vkbPhysicalDevice.physical_device;
    vkb::Device vkbDevice = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value();
    _device = vkbDevice.device; // Not much to go wrong here, afaik

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo = {
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = _chosenGPU,
      .device = _device,
      .instance = _instance
    };
    vmaCreateAllocator(&allocatorInfo, &_allocator);
    _mainDeletionQueue.push_function([&]() {
      vmaDestroyAllocator(_allocator);
    });

    create_swapchain(DisplayWindow.Size);
    init_commands();
    init_sync_structures();
  }

  void Renderer::create_swapchain(glm::ivec2 size) {
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR{ .format = DisplayFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(size.x, size.y)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapchainExtent = vkbSwapchain.extent;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();


    VkExtent3D drawImageExtent = {
      static_cast<uint32_t>(size.x), // TODO: Review
      static_cast<uint32_t>(size.y),
      1
    };

    //hardcoding the draw format to 32 bit float
    _drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    _drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rawImgInfo = vkutil::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);
    VmaAllocationCreateInfo rawImgAllocInfo = {
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
      .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    vmaCreateImage(_allocator, &rawImgInfo, &rawImgAllocInfo, &_drawImage.image, &_drawImage.allocation, nullptr);
    VkImageViewCreateInfo rawViewIndo = vkutil::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &rawViewIndo, nullptr, &_drawImage.imageView));

    // TODO: Review this syntax
    _mainDeletionQueue.push_function([=]() {
      vkDestroyImageView(_device, _drawImage.imageView, nullptr);
      vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
    });
  }

  void Renderer::init_commands() {
    VkCommandPoolCreateInfo commandPoolInfo = vkutil::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (auto & frame : _frames) {
      VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &frame._commandPool));

      VkCommandBufferAllocateInfo cmdAllocInfo = vkutil::command_buffer_allocate_info(frame._commandPool, 1);

      VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &frame._mainCommandBuffer));
    }
  }

  void Renderer::init_sync_structures() {
    VkFenceCreateInfo fenceCreateInfo = vkutil::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkutil::semaphore_create_info();

    for (auto & frame : _frames) {
      VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame._renderFence));

      VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame._swapchainSemaphore));
      VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame._renderSemaphore));
    }
  }


  // Main
  void Renderer::draw() {
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
    get_current_frame()._deletionQueue.flush();
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));


    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = vkutil::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    draw_background(cmd);
    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);
    vkutil::transition_image(cmd, _swapchainImages[swapchainImageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdSubmitInfo = vkutil::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkutil::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkutil::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit = vkutil::submit_info(&cmdSubmitInfo, &signalInfo, &waitInfo);


    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &get_current_frame()._renderSemaphore,
      .swapchainCount = 1,
      .pSwapchains = &_swapchain,
      .pImageIndices = &swapchainImageIndex
    };

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber++;
  }

  void Renderer::draw_background(VkCommandBuffer cmd) {
    float flash = std::abs(std::sin(_frameNumber / 120.f));
    VkClearColorValue clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clearRange = vkutil::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
  }



  // Cleanup
  void Renderer::destroy_swapchain()
  {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    for (int i = 0; i < _swapchainImageViews.size(); i++) {

      vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
  }

  Renderer::~Renderer() {
    vkDeviceWaitIdle(_device);

    for (auto & frame : _frames) {
      vkDestroyCommandPool(_device, frame._commandPool, nullptr);

      vkDestroyFence(_device, frame._renderFence, nullptr);
      vkDestroySemaphore(_device, frame._renderSemaphore, nullptr);
      vkDestroySemaphore(_device, frame._swapchainSemaphore, nullptr);

      frame._deletionQueue.flush();
    }

    _mainDeletionQueue.flush();

    destroy_swapchain();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
  }
}