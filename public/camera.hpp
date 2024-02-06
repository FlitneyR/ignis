#pragma once

#include "libraries.hpp"
#include "allocated.hpp"
#include "descriptorSet.hpp"
#include "resourceScope.hpp"

namespace ignis {

struct CameraUniform {
    glm::mat4 view;
    glm::mat4 perspective;
};

struct Camera {
    glm::vec3 position { 0.f, 0.f, 0.f };
    glm::vec3 forward  { 0.f, 1.f, 0.f };
    glm::vec3 up       { 0.f, 0.f, 1.f };

    float near = 0.01f;
    float far = 1'000.f;
    float fov = 45.f;

    std::vector<ignis::Allocated<vk::Buffer>> m_buffers;
    ignis::DescriptorSetCollection            m_descriptorSets;

    void setup(ResourceScope& scope);
    CameraUniform getUniformData(vk::Extent2D viewport);
};

}