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
FetchContent_Declare(
    DearImGUI
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        v1.90.1-docking
)

FetchContent_MakeAvailable(vk_bootstrap fetch_VulkanMemoryAllocator DearImGUI)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
set(imgui_SRC ${FETCHCONTENT_BASE_DIR}/dearimgui-src)
set(imgui_INCLUDE ${imgui_SRC})
set(imgui_BACKENDS ${imgui_SRC}/backends)
add_library(imgui
    ${imgui_SRC}/imgui.cpp
    ${imgui_SRC}/imgui_draw.cpp
    ${imgui_SRC}/imgui_demo.cpp
    ${imgui_SRC}/imgui_tables.cpp
    ${imgui_SRC}/imgui_widgets.cpp
    ${imgui_BACKENDS}/imgui_impl_glfw.cpp
    ${imgui_BACKENDS}/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC ${imgui_INCLUDE} ${imgui_BACKENDS})
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)

add_library(engine
    private/engine.cpp
    private/resourceScope.cpp
    private/pipelineBuilder.cpp
    private/bufferBuilder.cpp
    private/image.cpp
    private/allocated.cpp
    private/builder.cpp
    private/descriptorSet.cpp
    private/descriptorSetBuilder.cpp
    private/log.cpp
    private/external/stb_image_impl.cpp
)

target_link_libraries(engine PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap glfw VulkanMemoryAllocator imgui)
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
target_link_libraries(test engine)
add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(test shaders)
