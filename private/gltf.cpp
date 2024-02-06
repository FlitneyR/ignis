#include "gltf.hpp"
#include "log.hpp"
#include "bufferBuilder.hpp"
#include "descriptorSetBuilder.hpp"
#include "engine.hpp"
#include <thread>

namespace ignis {

bool GLTFModel::load(std::string filename, bool async) {
    if (async) {
        std::thread([&, filename]() {
            load(filename);
        }).detach();

        return true;
    }

    gltf::TinyGLTF loader;
    std::string error, warning;

    IGNIS_LOG("glTF", Info, "Loading glTF file: " << filename);
    bool loadSuccess = loader.LoadBinaryFromFile(&m_model, &error, &warning, filename);

    if (error != "") {
        error.pop_back();
        IGNIS_LOG("glTF", Error, error);
    }

    if (warning != "") {
        warning.pop_back();
        IGNIS_LOG("glTF", Warning, warning);
    }

    if (!loadSuccess) {
        IGNIS_LOG("glTF", Error, "Failed to load glTF file: " << filename);
        return false;
    }

    IGNIS_LOG("glTF", Info, "Loaded glTF file: " << filename);

    m_status = Loaded;
    return true;
}

bool GLTFModel::setupBuffers(ResourceScope& scope) {
    for (int i = 0; i < m_model.buffers.size(); i++) {
        auto bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer
                         | vk::BufferUsageFlagBits::eIndexBuffer;

        auto allocationUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        auto bufferResult = BufferBuilder { scope }
            .setBufferUsage(bufferUsage)
            .setAllocationUsage(allocationUsage)
            .setSizeBuildAndCopyData(m_model.buffers[i].data);
        
        if (bufferResult.result != vk::Result::eSuccess) {
            IGNIS_LOG("glTF", Error, "Failed to create buffer: " << bufferResult.result);
            return false;
        }

        m_buffers.push_back(bufferResult.value);
    }
    
    return true;
}

bool GLTFModel::setupImages(ResourceScope& scope) {
    for (auto& image : m_model.images) {
        m_images.push_back(ImageBuilder { scope }
            .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .load(&image.image[0], image.width, image.height));
        
        m_imageViews.push_back(ImageViewBuilder { *m_images.back(), scope }
            .build());
    }

    return true;
}

bool GLTFModel::setupSamplers(ResourceScope& scope) {
    vk::Device device = IEngine::get().getDevice();

    vk::Sampler defaultSampler = device.createSampler(vk::SamplerCreateInfo {});
    scope.addDeferredCleanupFunction([=, sampler = defaultSampler]() {
        device.destroySampler(sampler);
    });
    m_samplers.push_back(defaultSampler);

    // setup samplers
    for (auto& sampler : m_model.samplers) {
        auto createInfo = vk::SamplerCreateInfo {};

        switch (sampler.wrapS) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:          createInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat); break;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:   createInfo.setAddressModeU(vk::SamplerAddressMode::eClampToEdge); break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: createInfo.setAddressModeU(vk::SamplerAddressMode::eMirroredRepeat); break;
        }

        switch (sampler.wrapT) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:          createInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat); break;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:   createInfo.setAddressModeV(vk::SamplerAddressMode::eClampToEdge); break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: createInfo.setAddressModeV(vk::SamplerAddressMode::eMirroredRepeat); break;
        }

        switch (sampler.minFilter) {
        case TINYGLTF_TEXTURE_FILTER_LINEAR:  createInfo.setMinFilter(vk::Filter::eLinear); break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST: createInfo.setMinFilter(vk::Filter::eNearest); break;
        }

        switch (sampler.magFilter) {
        case TINYGLTF_TEXTURE_FILTER_LINEAR:  createInfo.setMinFilter(vk::Filter::eLinear); break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST: createInfo.setMinFilter(vk::Filter::eNearest); break;
        }

        m_samplers.push_back(device.createSampler(createInfo));
        
        scope.addDeferredCleanupFunction([=, sampler = m_samplers.back()]() {
            device.destroySampler(sampler);
        });
    }

    return true;
}

bool GLTFModel::setupMaterials(ResourceScope& scope) {
    uint32_t materialCount = m_model.materials.size();

    vk::DescriptorPool materialPool = DescriptorPoolBuilder { scope }
        .setMaxSetCount(materialCount)
        .addPoolSize({ vk::DescriptorType::eCombinedImageSampler, 5 * materialCount })
        .build();

    auto binding = vk::DescriptorSetLayoutBinding {}
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics);

    vk::DescriptorSetLayout materialLayout = DescriptorLayoutBuilder { scope }
        .addBinding(binding.setBinding(0))
        .addBinding(binding.setBinding(1))
        .addBinding(binding.setBinding(2))
        .addBinding(binding.setBinding(3))
        .addBinding(binding.setBinding(4))
        .build();
    
    for (auto& material : m_model.materials) {
        m_materials.push_back(DescriptorSetBuilder { scope, materialPool }
            .addLayouts(materialLayout)
            .build());
        
        std::vector<int> textureIDs {
            material.pbrMetallicRoughness.baseColorTexture.index,
            material.pbrMetallicRoughness.metallicRoughnessTexture.index,
            material.emissiveTexture.index,
            material.occlusionTexture.index,
            material.normalTexture.index
        };
        
        std::vector<vk::DescriptorImageInfo> imageInfos;
        std::vector<vk::WriteDescriptorSet> descriptorWrites;

        for (int binding = 0; binding < textureIDs.size(); binding++) {
            auto& texture = m_model.textures[textureIDs[binding]];

            imageInfos.push_back(vk::DescriptorImageInfo {}
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(texture.source < 0 ? VK_NULL_HANDLE : m_imageViews[texture.source])
                .setSampler(m_samplers[std::min<uint32_t>(texture.sampler + 1, m_samplers.size() - 1)]));
            
            descriptorWrites.push_back(vk::WriteDescriptorSet {}
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setDstSet(m_materials.back().getSet())
                .setDstBinding(binding)
                .setImageInfo(imageInfos.back()));
            
            IEngine::get().getDevice().updateDescriptorSets(descriptorWrites.back(), {});
        }

        m_materialStructs.push_back(MaterialData {
            .emissiveFactor = { material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2] },
            .baseColorFactor = {
                material.pbrMetallicRoughness.baseColorFactor[0],
                material.pbrMetallicRoughness.baseColorFactor[1],
                material.pbrMetallicRoughness.baseColorFactor[2],
                material.pbrMetallicRoughness.baseColorFactor[3],
            },
            .metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
            .roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor),
        });
    }

    return true;
}

bool GLTFModel::setupPipelines(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout) {
    for (auto& material : m_materials)
        m_pipelineLayouts.push_back(PipelineLayoutBuilder { scope }
            .addSet(cameraUniformLayout)
            .addSet(material.getLayout())
            .addPushConstantRange(vk::PushConstantRange {}
                .setSize(sizeof(MaterialData))
                .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
            .build());

    for (auto& mesh : m_model.meshes)
    for (auto& primitive : mesh.primitives) {
        auto pipelineBuilder = GraphicsPipelineBuilder { m_pipelineLayouts[primitive.material], scope }
            .addColorAttachmentFormat(IEngine::get().getVkbSwapchain().image_format)
            .setDepthAttachmentFormat(IEngine::get().getDepthBuffer()->getFormat())
            .addAttachmentBlendState()
            .setDepthStencilState();

        bool foundPositionBinding = false;
        bool foundUVBinding = false;
        bool foundNormalBinding = false;
        bool foundTangentBinding = false;

        for (auto& attribute : primitive.attributes) {
            if (attribute.first == "POSITION") {
                pipelineBuilder
                    .addVertexAttribute<glm::vec3>(attribute.second, 0, 0)
                    .addVertexBinding<glm::vec3>(attribute.second);
                foundPositionBinding = true;
            }
            
            if (attribute.first == "TEXCOORD_0") {
                pipelineBuilder
                    .addVertexAttribute<glm::vec2>(attribute.second, 1, 0)
                    .addVertexBinding<glm::vec2>(attribute.second);
                foundUVBinding = true;
            }
            
            if (attribute.first == "NORMAL") {
                pipelineBuilder
                    .addVertexAttribute<glm::vec3>(attribute.second, 2, 0)
                    .addVertexBinding<glm::vec3>(attribute.second);
                foundNormalBinding = true;
            }
            
            if (attribute.first == "TANGENT") {
                pipelineBuilder
                    .addVertexAttribute<glm::vec4>(attribute.second, 3, 0)
                    .addVertexBinding<glm::vec4>(attribute.second);
                foundTangentBinding = true;
            }
        }

        if (!(foundPositionBinding && foundUVBinding && foundNormalBinding && foundTangentBinding)) {
            if (!foundPositionBinding) IGNIS_LOG("glTF", Error, "Failed to create glTF model pipeline, no POSITION vertex attribute");
            if (!foundTangentBinding)  IGNIS_LOG("glTF", Error, "Failed to create glTF model pipeline, no TANGENT vertex attribute");
            if (!foundNormalBinding)   IGNIS_LOG("glTF", Error, "Failed to create glTF model pipeline, no NORMAL vertex attribute");
            if (!foundUVBinding)       IGNIS_LOG("glTF", Error, "Failed to create glTF model pipeline, no TEXCOORD_0 vertex attribute");

            return false;
        }
        
        try {
            pipelineBuilder
                .addStageFromFile("shaders/gltf.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
                .addStageFromFile("shaders/gltf.frag.spv", "main", vk::ShaderStageFlagBits::eFragment);
        } catch (std::runtime_error& e) {
            IGNIS_LOG("glTF", Error, "Error while loading glTF shader: " << e.what());
            return false;
        }

        pipelineBuilder
            .addVertexAttribute<glm::mat4>(10, 4, 0)
            .addInstanceBinding<Instance>(10);

        auto pipelineResult = pipelineBuilder.build();

        if (pipelineResult.result != vk::Result::eSuccess) {
            IGNIS_LOG("glTF", Error, "Failed to create pipeline: " << pipelineResult.result);
            return false;
        }

        m_pipelines.push_back(pipelineResult.value);
    }

    return true;
}

bool GLTFModel::setup(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout) {
    vk::Device device = IEngine::get().getDevice();
    
    for (auto& s : m_oneFrameScopes) 
        scope.addDeferredCleanupFunction([&]() { s.executeDeferredCleanupFunctions(); });

    bool success = true
        && setupBuffers(scope)
        && setupImages(scope)
        && setupSamplers(scope)
        && setupMaterials(scope)
        && setupPipelines(scope, cameraUniformLayout);

    m_status = success ? Ready : Failed;

    return success;
}

void GLTFModel::updateInstances(gltf::Scene& scene) {
    m_instances.clear();
    m_instances.resize(m_model.meshes.size());

    for (auto& nodeID : scene.nodes) {
        auto& node = m_model.nodes[nodeID];

        glm::mat4 mat { 1.f };

        if (node.matrix.size() > 0) {
            mat[0] = { node.matrix[ 0], node.matrix[ 1], node.matrix[ 2], node.matrix[ 3] };
            mat[1] = { node.matrix[ 4], node.matrix[ 5], node.matrix[ 6], node.matrix[ 7] };
            mat[2] = { node.matrix[ 8], node.matrix[ 9], node.matrix[10], node.matrix[11] };
            mat[3] = { node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15] };
        }

        m_instances[node.mesh].push_back({ mat });
    }
}

void GLTFModel::bindBuffer(vk::CommandBuffer cmd, gltf::Primitive primitive, const char* name) {
    auto& accessorIndex = primitive.attributes.at(name);
    auto& accessor = m_model.accessors[accessorIndex];
    auto& bufferView = m_model.bufferViews[accessor.bufferView];
    auto& buffer = m_buffers[bufferView.buffer];

    cmd.bindVertexBuffers(accessorIndex, *buffer, bufferView.byteOffset, {});
}

void GLTFModel::draw(vk::CommandBuffer cmd, ignis::Camera& camera) {
    updateInstances();

    auto& oneFrameScope = m_oneFrameScopes[IEngine::get().getInFlightIndex()];

    oneFrameScope.executeDeferredCleanupFunctions();
    std::vector<Allocated<vk::Buffer>> instanceBuffers;

    for (auto& instances : m_instances) {
        auto bufferResult = BufferBuilder { oneFrameScope }
            .setBufferUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .setSizeBuildAndCopyData(instances);
        
        if (bufferResult.result != vk::Result::eSuccess) {
            IGNIS_LOG("glTF", Error, "Failed to create one-frame instance buffer: vk::Result = " << bufferResult.result);
            return;
        }

        instanceBuffers.push_back(bufferResult.value);
    }

    uint32_t pipelineID = 0;

    for (int meshID = 0; meshID < m_model.meshes.size(); meshID++) {
        auto& mesh = m_model.meshes[meshID];
        for (auto& primitive : mesh.primitives) {
            auto& layout = m_pipelineLayouts[primitive.material];

            vk::DescriptorSet cameraDescriptorSet = camera.m_descriptorSets.getSet(IEngine::get().getInFlightIndex());

            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipelines[pipelineID++]);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, cameraDescriptorSet, {});
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 1, m_materials[primitive.material].getSet(), {});
            cmd.pushConstants<MaterialData>(layout, vk::ShaderStageFlagBits::eAllGraphics, 0, m_materialStructs[primitive.material]);

            bindBuffer(cmd, primitive, "POSITION");
            bindBuffer(cmd, primitive, "TEXCOORD_0");
            bindBuffer(cmd, primitive, "NORMAL");
            bindBuffer(cmd, primitive, "TANGENT");

            cmd.bindVertexBuffers(10, *instanceBuffers[meshID], { 0 }, {});

            auto& indexAccessor = m_model.accessors[primitive.indices];
            uint32_t indexCount = indexAccessor.count;
            auto& indexBufferView = m_model.bufferViews[indexAccessor.bufferView];
            auto& indexBuffer = m_buffers[indexBufferView.buffer];
            cmd.bindIndexBuffer(*indexBuffer, indexBufferView.byteOffset, vk::IndexType::eUint16);

            cmd.drawIndexed(indexCount, 1, 0, 0, 0);
        }
    }
}

void GLTFModel::renderUI() {
    renderSceneUI();
}

void GLTFModel::renderSceneUI(gltf::Scene& scene) {
    for (auto& nodeID : scene.nodes)
        renderNodeUI(nodeID);
}

void GLTFModel::renderNodeUI(uint32_t nodeID) {
    gltf::Node& node = m_model.nodes[nodeID];
    if (!ImGui::TreeNode(node.name.c_str())) return;
    
    renderNodeTransformUI(node);

    if (ImGui::TreeNode("Materials")) {
        for (auto& primitive : m_model.meshes[node.mesh].primitives)
        if (ImGui::TreeNode(("Name: " + m_model.materials[primitive.material].name).c_str())) {
            MaterialData& material = m_materialStructs[primitive.material];

            ImGui::DragFloat3("Base color factor", &material.baseColorFactor.x, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat3("Emissive factor",   &material.emissiveFactor.x,  0.05f, 0.0f);
            ImGui::DragFloat("Metallic factor",  &material.metallicFactor,  0.025f, 0.0f, 1.0f);
            ImGui::DragFloat("Roughness factor", &material.roughnessFactor, 0.025f, 0.0f, 1.0f);

            material.emissiveFactor = glm::max(material.emissiveFactor, 0.f);

            ImGui::TreePop();
        }
        
        ImGui::TreePop();
    }

    for (auto& child : node.children)
        renderNodeUI(child);
    
    ImGui::TreePop();
}

void GLTFModel::renderNodeTransformUI(gltf::Node& node) {
    glm::vec3 position { 0, 0, 0 };
    glm::vec3 scale    { 1, 1, 1 };
    glm::vec3 euler    { 0, 0, 0 };

    if (node.translation.size() > 0) {
        position = { node.translation[0], node.translation[1], node.translation[2] };
    }

    if (node.scale.size() > 0) {
        scale = { node.scale[0], node.scale[1], node.scale[2] };
    }

    if (node.rotation.size() > 0) {
        euler = glm::eulerAngles(glm::quat {
            static_cast<float>(node.rotation[3]),
            static_cast<float>(node.rotation[0]),
            static_cast<float>(node.rotation[1]),
            static_cast<float>(node.rotation[2])
        });
    }

    euler *= 180.f / glm::pi<float>();

    ImGui::DragFloat3("Position", &position.x, 0.1f);
    ImGui::DragFloat3("Scale", &scale.x, 0.1f);
    ImGui::DragFloat3("Rotation", &euler.x, 1.0f);

    euler /= 180.f / glm::pi<float>();

    glm::quat rotation { euler };

    glm::mat4 matrix = glm::translate(position)
                     * glm::mat4 { rotation }
                     * glm::scale(scale);
    
    node.translation = { position.x, position.y, position.z };
    node.scale       = { scale.x,    scale.y,    scale.z };
    node.rotation    = { rotation.x, rotation.y, rotation.z, rotation.w };

    node.matrix = {
        matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
        matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
        matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
        matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3],
    };
}

}
