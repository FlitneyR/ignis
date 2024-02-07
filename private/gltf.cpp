#include "gltf.hpp"
#include "log.hpp"
#include "bufferBuilder.hpp"
#include "descriptorSetBuilder.hpp"
#include "engine.hpp"
#include <thread>

namespace ignis {

vk::Pipeline            GLTFModel::s_pipeline       = VK_NULL_HANDLE;
vk::PipelineLayout      GLTFModel::s_pipelineLayout = VK_NULL_HANDLE;
vk::DescriptorSetLayout GLTFModel::s_materialLayout = VK_NULL_HANDLE;
vk::Pipeline            GLTFModel::s_backupPipeline = VK_NULL_HANDLE;
Allocated<Image>        GLTFModel::s_nullImage      = {};
vk::ImageView           GLTFModel::s_nullImageView  = VK_NULL_HANDLE;

bool GLTFModel::load(const std::string& filename, bool async) {
    m_filename = filename;

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

bool GLTFModel::extensionIsSupported(const std::string& extension) {
    for (auto& supportedExtension : s_supportedExtensions)
        if (supportedExtension == extension) return true;
    
    return false;
}

bool GLTFModel::checkCompatibility() {
    if (m_model.skins.size() > 0)
        IGNIS_LOG("glTF", Warning, "Model " << getFileName() << " uses skinning which is not supported");

    for (auto& extension : m_model.extensionsUsed)
    if (!extensionIsSupported(extension))
        IGNIS_LOG("glTF", Warning, "Model " << getFileName() << " uses unsupported extension " << extension);
    
    bool anyMissingRequiredExtensions = false;
    for (auto& extension : m_model.extensionsRequired)
    if (!extensionIsSupported(extension)) {
        anyMissingRequiredExtensions = true;
        IGNIS_LOG("glTF", Error, "Model " << getFileName() << " requires unsupported extension " << extension);
    }

    return !anyMissingRequiredExtensions;
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
    // workout image formats
    std::vector<vk::Format> imageFormats(m_model.images.size(), vk::Format::eR8G8B8A8Unorm);

    for (auto& material : m_model.materials) {
        #define SET_IMAGE_FORMAT(index, format) if (index >= 0) { \
            uint32_t textureIndex = m_model.textures[index].source; \
            if (textureIndex >= 0) { \
                imageFormats[textureIndex] = vk::Format::format; \
                IGNIS_LOG("glTF", Debug, "Set image[" << textureIndex << "] format to " #format); \
            } \
        }

        SET_IMAGE_FORMAT(material.normalTexture.index, eR8G8B8A8Unorm);
        SET_IMAGE_FORMAT(material.emissiveTexture.index, eR8G8B8A8Srgb);
        SET_IMAGE_FORMAT(material.occlusionTexture.index, eR8G8B8A8Unorm);
        SET_IMAGE_FORMAT(material.pbrMetallicRoughness.baseColorTexture.index, eR8G8B8A8Srgb);
        SET_IMAGE_FORMAT(material.pbrMetallicRoughness.metallicRoughnessTexture.index, eR8G8B8A8Unorm);

        #undef SET_IMAGE_FORMAT
    }

    for (int i = 0; i < m_model.images.size(); i++) {
        auto& image = m_model.images[i];
        m_images.push_back(ImageBuilder { scope }
            .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setFormat(imageFormats[i])
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
    
    for (auto& material : m_model.materials) {
        m_materials.push_back(DescriptorSetBuilder { scope, materialPool }
            .addLayouts(s_materialLayout)
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
                .setImageView(texture.source < 0 ? s_nullImageView : m_imageViews[texture.source])
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
            .emissiveFactor = {
                material.emissiveFactor[0],
                material.emissiveFactor[1],
                material.emissiveFactor[2] },
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

bool GLTFModel::setupStatics(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout) {
    scope.addDeferredCleanupFunction([&]() {
        s_pipeline       = VK_NULL_HANDLE;
        s_pipelineLayout = VK_NULL_HANDLE;
        s_materialLayout = VK_NULL_HANDLE;
        s_backupPipeline = VK_NULL_HANDLE;
        s_nullImage      = {};
        s_nullImageView  = VK_NULL_HANDLE;
    });

    { // build null image
        uint8_t nullImageColor[] = { 1, 1, 1, 1 };

        s_nullImage = ImageBuilder { scope }
            .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setFormat(vk::Format::eR8G8B8A8Unorm)
            .load(nullImageColor, 1, 1);
        
        s_nullImageView = ImageViewBuilder { *s_nullImage, scope }
            .build();
    }

    { // build material layouts
        auto binding = vk::DescriptorSetLayoutBinding {}
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics);

        s_materialLayout = DescriptorLayoutBuilder { scope }
            .addBinding(binding.setBinding(0))
            .addBinding(binding.setBinding(1))
            .addBinding(binding.setBinding(2))
            .addBinding(binding.setBinding(3))
            .addBinding(binding.setBinding(4))
            .build();
    }

    { // build default pipeline
        s_pipelineLayout = PipelineLayoutBuilder { scope }
            .addSet(cameraUniformLayout)
            .addSet(s_materialLayout)
            .addPushConstantRange(vk::PushConstantRange {}
                .setSize(sizeof(MaterialData))
                .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics))
            .build();
        
        auto pipelineBuilder = GraphicsPipelineBuilder { s_pipelineLayout, scope }
            .addColorAttachmentFormat(IEngine::get().getVkbSwapchain().image_format)
            .setDepthAttachmentFormat(IEngine::get().getDepthBuffer()->getFormat())
            .addAttachmentBlendState()
            .setDepthStencilState()
            .addVertexAttribute<glm::vec3>(0, 0, 0) // POSITION
            .addVertexBinding<glm::vec3>(0)
            .addVertexAttribute<glm::vec2>(1, 1, 0) // TEXCOORD_0
            .addVertexBinding<glm::vec2>(1)
            .addVertexAttribute<glm::vec3>(2, 2, 0) // NORMAL
            .addVertexBinding<glm::vec3>(2)
            .addVertexAttribute<glm::vec4>(3, 3, 0) // TANGENT
            .addVertexBinding<glm::vec4>(3)
            .addVertexAttribute<glm::mat4>(4, 4, 0) // instance
            .addInstanceBinding<Instance>(4);
        
        try {
            pipelineBuilder
                .addStageFromFile("shaders/gltf.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
                .addStageFromFile("shaders/gltf.frag.spv", "main", vk::ShaderStageFlagBits::eFragment);
        } catch (std::runtime_error& e) {
            IGNIS_LOG("glTF", Error, "Error while loading glTF shader: " << e.what());
            return false;
        }

        auto pipelineResult = pipelineBuilder.build();

        if (pipelineResult.result != vk::Result::eSuccess) {
            IGNIS_LOG("glTF", Error, "Failed to create pipeline: " << pipelineResult.result);
            return false;
        }

        s_pipeline = pipelineResult.value;
    }

    { // build backup pipeline
        auto pipelineBuilder = GraphicsPipelineBuilder { s_pipelineLayout, scope }
            .addColorAttachmentFormat(IEngine::get().getVkbSwapchain().image_format)
            .setDepthAttachmentFormat(IEngine::get().getDepthBuffer()->getFormat())
            .addAttachmentBlendState()
            .setDepthStencilState()
            .addVertexAttribute<glm::vec3>(0, 0, 0) // POSITION
            .addVertexBinding<glm::vec3>(0)
            .addVertexAttribute<glm::vec2>(1, 1, 0) // TEXCOORD_0
            .addVertexBinding<glm::vec2>(1)
            .addVertexAttribute<glm::vec3>(2, 2, 0) // NORMAL
            .addVertexBinding<glm::vec3>(2)
            .addVertexAttribute<glm::mat4>(4, 3, 0) // instance
            .addInstanceBinding<Instance>(4);
        
        try {
            pipelineBuilder
                .addStageFromFile("shaders/gltf_backup.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
                .addStageFromFile("shaders/gltf_backup.frag.spv", "main", vk::ShaderStageFlagBits::eFragment);
        } catch (std::runtime_error& e) {
            IGNIS_LOG("glTF", Error, "Error while loading glTF shader: " << e.what());
            return false;
        }

        auto pipelineResult = pipelineBuilder.build();

        if (pipelineResult.result != vk::Result::eSuccess) {
            IGNIS_LOG("glTF", Error, "Failed to create pipeline: " << pipelineResult.result);
            return false;
        }

        s_backupPipeline = pipelineResult.value;
    }

    return true;
}

bool GLTFModel::setup(ResourceScope& scope, vk::DescriptorSetLayout cameraUniformLayout) {
    if (!s_pipeline) {
        IGNIS_LOG("glTF", Error, "Trying to setup glTF model before setting up glTF shared resources." <<
                                 "Call GLTFModel::setupPipelines() before setting up any models");
        
        m_status = Failed;
        return false;
    }

    vk::Device device = IEngine::get().getDevice();
    
    for (auto& s : m_oneFrameScopes) 
        scope.addDeferredCleanupFunction([&]() { s.executeDeferredCleanupFunctions(); });

    bool success = true
        && checkCompatibility()
        && setupBuffers(scope)
        && setupImages(scope)
        && setupSamplers(scope)
        && setupMaterials(scope);

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

bool GLTFModel::bindBuffer(vk::CommandBuffer cmd, gltf::Primitive primitive, const char* name, uint32_t binding) {
    auto it = primitive.attributes.find(name);

    if (it == primitive.attributes.end()) return false;

    auto& accessorIndex = it->second;
    auto& accessor = m_model.accessors[accessorIndex];
    auto& bufferView = m_model.bufferViews[accessor.bufferView];
    auto& buffer = m_buffers[bufferView.buffer];

    cmd.bindVertexBuffers(binding, *buffer, bufferView.byteOffset, {});

    return true;
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

    for (int meshID = 0; meshID < m_model.meshes.size(); meshID++) {
        auto& mesh = m_model.meshes[meshID];
        for (auto& primitive : mesh.primitives) {
            vk::DescriptorSet cameraDescriptorSet = camera.m_descriptorSets.getSet(IEngine::get().getInFlightIndex());

            bool positionFound = bindBuffer(cmd, primitive, "POSITION", 0);
            bool texcoordFound = bindBuffer(cmd, primitive, "TEXCOORD_0", 1);
            bool normalFound =   bindBuffer(cmd, primitive, "NORMAL", 2);
            bool tangentFound =  bindBuffer(cmd, primitive, "TANGENT", 3);
            cmd.bindVertexBuffers(4, *instanceBuffers[meshID], { 0 }, {});

            bool useBackupPipeline = !tangentFound;

            vk::Pipeline& pipeline = useBackupPipeline ? s_backupPipeline : s_pipeline;

            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_pipelineLayout, 0, cameraDescriptorSet, {});
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, s_pipelineLayout, 1, m_materials[primitive.material].getSet(), {});
            cmd.pushConstants<MaterialData>(s_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, m_materialStructs[primitive.material]);

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
