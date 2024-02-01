#include <pipelineBuilder.hpp>
#include <resourceScope.hpp>
#include <fstream>

namespace ignis {

PipelineLayoutBuilder::PipelineLayoutBuilder(
    vk::Device device,
    ResourceScope* scope
) : IBuilder(device, scope)
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
    vk::PipelineLayout layout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_setLayouts)
        .setPushConstantRanges(m_pushConstantRanges)
        );
    
    if (m_scope) m_scope->addDeferredCleanupFunction([=, device = m_device]() {
        device.destroyPipelineLayout(layout);
    });
    
    return layout;
}

IPipelineBuilder::IPipelineBuilder(
    vk::Device device,
    ResourceScope* scope
) : IBuilder(device, scope)
{}

IPipelineBuilder::IPipelineBuilder(
    vk::Device device,
    vk::PipelineLayout layout,
    ResourceScope* scope
) : IBuilder(device, scope),
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

    vk::ShaderModule shaderModule = m_device.createShaderModule(vk::ShaderModuleCreateInfo {}
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<uint32_t*>(code.data()))
        );
    
    if (m_scope) m_scope->addDeferredCleanupFunction([=, device = m_device]() {
        device.destroyShaderModule(shaderModule);
    });

    return shaderModule;
}

ComputePipelineBuilder::ComputePipelineBuilder(
    vk::Device device,
    ResourceScope* scope
) : IPipelineBuilder(device, scope)
{}

ComputePipelineBuilder::ComputePipelineBuilder(
    vk::Device device,
    vk::PipelineLayout layout,
    ResourceScope* scope
) : IPipelineBuilder(device, layout, scope)
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
    PipelineData data;

    auto pipeline_result = m_device.createComputePipeline(nullptr, vk::ComputePipelineCreateInfo {}
        .setLayout(m_layout)
        .setStage(vk::PipelineShaderStageCreateInfo {}
            .setModule(m_shaderModule)
            .setPName(m_functionName.c_str())
            .setStage(vk::ShaderStageFlagBits::eCompute)));
    
    if (pipeline_result.result != vk::Result::eSuccess)
        vk::ResultValue<PipelineData> { pipeline_result.result, data };
    
    data.pipeline = pipeline_result.value;
    data.pipelineLayout = m_layout;
    data.shaderModules = m_modules;

    if (m_scope) m_scope->addDeferredCleanupFunction([=, device = m_device, pipeline = data.pipeline ] {
        device.destroyPipeline(pipeline);
    });

    return vk::ResultValue<PipelineData> { vk::Result::eSuccess, data };
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    vk::Device device,
    ResourceScope* scope
) : IPipelineBuilder(device, scope)
{
    useDefaults();
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    vk::Device device,
    vk::PipelineLayout pipelineLayout,
    ResourceScope* scope
) : IPipelineBuilder(device, pipelineLayout, scope)
{
    useDefaults();
}

vk::PipelineColorBlendAttachmentState GraphicsPipelineBuilder::defaultAttachmentBlendState() {
    return vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(true)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::useDefaults() {
    m_dynamicStates = {};
    m_dynamicStates.push_back(vk::DynamicState::eViewport);
    m_dynamicState = vk::PipelineDynamicStateCreateInfo {};
    
    m_vertexInputState = vk::PipelineVertexInputStateCreateInfo {};
    
    inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        ;
    
    m_depthStencilState = std::nullopt;
    m_tessellationState = std::nullopt;

    m_rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.f)
        ;
    
    m_multisampleState = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        ;
    
    m_attachmentBlendStates = {};
    m_colorBlendState = vk::PipelineColorBlendStateCreateInfo {};

    m_viewports = {};
    m_scissors = {};
    m_viewportState = vk::PipelineViewportStateCreateInfo {};

    m_colorAttachmentFormats = {};
    m_renderingCreateInfo = vk::PipelineRenderingCreateInfoKHR {};
    
    return *this;
}

vk::ResultValue<PipelineData> GraphicsPipelineBuilder::build() {
    m_dynamicState
        .setDynamicStates(m_dynamicStates)
        ;

    m_viewportState
        .setViewports(m_viewports)
        .setScissors(m_scissors)
        ;
    
    m_vertexInputState
        .setVertexAttributeDescriptions(m_vertexAttributes)
        .setVertexBindingDescriptions(m_vertexBindings)
        ;

    m_colorBlendState
        .setAttachments(m_attachmentBlendStates)
        ;
    
    m_renderingCreateInfo
        .setColorAttachmentFormats(m_colorAttachmentFormats)
        ;

    auto createInfo = vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_layout)
        .setStages(m_stages)
        .setPDynamicState(&m_dynamicState)
        .setPVertexInputState(&m_vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPDepthStencilState(m_depthStencilState ? &*m_depthStencilState : nullptr)
        .setPTessellationState(m_tessellationState ? &*m_tessellationState : nullptr)
        .setPRasterizationState(&m_rasterizationState)
        .setPMultisampleState(&m_multisampleState)
        .setPColorBlendState(&m_colorBlendState)
        .setPViewportState(&m_viewportState)
        .setPNext(&m_renderingCreateInfo)
        ;
    
    PipelineData ret;

    vk::ResultValue<vk::Pipeline> pipelineResult = m_device.createGraphicsPipeline(
        VK_NULL_HANDLE,
        createInfo
    );

    if (pipelineResult.result != vk::Result::eSuccess)
        return vk::ResultValue<PipelineData> { pipelineResult.result, ret };

    ret.pipeline = pipelineResult.value;
    ret.shaderModules = m_modules;
    ret.pipelineLayout = m_layout;

    if (m_scope) m_scope->addDeferredCleanupFunction([=, device = m_device, pipeline = ret.pipeline]() {
        device.destroyPipeline(pipeline);
    });

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

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexBinding(vk::VertexInputBindingDescription binding) {
    m_vertexBindings.push_back(binding);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setInputAssemblyState(vk::PipelineInputAssemblyStateCreateInfo ias) {
    inputAssemblyState = ias;
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

}
