#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "ProceduralLoader.h"
#include "VoxLoader.h"

constexpr int PROCEDURAL_WORLD_SIZE = 64;

int main(int argc, char *argv[]) {
  spdlog::info("Starting Cubik");

  int worldSize = PROCEDURAL_WORLD_SIZE;
  auto world = cubik::loadStaircase(worldSize);
  world = cubik::loadVoxFile("../models/torus_simple.vox", worldSize);

  auto window = cubik::Window(glm::ivec2(1700, 900), "Cubik");
  auto renderer = cubik::Renderer(window, world, worldSize);
  auto camera = cubik::Camera(glm::vec3(-10., 0., 3.));

  while (!window.IsClosed()) {
    window.processInputs();
    renderer.draw(camera);
  }

  spdlog::info("Cubik has successfully shut down!");
  return 0;
}