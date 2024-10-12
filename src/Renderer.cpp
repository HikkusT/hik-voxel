#include "Renderer.h"
#include "VkBootstrap.h"
#include "spdlog/spdlog.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "Pipeline.h"

namespace cubik {
  Renderer::Renderer(const Window& window, const std::vector<int>& world, int worldSize)
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
    init_world(world, worldSize);
    init_commands();
    init_sync_structures();
    init_descriptors(worldSize);
    init_pipelines();
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

  void Renderer::init_world(const std::vector<int>& world, int size) {
    size_t bufferSize = sizeof(int) + sizeof(VOXEL_SIZE) + sizeof(int) * world.size();

    VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = bufferSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VK_CHECK(vkCreateBuffer(_device, &bufferInfo, nullptr, &_worldBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _worldBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = vkutil::findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _chosenGPU)
    };

    VK_CHECK(vkAllocateMemory(_device, &allocInfo, nullptr, &_worldBufferMemory));

    VK_CHECK(vkBindBufferMemory(_device, _worldBuffer, _worldBufferMemory, 0));

    void* data;
    VK_CHECK(vkMapMemory(_device, _worldBufferMemory, 0, bufferSize, 0, &data));
    memcpy(data, &size, sizeof(int));
    memcpy(static_cast<char*>(data) + sizeof(int), &VOXEL_SIZE, sizeof(VOXEL_SIZE));
    memcpy(static_cast<char*>(data) + sizeof(int) + sizeof(VOXEL_SIZE), world.data(), sizeof(int) * world.size());

    vkUnmapMemory(_device, _worldBufferMemory);
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

  void Renderer::init_descriptors(int worldSize) {
    std::vector<vkutil::DescriptorAllocator::PoolSizeRatio> sizes = {
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    globalDescriptorAllocator.init_pool(_device, 10, sizes);

    _drawImageDescriptorLayout =
      vkutil::DescriptorLayoutBuilder {}
      .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
      .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
      .build(_device, VK_SHADER_STAGE_COMPUTE_BIT);

    _drawImageDescriptors = globalDescriptorAllocator.allocate(_device,_drawImageDescriptorLayout);

    // Image binding
    VkDescriptorImageInfo imgInfo{
      .imageView = _drawImage.imageView,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    VkWriteDescriptorSet drawImageWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = _drawImageDescriptors,
      .dstBinding = 0,
      .descriptorCount = 1, // TODO: What is this???
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .pImageInfo = &imgInfo
    };
    vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

    // World binding
    VkDescriptorBufferInfo bufferInfo = {
      .buffer = _worldBuffer,
      .offset = 0,
      .range =  sizeof(int) + sizeof(VOXEL_SIZE) + sizeof(int) * worldSize * worldSize * worldSize // FIXME
    };
    VkWriteDescriptorSet descriptorWrite = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = _drawImageDescriptors,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);

    _mainDeletionQueue.push_function([&]() {
      globalDescriptorAllocator.destroy_pool(_device);
      vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
    });
  }

  void Renderer::init_pipelines() {
    init_background_pipelines();
  }

  void Renderer::init_background_pipelines() {
    VkPushConstantRange pushConstant {
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0,
      .size = sizeof(CameraPushConstants)
    };

    VkPipelineLayoutCreateInfo computeLayout {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .setLayoutCount = 1,
      .pSetLayouts = &_drawImageDescriptorLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstant,
    };
    VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

    VkShaderModule computeDrawShader;
//    if (!vkutil::load_shader_module("../shaders/sphere.comp.spv", _device, &computeDrawShader))
    if (!vkutil::load_shader_module("../shaders/naiveRayMarcher.comp.spv", _device, &computeDrawShader))
    {
      fmt::print("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stageInfo {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = computeDrawShader,
      .pName = "main"
    };
    VkComputePipelineCreateInfo computePipelineCreateInfo {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .stage = stageInfo,
      .layout = _gradientPipelineLayout
    };

    VK_CHECK(vkCreateComputePipelines(_device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &_gradientPipeline));

    vkDestroyShaderModule(_device, computeDrawShader, nullptr);
    _mainDeletionQueue.push_function([&]() {
      vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
      vkDestroyPipeline(_device, _gradientPipeline, nullptr);
    });
  }


  // Main
  void Renderer::draw(const Camera& camera) {
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
    get_current_frame()._deletionQueue.flush();
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));

    CameraPushConstants pc {
      .position = camera.Position,
      .forward = camera.Forward,
      .up = camera.Up
    };

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = vkutil::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    _drawExtent.width = _drawImage.imageExtent.width;
    _drawExtent.height = _drawImage.imageExtent.height;

    vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
//    draw_background(cmd);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);
    vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CameraPushConstants), &pc);
    vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
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