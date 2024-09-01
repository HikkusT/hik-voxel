cmake_minimum_required(VERSION 3.22)
MESSAGE(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

project(hik-voxel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hik-voxel
        src/main.cpp
        src/Window.cpp
        src/Renderer.cpp
        src/VulkanHelper.cpp)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(hik-voxel
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

find_package(Vulkan REQUIRED)
target_link_libraries(hik-voxel PRIVATE Vulkan::Vulkan)

find_package(VulkanUtilityLibraries CONFIG REQUIRED)
target_link_libraries(hik-voxel PRIVATE Vulkan::LayerSettings Vulkan::UtilityHeaders Vulkan::CompilerConfiguration)

message(STATUS "Vulkan Include Directory: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARIES}")
if (Vulkan_FOUND)
    message(STATUS "Vulkan found version: ${Vulkan_VERSION}")
endif()

find_package(glm CONFIG REQUIRED)
target_link_libraries(hik-voxel PRIVATE glm::glm)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(hik-voxel PRIVATE spdlog::spdlog)

find_package(vk-bootstrap CONFIG REQUIRED)
target_link_libraries(hik-voxel PRIVATE vk-bootstrap::vk-bootstrap vk-bootstrap::vk-bootstrap-compiler-warnings)
