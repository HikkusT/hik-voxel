#pragma once

#include <SDL.h>
#include <string>
#include <glm/vec2.hpp>
#include "SDL_vulkan.h"

namespace cubik {
  struct MouseInput {
    float yaw;
    float pitch;
  };

  class Window {
  private:
    struct SDL_Window* _window { nullptr };
    bool _isClosed { false };
  public:
    Window(glm::ivec2 size, const std::string &name, const uint8_t* &keyboardState);
    ~Window();

    const glm::ivec2 Size;

    bool IsClosed() const { return _isClosed; }

    MouseInput processInputs();
    VkSurfaceKHR const createVulkanSurface(const VkInstance* instance) const;
  };
}
