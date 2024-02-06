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
    gltf::Model m_model;

    std::vector<Allocated<vk::Buffer>> m_buffers;
    std::vector<Allocated<Image>>      m_images;
    std::vector<vk::ImageView>         m_imageViews;
    std::vector<vk::Sampler>           m_samplers;

    std::vector<vk::PipelineLayout> m_pipelineLayouts;
    std::vector<vk::Pipeline>       m_pipelines;

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

    void bindBuffer(vk::CommandBuffer cmd, gltf::Primitive primitive, const char* name);

    bool setupBuffers(ResourceScope& scope);
    bool setupImages(ResourceScope& scope);
    bool setupSamplers(ResourceScope& scope);
    bool setupMaterials(ResourceScope& scope);
    bool setupPipelines(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout);

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

    bool load(std::string filename, bool async = false);
    bool setup(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout);
    void draw(vk::CommandBuffer cmd, Camera& camera);

    Status status() { return m_status; }

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
