#include <SDL.h>
#include <spdlog/spdlog.h>
#include "Window.h"

namespace cubik {
  Window::Window(glm::ivec2 size, const std::string &name, const Uint8* &keyboardState)
  : Size(size) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
      abort();
    }

    _window = SDL_CreateWindow(
        name.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        size.x,
        size.y,
        SDL_WINDOW_VULKAN);
    if (_window == nullptr) {
      spdlog::error("Failed to create SDL window: {}", SDL_GetError());
      SDL_Quit();
      abort();
    }

    keyboardState = SDL_GetKeyboardState(nullptr);
  }

  MouseInput Window::processInputs() {
    MouseInput mouseInput {};
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT)
        _isClosed = true;

      if (e.type == SDL_MOUSEMOTION) {
        mouseInput.yaw += (float)e.motion.xrel / 200.f;
        mouseInput.pitch -= (float)e.motion.yrel / 200.f;
      }

      // TODO: Handle minimize/maximize
    }

    return mouseInput;
  }

  VkSurfaceKHR const Window::createVulkanSurface(const VkInstance *instance) const {
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(_window, *instance, &surface) == SDL_FALSE) {
      spdlog::error("Failed to create vulkan surface: {}", SDL_GetError());
      SDL_Quit();
      abort();
    }

    return surface;
  }

  Window::~Window() {
    if (_window) SDL_DestroyWindow(_window);
    SDL_Quit();
  }
}