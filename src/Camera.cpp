#include "Camera.h"
#include "spdlog/spdlog.h"
#include <SDL.h>
#include <glm/matrix.hpp>
#include <glm/ext.hpp>

namespace cubik {
  constexpr float SPEED = 1.f;

  Camera::Camera(glm::vec3 position) {
    Position = position;
    Forward = glm::vec3(0., 0., 1.);
    Up = glm::vec3(0., 1., 0.);
    Right = glm::vec3(1., 0, 0.);
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

    Position += SPEED * deltaTime.count() * moveInput;
  }
}