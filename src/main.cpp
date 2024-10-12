#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <chrono>
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "ProceduralLoader.h"
#include "VoxLoader.h"

constexpr int PROCEDURAL_WORLD_SIZE = 10;

int main(int argc, char *argv[]) {
  spdlog::info("Starting Cubik");

  int worldSize = PROCEDURAL_WORLD_SIZE;
  auto world = cubik::loadStaircase(worldSize);
  world = cubik::loadVoxFile("../models/teapot_simple.vox", worldSize);
  auto camera = cubik::Camera(glm::vec3(3., 0., -10.));

  const uint8_t* keyboardInput;
  auto window = cubik::Window(glm::ivec2(1700, 900), "Cubik", keyboardInput);
  auto renderer = cubik::Renderer(window, world, worldSize);

  auto lastFrameTime = std::chrono::high_resolution_clock::now();
  while (!window.IsClosed()) {
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;

    cubik::MouseInput mouseInput = window.processInputs();
    camera.update(keyboardInput, mouseInput, deltaTime);
    renderer.draw(camera);
  }

  spdlog::info("Cubik has successfully shut down!");
  return 0;
}