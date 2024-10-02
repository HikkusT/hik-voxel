#pragma once

#include <glm/vec3.hpp>

namespace cubik {
  class Camera {
  private:
//    struct SDL_Window* _window { nullptr };
//    bool _isClosed { false };
  public:
    Camera(glm::vec3 position);

    glm::vec3 Position{};
    glm::vec3 Forward{};
    glm::vec3 Up{};
    glm::vec3 Right{};
//    ~Window();
//
//    const glm::ivec2 Size;
//
//    bool IsClosed() const { return _isClosed; }
//
//    void processInputs();
//    VkSurfaceKHR const createVulkanSurface(const VkInstance* instance) const;
  };
}
