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

add_library(engine
    private/engine.cpp
    private/resourceScope.cpp
    private/pipelineBuilder.cpp
    private/bufferBuilder.cpp
    private/image.cpp
    private/allocated.cpp
    private/builder.cpp
    private/descriptorSetBuilder.cpp)

target_link_libraries(engine PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap glfw VulkanMemoryAllocator)
target_include_directories(engine PUBLIC public)
target_include_directories(engine PRIVATE private)

file(GLOB GLSL_SHADERS shaders/*.vert shaders/*.frag shaders/*.comp)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

foreach(SHADER IN LISTS GLSL_SHADERS)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(COMPILED_SHADER shaders/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${COMPILED_SHADER}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${COMPILED_SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER}")
    list(APPEND SPV_SHADERS ${COMPILED_SHADER})
endforeach()

add_executable(test demos/test.cpp)
target_include_directories(test PUBLIC ${engine_INCLUDE_DIRECTORIES})
target_link_libraries(test engine)
add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(test shaders)
