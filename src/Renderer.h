#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "Window.h"

namespace cubik {
  class Renderer {
  private:
    const VkFormat DisplayFormat = VK_FORMAT_B8G8R8A8_UNORM;
    const Window DisplayWindow;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    void create_swapchain(glm::ivec2 size);

    void destroy_swapchain();
  public:
    explicit Renderer(const Window& window);
    ~Renderer();

    void cleanup();
  };
}
