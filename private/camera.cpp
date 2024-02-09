#include "camera.hpp"
#include "common.hpp"
#include "engine.hpp"
#include "bufferBuilder.hpp"
#include "uniformBuilder.hpp"

namespace ignis {

CameraUniform Camera::getUniformData(vk::Extent2D viewport) {
    float width = viewport.width;
    float height = viewport.height;

    auto uniform = CameraUniform {
        .view = glm::lookAt(position, position + forward, up),
        .perspective = glm::perspective(glm::radians(fov), width / height, near, far),
    };
    uniform.perspective = glm::scale(uniform.perspective, { 1.f, -1.f, 1.f});

    return uniform;
}

void Camera::setup(ResourceScope& scope) {
    uniform = ignis::UniformBuilder { scope }
        .setPool(ignis::DescriptorPoolBuilder { scope }
            .setMaxSetCount(IEngine::s_framesInFlight)
            .addPoolSize({ vk::DescriptorType::eUniformBuffer, IEngine::s_framesInFlight })
            .build())
        .addLayouts(ignis::DescriptorLayoutBuilder { scope }
            .addBinding(vk::DescriptorSetLayoutBinding {}
                .setBinding(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
            .build(), IEngine::s_framesInFlight)
        .build();

    std::vector<Uniform::Update> updates;
    for (int i = 0; i < IEngine::s_framesInFlight; i++) {
        m_buffers.push_back(getValue(ignis::BufferBuilder { scope }
            .addQueueFamilyIndices({ IEngine::get().getQueueIndex(vkb::QueueType::graphics) })
            .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .setBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
            .setSize<ignis::CameraUniform>(1)
            .build(), "Failed to create a camera uniform buffer"));
        
        updates.push_back(uniform.update(vk::DescriptorType::eUniformBuffer, i, 0)
            .addBufferInfo(vk::DescriptorBufferInfo {}
                .setBuffer(*m_buffers.back())
                .setRange(sizeof(CameraUniform))));
    }
    Uniform::updateUniforms(updates);
}

}
