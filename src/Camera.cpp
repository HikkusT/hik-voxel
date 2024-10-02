#include "Camera.h"

namespace cubik {
  Camera::Camera(glm::vec3 position) {
    Position = position;
    Forward = glm::vec3(1.0, 0., 0.);
    Up = glm::vec3(0., 0., -1.);
    Right = glm::vec3(0., 1., 0.);
  }
}