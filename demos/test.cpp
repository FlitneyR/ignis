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

template<typename Data>
struct IInstance {
    virtual operator Data() = 0;
};

struct InstanceData {
    glm::mat4 transform = glm::mat4 { 1.f };
};

struct Instance : public IInstance<InstanceData> {
    glm::vec3 position { 0.f, 0.f, 0.f };
    glm::vec3 scale    { 1.f, 1.f, 1.f };
    glm::quat rotation {};

    struct Init {
        glm::vec3 position { 0.f, 0.f, 0.f };
        glm::vec3 scale    { 1.f, 1.f, 1.f };
        glm::quat rotation {};
    };

    Instance(const Init& init) :
        position(init.position),
        scale(init.scale),
        rotation(init.rotation)
    {}

    operator InstanceData() override {
        InstanceData ret {};
        ret. transform = glm::translate(position)
                       * glm::scale(scale)
                       * glm::mat4 { rotation };
        return ret;
    }
};

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

    CameraUniform getUniformData(vk::Extent2D viewport) {
        float width = viewport.width;
        float height = viewport.height;

        auto uniform = CameraUniform {
            .view = glm::lookAt(position, position + forward, up),
            .perspective = glm::perspective(glm::radians(fov), width / height, near, far),
        };
        uniform.perspective = glm::scale(uniform.perspective, { 1.f, -1.f, 1.f});

        return uniform;
    }
};

class Test final : public ignis::IEngine {
    ignis::Allocated<vk::Buffer> m_vertexBuffer;
    ignis::Allocated<vk::Buffer> m_indexBuffer;
    ignis::Allocated<vk::Buffer> m_instanceBuffer;

    Camera m_camera;

    ignis::Allocated<ignis::Image> m_texture;
    ignis::DescriptorSetCollection m_textureDescriptorSet;

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;

    std::vector<Vertex> m_vertices {
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
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
        { { .position = { -1.0f, -1.0f, -1.0f } } },
        { { .position = { -1.0f, -1.0f,  1.0f } } },
        { { .position = { -1.0f,  1.0f, -1.0f } } },
        { { .position = { -1.0f,  1.0f,  1.0f } } },
        { { .position = {  1.0f, -1.0f, -1.0f } } },
        { { .position = {  1.0f, -1.0f,  1.0f } } },
        { { .position = {  1.0f,  1.0f, -1.0f } } },
        { { .position = {  1.0f,  1.0f,  1.0f } } },
    };

    void setup() override {
        auto bufferBuilder = ignis::BufferBuilder { getGlobalResourceScope() }
            .addQueueFamilyIndices({ getQueueIndex(vkb::QueueType::graphics) })
            .setBufferUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_vertexBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setSizeBuildAndStagedCopyData(m_vertices), "Failed to createVertexBuffer");

        m_instanceBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setSize<InstanceData>(m_instances.size())
            .build(), "Failed to create instance buffer");

        m_indexBuffer = ignis::getValue(ignis::BufferBuilder { bufferBuilder }
            .setBufferUsage(vk::BufferUsageFlagBits::eIndexBuffer)
            .setSizeBuildAndStagedCopyData(m_indices), "Failed to create index buffer");

        m_textureDescriptorSet = ignis::DescriptorSetBuilder { getGlobalResourceScope() }
            .setPool(ignis::DescriptorPoolBuilder { getGlobalResourceScope() }
                .setMaxSetCount(1)
                .addPoolSize({ vk::DescriptorType::eCombinedImageSampler, 1 })
                .build())
            .addLayouts(ignis::DescriptorLayoutBuilder { getGlobalResourceScope() }
                .addBinding(vk::DescriptorSetLayoutBinding {}
                    .setBinding(0)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDescriptorCount(1)
                    .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
                .build())
            .build();
        
        m_texture = ignis::ImageBuilder { getGlobalResourceScope() }
            .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .load("../demos/resources/test.jpg");
        
        {
            vk::Sampler sampler = getDevice().createSampler(vk::SamplerCreateInfo {});
            getGlobalResourceScope().addDeferredCleanupFunction([device = getDevice(), sampler]() {
                device.destroySampler(sampler);
            });

            auto imageInfo = vk::DescriptorImageInfo {}
                .setImageLayout(m_texture->layout())
                .setImageView(ignis::ImageViewBuilder { *m_texture, getGlobalResourceScope() }.build())
                .setSampler(sampler);
            
            getDevice().updateDescriptorSets(
                vk::WriteDescriptorSet {}
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDstSet(m_textureDescriptorSet.getSet(0))
                    .setImageInfo(imageInfo),
                {}
            );
        }
        
        m_camera.m_descriptorSets = ignis::DescriptorSetBuilder { getGlobalResourceScope() }
            .setPool(ignis::DescriptorPoolBuilder { getGlobalResourceScope() }
                .setMaxSetCount(s_framesInFlight)
                .addPoolSize({ vk::DescriptorType::eUniformBuffer, s_framesInFlight })
                .build())
            .addLayouts(ignis::DescriptorLayoutBuilder { getGlobalResourceScope() }
                .addBinding(vk::DescriptorSetLayoutBinding {}
                    .setBinding(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
                .build(), s_framesInFlight)
            .build();

        for (int i = 0; i < s_framesInFlight; i++) {
            m_camera.m_buffers.push_back(ignis::getValue(ignis::BufferBuilder { getGlobalResourceScope() }
                .addQueueFamilyIndices({ getQueueIndex(vkb::QueueType::graphics) })
                .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                .setBufferUsage(vk::BufferUsageFlagBits::eUniformBuffer)
                .setSize<CameraUniform>(1)
                .build(), "Failed to create a camera uniform buffer"));
            
            auto bufferInfo = vk::DescriptorBufferInfo {}
                .setBuffer(*m_camera.m_buffers.back())
                .setRange(sizeof(CameraUniform));

            getDevice().updateDescriptorSets(vk::WriteDescriptorSet {}
                .setBufferInfo(bufferInfo)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDstSet(m_camera.m_descriptorSets.getSet(i)), {});
        }

        m_pipelineLayout = ignis::PipelineLayoutBuilder { getGlobalResourceScope() }
            .addSet(m_camera.m_descriptorSets.getLayout())
            .addSet(m_textureDescriptorSet.getLayout())
            .build();

        m_pipeline = ignis::getValue(ignis::GraphicsPipelineBuilder { m_pipelineLayout, getGlobalResourceScope() }
            .addVertexBinding<Vertex>(0)
            .addVertexAttribute<glm::vec3>(0, 0, __offsetof(Vertex, position))
            .addVertexAttribute<glm::vec4>(0, 1, __offsetof(Vertex, color))
            .addInstanceBinding<InstanceData>(1)
            .addVertexAttribute<glm::mat4>(1, 2, __offsetof(InstanceData, transform))
            .addColorAttachmentFormat(getVkbSwapchain().image_format)
            .setDepthAttachmentFormat(getDepthBuffer()->getFormat())
            .addAttachmentBlendState()
            .setDepthStencilState()
            .addStageFromFile("shaders/shader.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
            .addStageFromFile("shaders/shader.frag.spv", "main", vk::ShaderStageFlagBits::eFragment)
            .build(), "Failed to create pipeline");
    }
    
    void recordDrawCommands(vk::CommandBuffer cmd, vk::Extent2D viewport) override {
        m_camera.m_buffers[getInFlightIndex()].copyData(m_camera.getUniformData(viewport));

        std::vector<InstanceData> instanceData(m_instances.size());
        for (int i = 0; i < m_instances.size(); i++) instanceData[i] = m_instances[i];
        m_instanceBuffer.copyData(instanceData);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

        cmd.bindVertexBuffers(0, { *m_vertexBuffer, *m_instanceBuffer }, { 0, 0 });
        cmd.bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint16);

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout,
            0, m_camera.m_descriptorSets.getSet(getInFlightIndex()), {});
        
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout,
            1, m_textureDescriptorSet.getSet(), {});
        
        cmd.drawIndexed(m_indices.size(), m_instances.size(), 0, 0, 0);
    }

    void drawUI() override {
        static float yaw      = 0.f;
        static float pitch    = 0.f;
        static float distance = 5.f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

        ImGui::Begin("Metrics");
        ImGui::Text("FPS: %f", 1.0f / getDeltaTime());
        ImGui::End();

        getLog().draw();

        ImGui::Begin("Scene");

        if (ImGui::TreeNode("Camera")) {
            ImGui::DragFloat("FOV", &m_camera.fov, 1.0f, 30.f, 110.f);
            ImGui::DragFloat("Yaw", &yaw);
            ImGui::DragFloat("Pitch", &pitch, 1.f, -85.f, 85.f);
            ImGui::DragFloat("Distance", &distance, 0.1f, 1.0f, 10.0f);
            ImGui::TreePop();
        }

        m_camera.position = glm::rotate(glm::radians(yaw),   glm::vec3 {  0.f, 0.f, 1.f })
                          * glm::rotate(glm::radians(pitch), glm::vec3 { -1.f, 0.f, 0.f })
                          * glm::vec4 { 0.f, -distance, 0.f, 1.f };
        m_camera.forward = -m_camera.position;

        if (ImGui::TreeNode("Instances")) {
            for (int i = 0; i < m_instances.size(); i++) {
                char instanceID[512];
                std::snprintf(instanceID, sizeof(instanceID), "Instance: %d", i);

                if (ImGui::TreeNode(instanceID)) {
                    glm::vec3 eulerRotation = glm::eulerAngles(m_instances[i].rotation);
                    for (int i = 0; i < 3; i++) eulerRotation[i] = glm::degrees(eulerRotation[i]);

                    ImGui::DragFloat3("Position", &m_instances[i].position[0], 0.1f);
                    ImGui::DragFloat3("Scale",    &m_instances[i].scale[0],    0.1f);
                    ImGui::DragFloat3("Rotation", &eulerRotation[0],           1.0f);

                    for (int i = 0; i < 3; i++) eulerRotation[i] = glm::radians(eulerRotation[i]);
                    m_instances[i].rotation = glm::quat(eulerRotation);
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }

        ImGui::End();

        ImGui::PopStyleVar(3);
    }

    void update() override {}

    std::string getName()       override { return "Test"; }
    uint32_t    getAppVersion() override { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

public:
    static Test s_singleton;
};

Test Test::s_singleton {};

int main() {
    ignis::IEngine::get().main();

    return 0;
}
