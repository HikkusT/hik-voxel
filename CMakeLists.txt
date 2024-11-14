cmake_minimum_required(VERSION 3.22)
MESSAGE(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

project(hik-voxel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hik-voxel
        src/main.cpp
        src/Window.cpp
        src/Renderer.cpp
        src/VulkanHelper.cpp
        src/Descriptor.cpp
        src/Pipeline.cpp
        src/Camera.cpp
        src/VoxLoader.cpp
        src/ProceduralLoader.cpp
        src/UncompressedGridWorld.cpp
        src/UncompressedGridWorld.h)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(hik-voxel
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(hik-voxel PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)

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


# TODO: Review shader compilation...
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/shaders/*.comp"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER: ${GLSL}")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -gVS -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)
