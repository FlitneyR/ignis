#include "libraries.hpp"
#include "engine.hpp"
#include "pipelineBuilder.hpp"
#include "allocated.hpp"
#include "bufferBuilder.hpp"
#include "common.hpp"
#include "descriptorSetBuilder.hpp"
#include "gltf.hpp"
#include "camera.hpp"

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

    std::list<ignis::GLTFModel> m_models;

    void setup() override {
        m_camera.setup(getGlobalResourceScope());

        ignis::GLTFModel::setupStatics(getGlobalResourceScope(), m_camera.m_descriptorSets.getLayout());

        getGlobalResourceScope().addDeferredCleanupFunction([&]() { m_models.clear(); });
    }

    void update() override {
        for (auto& model : m_models)
        if (model.shouldSetup())
            model.setup(m_camera.m_descriptorSets.getLayout());
    }
    
    void recordDrawCommands(vk::CommandBuffer cmd, vk::Extent2D viewport) override {
        m_camera.m_buffers[getInFlightIndex()].copyData(m_camera.getUniformData(viewport));

        for (auto& model : m_models)
        if (model.isReady())
            model.draw(cmd, m_camera);
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
        
        if (ImGui::BeginMenu("Load model")) {
            static char filename[512] = "";

            ImGui::InputText("File name", filename, sizeof(filename));

            if (ImGui::Button("Load")) {
                IGNIS_LOG("glTF", Debug, "Entered filename: " << filename);
                m_models.emplace_back().loadAsync(filename);
                filename[0] = '\0';
            }

            ImGui::EndMenu();
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

        for (auto it = m_models.begin(); it != m_models.end();) {
            auto& model = *it;

            if (ImGui::TreeNode(&model, "%s", model.getFileName().c_str())) {
                if (model.isReady()) {
                    if (ImGui::Button("Delete")) {
                        getDevice().waitIdle();
                        m_models.erase(it++);

                        ImGui::TreePop();
                        continue;
                    }

                    model.renderUI();
                }

                ImGui::TreePop();
            }

            it++;
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
