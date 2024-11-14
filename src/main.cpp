#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <chrono>
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "ProceduralLoader.h"
#include "VoxLoader.h"
#include "UncompressedGridWorld.h"
#include "SvoWorld.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

constexpr int PROCEDURAL_WORLD_SIZE = 4;

int main(int argc, char *argv[]) {
  spdlog::info("Starting Cubik");

  int worldSize = PROCEDURAL_WORLD_SIZE;
  auto rawWorld = cubik::loadStaircase(worldSize);
  rawWorld = cubik::loadVoxFile("../models/teapot_simple.vox", worldSize);
  auto camera = cubik::Camera(glm::vec3(3., 0., -10.));

  auto world = cubik::UncompressedGridWorld(rawWorld, worldSize);
  auto svoWorld = cubik::SvoWorld(rawWorld, worldSize);
//  svoWorld.print();
//
//  for (int x = 0; x < worldSize; x++) {
//    for (int y = 0; y < worldSize; y++) {
//      for (int z = 0 ; z < worldSize; z++) {
//        auto position = glm::ivec3(x, y, z);
//        int base = world.get(position);
//        int optimized = svoWorld.get(glm::ivec3(x, y, z));
//
//        if (base != optimized) {
//          spdlog::error("{}: base: {}   svo: {}", glm::to_string(position), base, optimized);
//        }
//      }
//    }
//  }

  const uint8_t* keyboardInput;
  auto window = cubik::Window(glm::ivec2(1700, 900), "Cubik", keyboardInput);
  auto renderer = cubik::Renderer(window, world);

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