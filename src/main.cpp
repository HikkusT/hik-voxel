#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include "Window.h"
#include "Renderer.h"

int main(int argc, char *argv[]) {
  spdlog::info("Starting Cubik");

  auto window = cubik::Window(glm::ivec2(1700, 900), "Test hik");
  auto renderer = cubik::Renderer(window);

  while (!window.IsClosed()) {
    window.processInputs();
  }

  spdlog::info("Cubik has successfully shut down!");
  return 0;
}