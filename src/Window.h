#pragma once

#include <string>
#include <glm/vec2.hpp>

namespace cubik {
  class Window {
  private:
    bool _isClosed = false;
  public:
    Window(glm::ivec2 size, const std::string &name);

    bool IsClosed() { return _isClosed; }

    void processInputs();
  };
}
