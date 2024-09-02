#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "Window.h"
#include "VulkanHelper.h"
#include "Descriptor.h"
#include "vk_mem_alloc.h"

namespace cubik {
  struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
  };


  struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;

    vkutil::DeletionQueue _deletionQueue;
  };

  constexpr unsigned int FRAME_OVERLAP = 2;


  class Renderer {
  private:
    const VkFormat DisplayFormat = VK_FORMAT_B8G8R8A8_UNORM;
    const Window DisplayWindow;

    VmaAllocator _allocator;
    vkutil::DeletionQueue _mainDeletionQueue = {}; // const? readonly?
    AllocatedImage _drawImage;
    VkExtent2D _drawExtent;

    vkutil::DescriptorAllocator globalDescriptorAllocator;
    VkDescriptorSet _drawImageDescriptors;
    VkDescriptorSetLayout _drawImageDescriptorLayout;

    VkPipeline _gradientPipeline;
    VkPipelineLayout _gradientPipelineLayout;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;

    int _frameNumber {0};
    FrameData _frames[FRAME_OVERLAP];
    FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    void create_swapchain(glm::ivec2 size);
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_pipelines();
    void init_background_pipelines();

    void draw_background(VkCommandBuffer cmd);

    void destroy_swapchain();
  public:
    explicit Renderer(const Window& window);
    ~Renderer();

    void draw();
    void cleanup();
  };
}
