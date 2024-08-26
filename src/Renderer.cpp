#include "Renderer.h"
#include "VkBootstrap.h"
#include "spdlog/spdlog.h"

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
    _device = vkb::DeviceBuilder{ vkbPhysicalDevice }.build().value().device; // Not much to go wrong here, afaik

    create_swapchain(DisplayWindow.Size);
  }

  void Renderer::create_swapchain(glm::ivec2 size)
  {
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR{ .format = DisplayFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(size.x, size.y)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
  }

  // Cleanup
  void Renderer::destroy_swapchain()
  {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    for (int i = 0; i < _swapchainImageViews.size(); i++) {

      vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
  }

  Renderer::~Renderer()
  {
    destroy_swapchain();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
  }
}