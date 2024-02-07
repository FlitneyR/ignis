#pragma once

#include "libraries.hpp"
#include "resourceScope.hpp"
#include "pipelineBuilder.hpp"
#include "allocated.hpp"
#include "descriptorSet.hpp"
#include "image.hpp"
#include "camera.hpp"

namespace ignis {

class GLTFModel {
    std::string m_filename;

    gltf::Model m_model;

    std::vector<Allocated<vk::Buffer>> m_buffers;
    std::vector<Allocated<Image>>      m_images;
    std::vector<vk::ImageView>         m_imageViews;
    std::vector<vk::Sampler>           m_samplers;

    static vk::Pipeline            s_pipeline;
    static vk::Pipeline            s_backupPipeline;
    static vk::PipelineLayout      s_pipelineLayout;
    static vk::DescriptorSetLayout s_materialLayout;
    static Allocated<Image>        s_nullImage;
    static vk::ImageView           s_nullImageView;

    struct MaterialData {
        alignas(16) glm::vec3 emissiveFactor;
        alignas(16) glm::vec4 baseColorFactor;
        alignas(16) float     metallicFactor;
        alignas(16) float     roughnessFactor;
    };

    std::vector<DescriptorSetCollection> m_materials;
    std::vector<MaterialData> m_materialStructs;

    struct Instance {
        glm::mat4 transform;
    };

    std::vector<std::vector<Instance>> m_instances;

    std::array<ResourceScope, 5> m_oneFrameScopes;

    void updateInstances(uint32_t scene = 0) { updateInstances(m_model.scenes[scene]); }
    void updateInstances(gltf::Scene& scene);

    /**
     * @brief Bind a vertex buffer by name
     * 
     * @return true if the primitive contains a vertex attribute with the given name, otherwise false
     */
    bool bindBuffer(vk::CommandBuffer cmd, gltf::Primitive primitive, const char* name, uint32_t binding);

    static constexpr std::vector<std::string> s_supportedExtensions {};

    bool extensionIsSupported(const std::string& extension);

    bool checkCompatibility();
    bool setupBuffers(ResourceScope& scope);
    bool setupImages(ResourceScope& scope);
    bool setupSamplers(ResourceScope& scope);
    bool setupMaterials(ResourceScope& scope);

public:
    enum Status {
        Failed = 0,
        Initial,
        Loaded,
        Ready,
    };

    GLTFModel() = default;
    GLTFModel(GLTFModel&& other) = default;
    GLTFModel& operator =(GLTFModel&& other) = default;

    static bool setupStatics(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout);

    bool load(const std::string& filename, bool async = false);
    bool setup(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout);
    void draw(vk::CommandBuffer cmd, Camera& camera);

    Status status() const { return m_status; }

    std::string& getFileName() { return m_filename; }

    void renderUI();
    void renderSceneUI(uint32_t index = 0) { renderSceneUI(m_model.scenes[index]); }
    void renderSceneUI(gltf::Scene& scene);
    void renderNodeUI(uint32_t nodeID);
    void renderNodeTransformUI(gltf::Node& node);

    bool shouldSetup() const { return isLoaded() && !isReady(); }
    bool isLoaded() const { return m_status >= Loaded; }
    bool isReady() const { return m_status >= Ready; }
    bool failed() const { return m_status == Failed; }

private:
    Status m_status;
    
};

}
