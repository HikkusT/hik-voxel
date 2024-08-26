#pragma once

#include <SDL.h>
#include <string>
#include <glm/vec2.hpp>
#include "SDL_vulkan.h"

namespace cubik {
  class Window {
  private:
    struct SDL_Window* _window { nullptr };
    bool _isClosed { false };
  public:
    Window(glm::ivec2 size, const std::string &name);
    ~Window();

    const glm::ivec2 Size;

    bool IsClosed() const { return _isClosed; }

    void processInputs();
    VkSurfaceKHR const createVulkanSurface(const VkInstance* instance) const;
  };
}
