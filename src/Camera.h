#pragma once

#include <glm/vec3.hpp>
#include <chrono>
#include "Window.h"

namespace cubik {
  class Camera {
  private:
  public:
    explicit Camera(glm::vec3 position);
    explicit Camera(const std::string& configName);

    glm::vec3 Position{};
    glm::vec3 Forward{};
    glm::vec3 Up{};
    glm::vec3 Right{};

    void update(const uint8_t* inputs, cubik::MouseInput mouseInput, const std::chrono::duration<float> &deltaTime);
  };
}
