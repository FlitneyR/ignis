#include <pipelineBuilder.hpp>
#include <resourceScope.hpp>
#include <fstream>

namespace ignis {

PipelineLayoutBuilder::PipelineLayoutBuilder(
    ResourceScope& scope
) : IBuilder(scope)
{}

PipelineLayoutBuilder& PipelineLayoutBuilder::addSet(
    vk::DescriptorSetLayout setLayout
) {
    m_setLayouts.push_back(setLayout);
    return *this;
}

PipelineLayoutBuilder& PipelineLayoutBuilder::addPushConstantRange(
    vk::PushConstantRange pcRange
) {
    m_pushConstantRanges.push_back(pcRange);
    return *this;
}

vk::PipelineLayout PipelineLayoutBuilder::build() {
    vk::PipelineLayout layout = getDevice().createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_setLayouts)
        .setPushConstantRanges(m_pushConstantRanges)
        );
    
    r_scope.addDeferredCleanupFunction([=, device = getDevice()]() {
        device.destroyPipelineLayout(layout);
    });
    
    return layout;
}

IPipelineBuilder::IPipelineBuilder(
    ResourceScope& scope
) : IBuilder(scope)
{}

IPipelineBuilder::IPipelineBuilder(
    vk::PipelineLayout layout,
    ResourceScope& scope
) : IBuilder(scope),
    m_layout(layout)
{}

vk::ShaderModule IPipelineBuilder::loadShaderModule(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader code file: " + std::string { filename });
    
    size_t filesize = file.tellg();
    std::vector<char> code(filesize);
    file.seekg(0);
    file.read(code.data(), filesize);

    vk::ShaderModule shaderModule = getDevice().createShaderModule(vk::ShaderModuleCreateInfo {}
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<uint32_t*>(code.data()))
        );
    
    r_scope.addDeferredCleanupFunction([=, device = getDevice()]() {
        device.destroyShaderModule(shaderModule);
    });

    return shaderModule;
}

ComputePipelineBuilder::ComputePipelineBuilder(
    ResourceScope& scope
) : IPipelineBuilder(scope)
{}

ComputePipelineBuilder::ComputePipelineBuilder(
    vk::PipelineLayout layout,
    ResourceScope& scope
) : IPipelineBuilder(layout, scope)
{}

ComputePipelineBuilder& ComputePipelineBuilder::setPipelineLayout(vk::PipelineLayout pipelineLayout) {
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setShaderModule(vk::ShaderModule shaderModule) {
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setFunctionName(const char* functionName) {
    return *this;
}

vk::ResultValue<PipelineData> ComputePipelineBuilder::build() {
    PipelineData ret { .layout = m_layout };

    auto pipeline_result = getDevice().createComputePipeline(nullptr, vk::ComputePipelineCreateInfo {}
        .setLayout(m_layout)
        .setStage(vk::PipelineShaderStageCreateInfo {}
            .setModule(m_shaderModule)
            .setPName(m_functionName.c_str())
            .setStage(vk::ShaderStageFlagBits::eCompute)));
    
    if (pipeline_result.result != vk::Result::eSuccess)
        return vk::ResultValue<PipelineData> { pipeline_result.result, ret };

    r_scope.addDeferredCleanupFunction([=, device = getDevice(), pipeline = pipeline_result.value ] {
        device.destroyPipeline(pipeline);
    });

    ret.pipeline = pipeline_result.value;

    return vk::ResultValue<PipelineData> { vk::Result::eSuccess, ret };
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    ResourceScope& scope
) : IPipelineBuilder(scope)
{
    useDefaults();
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    vk::PipelineLayout pipelineLayout,
    ResourceScope& scope
) : IPipelineBuilder(pipelineLayout, scope)
{
    useDefaults();
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::useDefaults() {
    m_dynamicStates = {};
    m_dynamicStates.push_back(vk::DynamicState::eViewport);
    m_dynamicState = vk::PipelineDynamicStateCreateInfo {};
    
    m_vertexInputState = vk::PipelineVertexInputStateCreateInfo {};
    
    m_inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        ;
    
    m_depthStencilState = std::nullopt;
    m_tessellationState = std::nullopt;

    m_rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.f)
        ;
    
    m_multisampleState = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        ;
    
    m_attachmentBlendStates = {};
    m_colorBlendState = vk::PipelineColorBlendStateCreateInfo {};

    m_viewports = { vk::Viewport {} };
    m_scissors = { vk::Rect2D {} };
    m_viewportState = vk::PipelineViewportStateCreateInfo {};

    m_colorAttachmentFormats = {};
    m_renderingCreateInfo = vk::PipelineRenderingCreateInfoKHR {};
    
    return *this;
}

vk::ResultValue<PipelineData> GraphicsPipelineBuilder::build() {
    PipelineData ret { .layout = m_layout };

    m_dynamicState
        .setDynamicStates(m_dynamicStates);

    m_viewportState
        .setViewports(m_viewports)
        .setScissors(m_scissors);
    
    m_vertexInputState
        .setVertexAttributeDescriptions(m_vertexAttributes)
        .setVertexBindingDescriptions(m_vertexBindings);

    m_colorBlendState
        .setAttachments(m_attachmentBlendStates);
    
    m_renderingCreateInfo
        .setColorAttachmentFormats(m_colorAttachmentFormats);

    auto createInfo = vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_layout)
        .setStages(m_stages)
        .setPDynamicState(&m_dynamicState)
        .setPVertexInputState(&m_vertexInputState)
        .setPInputAssemblyState(&m_inputAssemblyState)
        .setPDepthStencilState(m_depthStencilState ? &*m_depthStencilState : nullptr)
        .setPTessellationState(m_tessellationState ? &*m_tessellationState : nullptr)
        .setPRasterizationState(&m_rasterizationState)
        .setPMultisampleState(&m_multisampleState)
        .setPColorBlendState(&m_colorBlendState)
        .setPViewportState(&m_viewportState)
        .setPNext(&m_renderingCreateInfo);

    vk::ResultValue<vk::Pipeline> pipelineResult = getDevice().createGraphicsPipeline(
        VK_NULL_HANDLE,
        createInfo
    );

    if (pipelineResult.result != vk::Result::eSuccess)
        return vk::ResultValue<PipelineData> { pipelineResult.result, ret };

    r_scope.addDeferredCleanupFunction([=, device = getDevice(), pipeline = pipelineResult.value]() {
        device.destroyPipeline(pipeline);
    });

    ret.pipeline = pipelineResult.value;

    return vk::ResultValue<PipelineData> { vk::Result::eSuccess, ret };
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPipelineLayout(vk::PipelineLayout pl) {
    m_layout = pl;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addDynamicState(vk::DynamicState ds) {
    m_dynamicStates.push_back(ds);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputState(vk::PipelineVertexInputStateCreateInfo vis) {
    m_vertexInputState = vis;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute(vk::VertexInputAttributeDescription attribute) {
    m_vertexAttributes.push_back(attribute);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, vk::Format format) {
    return addVertexAttribute(vk::VertexInputAttributeDescription {}
        .setBinding(binding)
        .setLocation(location)
        .setOffset(offset)
        .setFormat(format));
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<float>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute(binding, location, offset, vk::Format::eR32Sfloat);
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<glm::vec2>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute(binding, location, offset, vk::Format::eR32G32Sfloat);
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<glm::vec3>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute(binding, location, offset, vk::Format::eR32G32B32Sfloat);
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<glm::vec4>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute(binding, location, offset, vk::Format::eR32G32B32A32Sfloat);
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<glm::mat3>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute<glm::vec3>(binding, location + 0, offset + 0 * sizeof(glm::vec3))
          .addVertexAttribute<glm::vec3>(binding, location + 1, offset + 1 * sizeof(glm::vec3))
          .addVertexAttribute<glm::vec3>(binding, location + 2, offset + 2 * sizeof(glm::vec3));
}

template<>
GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute<glm::mat4>(uint32_t binding, uint32_t location, uint32_t offset) {
    return addVertexAttribute<glm::vec4>(binding, location + 0, offset + 0 * sizeof(glm::vec4))
          .addVertexAttribute<glm::vec4>(binding, location + 1, offset + 1 * sizeof(glm::vec4))
          .addVertexAttribute<glm::vec4>(binding, location + 2, offset + 2 * sizeof(glm::vec4))
          .addVertexAttribute<glm::vec4>(binding, location + 3, offset + 3 * sizeof(glm::vec4));
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexBinding(vk::VertexInputBindingDescription binding) {
    m_vertexBindings.push_back(binding);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexBinding(uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate) {
    return addVertexBinding(vk::VertexInputBindingDescription {}
        .setBinding(binding)
        .setStride(stride)
        .setInputRate(inputRate));
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addInstanceBinding(uint32_t binding, uint32_t stride) {
    return addVertexBinding(binding, stride, vk::VertexInputRate::eInstance);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setInputAssemblyState(vk::PipelineInputAssemblyStateCreateInfo ias) {
    m_inputAssemblyState = ias;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthStencilState(vk::PipelineDepthStencilStateCreateInfo dss) {
    m_depthStencilState = dss;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setTessellationState(std::optional<vk::PipelineTessellationStateCreateInfo> ts) {
    m_tessellationState = ts;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRasterizationState(vk::PipelineRasterizationStateCreateInfo rs) {
    m_rasterizationState = rs;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addAttachmentBlendState(vk::PipelineColorBlendAttachmentState abs) {
    m_attachmentBlendStates.push_back(abs);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addScissor(vk::Rect2D scissor) {
    m_scissors.push_back(scissor);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addViewport(vk::Viewport viewport) {
    m_viewports.push_back(viewport);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addStage(vk::PipelineShaderStageCreateInfo stage) {
    m_stages.push_back(stage);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addStageFromFile(const char* filename, const char* funcName, vk::ShaderStageFlagBits shaderStage) {
    m_modules.push_back(loadShaderModule(filename));

    return addStage(vk::PipelineShaderStageCreateInfo {}
        .setModule(m_modules.back())
        .setPName(funcName)
        .setStage(shaderStage));
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderingCreateInfo(vk::PipelineRenderingCreateInfoKHR rci) {
    m_renderingCreateInfo = rci;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addColorAttachmentFormat(vk::Format format) {
    m_colorAttachmentFormats.push_back(format);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthAttachmentFormat(vk::Format format) {
    m_renderingCreateInfo.setDepthAttachmentFormat(format);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addColorAttachmentFormat(VkFormat format) {
    return addColorAttachmentFormat(static_cast<vk::Format>(format));
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthAttachmentFormat(VkFormat format) {
    return setDepthAttachmentFormat(static_cast<vk::Format>(format));
}

}
