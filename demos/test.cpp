#include "libraries.hpp"
#include "engine.hpp"
#include "pipelineBuilder.hpp"
#include "allocated.hpp"
#include "bufferBuilder.hpp"
#include "common.hpp"
#include "descriptorSetBuilder.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

struct Instance {
    glm::mat4 transform;
};

struct Camera {
    glm::mat4 view;
    glm::mat4 perspective;
};

class Test final : public ignis::IEngine {
    ignis::Allocated<vk::Buffer> m_vertexBuffer;
    ignis::Allocated<vk::Buffer> m_indexBuffer;
    ignis::Allocated<vk::Buffer> m_instanceBuffer;

    vk::DescriptorSetLayout m_cameraDescriptorSetLayout;
    std::vector<ignis::Allocated<vk::Buffer>> m_cameraBuffers;
    std::vector<vk::DescriptorSet>            m_cameraDescriptorSets;

    Camera m_camera {
        .view = glm::translate(glm::mat4 { 1.f }, { 0.f, 0.f, -5.f }),
        .perspective = glm::perspectiveFov(glm::radians(45.f), 1280.f, 720.f, 0.01f, 1000.f),
    };

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;

    std::vector<Vertex> m_vertices {
        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    };

    std::vector<uint16_t> m_indices {
        0, 1, 2,    1, 3, 2,
        1, 5, 3,    3, 5, 7,
        5, 4, 7,    7, 4, 6,
        4, 0, 2,    4, 2, 6,
        0, 5, 1,    0, 4, 5,
        3, 7, 2,    2, 7, 6,
    };

    std::vector<Instance> m_instances {
        { glm::translate(glm::mat4 { 1.f }, { -1.0f, -1.0f, -1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, { -1.0f, -1.0f,  1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, { -1.0f,  1.0f, -1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, { -1.0f,  1.0f,  1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, {  1.0f, -1.0f, -1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, {  1.0f, -1.0f,  1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, {  1.0f,  1.0f, -1.0f }) },
        { glm::translate(glm::mat4 { 1.f }, {  1.0f,  1.0f,  1.0f }) },
    };

    void setup() {
        auto bufferBuilder = ignis::BufferBuilder { getGlobalResourceScope() }
            .addQueueFamilyIndices({ getQueueIndex(vkb::QueueType::graphics) })
            .setBufferUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_vertexBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setSizeBuildAndStagedCopyData(m_vertices), "Failed to createVertexBuffer");

        m_instanceBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setSizeBuildAndStagedCopyData(m_instances), "Failed to create instance buffer");

        m_indexBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setBufferUsage(vk::BufferUsageFlagBits::eIndexBuffer)
            .setSizeBuildAndStagedCopyData(m_indices), "Failed to create index buffer");
        
        m_cameraDescriptorSetLayout = ignis::DescriptorLayoutBuilder { getGlobalResourceScope() }
            .addBinding(vk::DescriptorSetLayoutBinding {}
                .setBinding(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
            .build();
        
        m_cameraDescriptorSets = ignis::DescriptorSetBuilder { getGlobalResourceScope() }
            .setPool(ignis::DescriptorPoolBuilder { getGlobalResourceScope() }
                .setMaxSetCount(s_framesInFlight)
                .addPoolSize({ vk::DescriptorType::eUniformBuffer, s_framesInFlight })
                .build())
            .addLayouts(m_cameraDescriptorSetLayout, s_framesInFlight)
            .build();

        for (int i = 0; i < s_framesInFlight; i++) {
            m_cameraBuffers.push_back(ignis::getValue(ignis::BufferBuilder { getGlobalResourceScope() }
                .addQueueFamilyIndices({ getQueueIndex(vkb::QueueType::graphics) })
                .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                .setBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
                .setSizeBuildAndCopyData(m_camera), "Failed to create a camera uniform buffer"));
            
            auto bufferInfo = vk::DescriptorBufferInfo {}
                .setBuffer(*m_cameraBuffers.back())
                .setRange(sizeof(Camera));

            getDevice().updateDescriptorSets(vk::WriteDescriptorSet {}
                .setBufferInfo(bufferInfo)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstSet(m_cameraDescriptorSets[i]), {});
        }

        m_pipelineLayout = ignis::PipelineLayoutBuilder { getGlobalResourceScope() }
            .addSet(m_cameraDescriptorSetLayout)
            .build();

        m_pipeline = ignis::getValue(ignis::GraphicsPipelineBuilder { m_pipelineLayout, getGlobalResourceScope() }
            .addVertexBinding<Vertex>(0)
            .addInstanceBinding<Instance>(1)
            .addVertexAttribute<glm::vec3>(0, 0, __offsetof(Vertex, position))
            .addVertexAttribute<glm::vec4>(0, 1, __offsetof(Vertex, color))
            .addVertexAttribute<glm::mat4>(1, 2, __offsetof(Instance, transform))
            .addColorAttachmentFormat(m_swapchain.image_format)
            .setDepthAttachmentFormat(m_depthImage->getFormat())
            .addAttachmentBlendState(ignis::GraphicsPipelineBuilder::defaultAttachmentBlendState())
            .setDepthStencilState(ignis::GraphicsPipelineBuilder::defaultDepthStencilState())
            .addViewport(vk::Viewport {})
            .addScissor(vk::Rect2D {})
            .addStageFromFile("shaders/shader.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
            .addStageFromFile("shaders/shader.frag.spv", "main", vk::ShaderStageFlagBits::eFragment)
            .build(), "Failed to create pipeline");
    }
    
    void recordDrawCommands(vk::CommandBuffer cmd) {
        m_cameraBuffers[getInFlightIndex()].copyData(m_camera);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

        cmd.bindVertexBuffers(0, { *m_vertexBuffer, *m_instanceBuffer }, { 0, 0 });
        cmd.bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint16);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout,
            0, m_cameraDescriptorSets[getInFlightIndex()], {});
        
        cmd.drawIndexed(m_indices.size(), m_instances.size(), 0, 0, 0);
    }

    void update(double deltaTime) {
        double time = getTime();
        auto eye = glm::normalize(glm::vec3 { sin(time), cos(time / 3.f), cos(time) }) * 10.0f;
        m_camera.view = glm::lookAt(eye, {}, { 0.0f, 1.0f, 0.0f });

        ImGui::ShowDemoWindow();
    }

    std::string getName()       { return "Test"; }
    uint32_t    getAppVersion() { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

    Test() {}

public:
    static Test s_singleton;
};

Test Test::s_singleton {};

int main() {
    ignis::IEngine::get().main();

    return 0;
}
