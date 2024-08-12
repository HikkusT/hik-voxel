//#include <fmt/core.h>
#include <SDL.h>
#include <spdlog/spdlog.h>
#include "Window.h"

namespace cubik {
    Window::Window(glm::ivec2 size, const std::string& name) {
      if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        abort();
      }

      SDL_Window* window = SDL_CreateWindow(
              name.c_str(),
              SDL_WINDOWPOS_UNDEFINED,
              SDL_WINDOWPOS_UNDEFINED,
              size.x,
              size.y,
              SDL_WINDOW_VULKAN);
      if (window == nullptr) {
        spdlog::error("Failed to create SDL window: {}", SDL_GetError());
        abort();
      }
    }

    void Window::processInputs() {
      SDL_Event e;
      while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT)
          _isClosed = true;

        // TODO: Handle minimize/maximize
      }
    }
}