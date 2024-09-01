#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "Window.h"

namespace cubik {
  struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;
  };

  constexpr unsigned int FRAME_OVERLAP = 2;


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

    int _frameNumber {0};
    FrameData _frames[FRAME_OVERLAP];
    FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    void create_swapchain(glm::ivec2 size);
    void init_commands();
    void init_sync_structures();

    void destroy_swapchain();
  public:
    explicit Renderer(const Window& window);
    ~Renderer();

    void draw();
    void cleanup();
  };
}
