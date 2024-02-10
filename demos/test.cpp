#include "libraries.hpp"
#include "engine.hpp"
#include "pipelineBuilder.hpp"
#include "allocated.hpp"
#include "bufferBuilder.hpp"
#include "common.hpp"
#include "uniformBuilder.hpp"
#include "gltf.hpp"
#include "camera.hpp"
#include "bloom.hpp"

#include <thread>

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
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

class Test final : public ignis::IEngine {
    ignis::Camera m_camera;

    std::optional<ignis::GLTFModel> m_model;

    bool m_bloomAvailable = false;
    ignis::BloomPostProcess m_bloomPass;

    void setup() override {
        m_camera.setup(getGlobalResourceScope());

        ignis::GLTFModel::setupStatics(getGlobalResourceScope(), m_camera.uniform.getLayout());

        getGlobalResourceScope().addDeferredCleanupFunction([&]() { m_model = std::nullopt; });

        m_model = ignis::GLTFModel();
        m_model->loadAsync("../demos/assets/BoomBoxTest.glb");
    }

    void onWindowSizeChanged(glm::vec<2, uint32_t> size) override {
        m_bloomPass = {};
        m_bloomAvailable = m_bloomPass.setup(getUntilWindowSizeChangeScope());
    }

    void update() override {
        if (m_model->shouldSetup())
            m_model->setup(m_camera.uniform.getLayout());
    }
    
    void recordGBufferCommands(vk::CommandBuffer cmd, vk::Extent2D viewport) override {
        m_camera.m_buffers[getInFlightIndex()].copyData(m_camera.getUniformData(viewport));

        if (m_model->isReady())
            m_model->drawMeshes(cmd, m_camera);
    }

    void recordLightingCommands(vk::CommandBuffer cmd, vk::Extent2D viewport) override {
        if (m_model->isReady())
            m_model->drawLights(cmd, m_camera);
    }

    void recordPostProcessingCommands(vk::CommandBuffer cmd, vk::Extent2D viewport) override {
        m_bloomPass.draw(cmd);
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
        
        if (ImGui::BeginMenu("Load scene")) {
            static char filename[512] = "";

            ImGui::InputText("File name", filename, sizeof(filename));

            if (ImGui::Button("Load")) {
                m_model = ignis::GLTFModel();
                m_model->loadAsync(filename);
                filename[0] = '\0';
            }

            ImGui::EndMenu();
        }

        if (ImGui::TreeNode("Bloom")) {
            ImGui::DragFloat("Clipping", &m_bloomPass.clipping, 0.05f, 0.0f, FLT_MAX);
            ImGui::DragFloat("Dispersion", &m_bloomPass.dispersion, 0.05f, 0.0f, FLT_MAX);
            ImGui::DragFloat("Mixing", &m_bloomPass.mixing, 0.05f, 0.0f, 1.0f);
            
            ImGui::TreePop();
        }

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

        int i = 0;

        if (m_model->isReady()) {
            m_model->renderUI();
        }

        ImGui::End();

        ImGui::PopStyleVar(3);
    }

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
