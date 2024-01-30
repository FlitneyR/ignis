cmake_minimum_required(VERSION 3.12)
project(Ignis)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v1.3.275
)
FetchContent_Declare(
    fetch_VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        v3.0.1
)

FetchContent_MakeAvailable(vk_bootstrap fetch_VulkanMemoryAllocator)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(engine)

target_link_libraries(engine PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap glfw VulkanMemoryAllocator)

target_include_directories(engine include)
