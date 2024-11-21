#include "Camera.h"
#include "spdlog/spdlog.h"
#include <SDL.h>
#include <glm/matrix.hpp>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace cubik {
  constexpr float SPEED = 1.f * 8;

  Camera::Camera(glm::vec3 position) {
    Position = position;
    Forward = glm::vec3(0., 0., 1.);
    Up = glm::vec3(0., 1., 0.);
    Right = glm::vec3(1., 0, 0.);
  }

  // Torus
  Camera::Camera(const std::string& configName) {
    if (configName.starts_with("torus")) {
      Position = glm::vec3(-1.336102, -0.382239, -4.317312);
      Forward = glm::vec3(0.424631, 0.619743, 0.660006);
      Up = glm::vec3(-0.340109, 0.784792, -0.518099);
      Right = glm::vec3(0.839056, 0.004472, -0.544027);
    } else if (configName.starts_with("teapot")) {
      Position = glm::vec3(-12.688724, 13.976421, 16.294796);
      Forward = glm::vec3(0.900059, 0.435090, -0.024289);
      Up = glm::vec3(-0.434928, 0.900385, 0.012014);
      Right = glm::vec3(-0.027098, 0.000251, -0.999633);
    } else if (configName.starts_with("pieta")) {
      Position = glm::vec3(-12.550045, -11.942577, 61.809383);
      Forward = glm::vec3(0.569628, 0.623298, -0.535746);
      Up = glm::vec3(-0.454313, 0.781984, 0.426733);
      Right = glm::vec3(-0.684926, -0.000317, -0.728613);
    } else {
      spdlog::error("Unknown config for camera {}", configName);
      Position = glm::vec3(3., 0., -10.);
      Forward = glm::vec3(0., 0., 1.);
      Up = glm::vec3(0., 1., 0.);
      Right = glm::vec3(1., 0, 0.);
    }
  }

  void Camera::update(const uint8_t *inputs, cubik::MouseInput mouseInput, const std::chrono::duration<float> &deltaTime) {
    glm::vec3 moveInput { 0. };
//    if (inputs[SDL_SCANCODE_W]) moveInput += glm::vec3(0., 0., 1.);
//    if (inputs[SDL_SCANCODE_S]) moveInput += glm::vec3(0., 0., -1.);
//    if (inputs[SDL_SCANCODE_A]) moveInput += glm::vec3(-1., 0., 0.);
//    if (inputs[SDL_SCANCODE_D]) moveInput += glm::vec3(1., 0., 0.);
    if (inputs[SDL_SCANCODE_W]) moveInput += Forward;
    if (inputs[SDL_SCANCODE_S]) moveInput += -Forward;
    if (inputs[SDL_SCANCODE_A]) moveInput += -Right;
    if (inputs[SDL_SCANCODE_D]) moveInput += Right;

    glm::mat4 yawRotationMat(1);
    yawRotationMat = glm::rotate(yawRotationMat, mouseInput.yaw, Up);
    Forward = glm::normalize(glm::vec3(yawRotationMat * glm::vec4(Forward, 1.0)));
    Right = glm::normalize(glm::vec3(yawRotationMat * glm::vec4(Right, 1.0)));
    Up = glm::normalize(glm::vec3(yawRotationMat * glm::vec4(Up, 1.0)));

    glm::mat4 pitchRotationMat(1);
    pitchRotationMat = glm::rotate(pitchRotationMat, mouseInput.pitch, Right);
    Forward = glm::normalize(glm::vec3(pitchRotationMat * glm::vec4(Forward, 1.0)));
    Up = glm::normalize(glm::vec3(pitchRotationMat * glm::vec4(Up, 1.0)));
    Right = glm::normalize(glm::vec3(pitchRotationMat * glm::vec4(Right, 1.0)));

//    glm::mat4 yawRotationMat(1);
//    yawRotationMat = glm::rotate(yawRotationMat, mouseInput.yaw, Up);
//    Forward = glm::normalize(glm::vec3(yawRotationMat * glm::vec4(Forward, 1.0)));
//    Right = glm::normalize(glm::vec3(yawRotationMat * glm::vec4(Right, 1.0)));
//
//    glm::mat4 pitchRotationMat(1);
//    pitchRotationMat = glm::rotate(pitchRotationMat, mouseInput.pitch, Right);
//    Forward = glm::normalize(glm::vec3(pitchRotationMat * glm::vec4(Forward, 1.0)));
//
//    Up = glm::normalize(glm::cross(Right, Forward)); // Recalculate Up to avoid roll

    Position += SPEED * deltaTime.count() * moveInput;

    // spdlog::info("Camera Pos: {} Forward: {} Up: {} Right: {}", glm::to_string(Position), glm::to_string(Forward), glm::to_string(Up), glm::to_string(Right));
  }
}