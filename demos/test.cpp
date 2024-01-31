#include "libraries.hpp"
#include "engine.hpp"
#include "pipelineBuilder.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

class Test final : public ignis::IEngine {
    vk::Buffer m_vertexBuffer;
    VmaAllocation m_vertexBufferAllocation;

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;

    std::vector<Vertex> m_vertices {
        { {  0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    void setup() {
        auto& grs = getGlobalResourceScope();

        VkBuffer vertexBuffer;

        auto queueIndex = getQueueIndex(vkb::QueueType::graphics);
        VkBufferCreateInfo vertexBufferCreateInfo = vk::BufferCreateInfo {}
            .setQueueFamilyIndices(queueIndex)
            .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setSize(sizeof(Vertex) * m_vertices.size());

        VmaAllocationCreateInfo vertexBufferAllocationInfo {
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        };

        vmaCreateBuffer(getAllocator(), &vertexBufferCreateInfo, &vertexBufferAllocationInfo, &vertexBuffer, &m_vertexBufferAllocation, nullptr);
        m_vertexBuffer = vertexBuffer;
        grs.addDeferredCleanupFunction([&]() {
            vmaDestroyBuffer(getAllocator(), m_vertexBuffer, m_vertexBufferAllocation);
        });

        VmaAllocationInfo vbInfo;
        vmaGetAllocationInfo(getAllocator(), m_vertexBufferAllocation, &vbInfo);
        void* p_data = getDevice().mapMemory(vbInfo.deviceMemory, vbInfo.offset, vbInfo.size);
        std::memcpy(p_data, m_vertices.data(), sizeof(Vertex) * m_vertices.size());
        getDevice().unmapMemory(vbInfo.deviceMemory);

        m_pipelineLayout = ignis::PipelineLayoutBuilder { getDevice() }
            .build();
        grs.addDeferredCleanupFunction([&]() {
            getDevice().destroyPipelineLayout(m_pipelineLayout);
        });

        auto pipeline_result = ignis::GraphicsPipelineBuilder { getDevice(), m_pipelineLayout }
            .addVertexBinding(vk::VertexInputBindingDescription {}
                .setBinding(0).setInputRate(vk::VertexInputRate::eVertex).setStride(sizeof(Vertex)))
            
            .addVertexAttribute(vk::VertexInputAttributeDescription {}
                .setBinding(0).setFormat(vk::Format::eR32G32B32Sfloat)
                .setLocation(0).setOffset(__offsetof(Vertex, position)))
            .addVertexAttribute(vk::VertexInputAttributeDescription {}
                .setBinding(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
                .setLocation(1).setOffset(__offsetof(Vertex, color)))

            .addColorAttachmentFormat(static_cast<vk::Format>(m_swapchain.image_format))
            .addAttachmentBlendState(ignis::GraphicsPipelineBuilder::defaultAttachmentBlendState())
            .addViewport(vk::Viewport {})
            .addScissor(vk::Rect2D {})

            .addStageFromFile("shaders/shader.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
            .addStageFromFile("shaders/shader.frag.spv", "main", vk::ShaderStageFlagBits::eFragment)

            .build();
        vk::resultCheck(pipeline_result.result, "Failed to create pipeline");
        m_pipeline = pipeline_result.value.pipeline;

        grs.addDeferredCleanupFunction([&, pipeline_result]() {
            getDevice().destroyPipeline(m_pipeline);
            for (auto& shaderModule : pipeline_result.value.shaderModules)
                getDevice().destroyShaderModule(shaderModule);
        });
    }
    
    void recordDrawCommands(vk::CommandBuffer cmd) {
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        cmd.bindVertexBuffers(0, { m_vertexBuffer }, { 0 });
        cmd.draw(3, 1, 0, 0);
    }

    void update(double deltaTime) {}

    std::string getName()       { return "Test"; }
    uint32_t    getAppVersion() { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

public:
    Test() {
        init();
        setup();
    }
};

int main() {
    Test test;

    test.mainLoop();

    return 0;
}
