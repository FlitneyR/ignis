#include <pipelineBuilder.hpp>
#include <fstream>

namespace ignis {

PipelineLayoutBuilder::PipelineLayoutBuilder(
    vk::Device device
) : device(device)
{}

PipelineLayoutBuilder& PipelineLayoutBuilder::addSet(
    vk::DescriptorSetLayout setLayout
) {
    setLayouts.push_back(setLayout);
    return *this;
}

PipelineLayoutBuilder& PipelineLayoutBuilder::addPushConstantRange(
    vk::PushConstantRange pcRange
) {
    pushConstantRanges.push_back(pcRange);
    return *this;
}

vk::PipelineLayout PipelineLayoutBuilder::build() {
    return device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(setLayouts)
        .setPushConstantRanges(pushConstantRanges)
        );
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(
    vk::Device device,
    vk::PipelineLayout pipelineLayout
) : device(device),
    pipelineLayout(pipelineLayout)
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
    dynamicStates = {};
    dynamicStates.push_back(vk::DynamicState::eViewport);
    dynamicState = vk::PipelineDynamicStateCreateInfo {};
    
    vertexInputState = vk::PipelineVertexInputStateCreateInfo {};
    
    inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        ;
    
    depthStencilState = std::nullopt;
    tessellationState = std::nullopt;

    rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.f)
        ;
    
    multisampleState = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        ;
    
    attachmentBlendStates = {};
    colorBlendState = vk::PipelineColorBlendStateCreateInfo {};

    viewports = {};
    scissors = {};
    viewportState = vk::PipelineViewportStateCreateInfo {};

    colorAttachmentFormats = {};
    renderingCreateInfo = vk::PipelineRenderingCreateInfoKHR {};
    
    return *this;
}

vk::ResultValue<GraphicsPipelineBuilder::PipelineData> GraphicsPipelineBuilder::build() {
    dynamicState
        .setDynamicStates(dynamicStates)
        ;

    viewportState
        .setViewports(viewports)
        .setScissors(scissors)
        ;
    
    vertexInputState
        .setVertexAttributeDescriptions(vertexAttributes)
        .setVertexBindingDescriptions(vertexBindings)
        ;

    colorBlendState
        .setAttachments(attachmentBlendStates)
        ;
    
    renderingCreateInfo
        .setColorAttachmentFormats(colorAttachmentFormats)
        ;

    auto createInfo = vk::GraphicsPipelineCreateInfo {}
        .setLayout(pipelineLayout)
        .setStages(stages)
        .setPDynamicState(&dynamicState)
        .setPVertexInputState(&vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPDepthStencilState(depthStencilState ? &*depthStencilState : nullptr)
        .setPTessellationState(tessellationState ? &*tessellationState : nullptr)
        .setPRasterizationState(&rasterizationState)
        .setPMultisampleState(&multisampleState)
        .setPColorBlendState(&colorBlendState)
        .setPViewportState(&viewportState)
        .setPNext(&renderingCreateInfo)
        ;
    
    PipelineData ret;

    vk::ResultValue<vk::Pipeline> pipelineResult = device.createGraphicsPipeline(
        VK_NULL_HANDLE,
        createInfo
    );

    if (pipelineResult.result != vk::Result::eSuccess)
        return vk::ResultValue<PipelineData> { pipelineResult.result, ret };

    ret.pipeline = pipelineResult.value;
    ret.shaderModules = modules;
    ret.pipelineLayout = pipelineLayout;

    return vk::ResultValue<PipelineData> { vk::Result::eSuccess, ret };
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPipelineLayout(vk::PipelineLayout pl) {
    pipelineLayout = pl;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addDynamicState(vk::DynamicState ds) {
    dynamicStates.push_back(ds);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputState(vk::PipelineVertexInputStateCreateInfo vis) {
    vertexInputState = vis;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute(vk::VertexInputAttributeDescription attribute) {
    vertexAttributes.push_back(attribute);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexBinding(vk::VertexInputBindingDescription binding) {
    vertexBindings.push_back(binding);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setInputAssemblyState(vk::PipelineInputAssemblyStateCreateInfo ias) {
    inputAssemblyState = ias;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthStencilState(vk::PipelineDepthStencilStateCreateInfo dss) {
    depthStencilState = dss;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setTessellationState(std::optional<vk::PipelineTessellationStateCreateInfo> ts) {
    tessellationState = ts;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRasterizationState(vk::PipelineRasterizationStateCreateInfo rs) {
    rasterizationState = rs;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addAttachmentBlendState(vk::PipelineColorBlendAttachmentState abs) {
    attachmentBlendStates.push_back(abs);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addScissor(vk::Rect2D scissor) {
    scissors.push_back(scissor);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addViewport(vk::Viewport viewport) {
    viewports.push_back(viewport);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addStage(vk::PipelineShaderStageCreateInfo stage) {
    stages.push_back(stage);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addStageFromFile(const char* filename, const char* funcName, vk::ShaderStageFlagBits shaderStage) {
    modules.push_back(loadShaderModule(filename));

    return addStage(vk::PipelineShaderStageCreateInfo {}
        .setModule(modules.back())
        .setPName(funcName)
        .setStage(shaderStage));
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderingCreateInfo(vk::PipelineRenderingCreateInfoKHR rci) {
    renderingCreateInfo = rci;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addColorAttachmentFormat(vk::Format format) {
    colorAttachmentFormats.push_back(format);
    return *this;
}

vk::ShaderModule GraphicsPipelineBuilder::loadShaderModule(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader code file: " + std::string { filename });
    
    size_t filesize = file.tellg();
    std::vector<char> code(filesize);
    file.seekg(0);
    file.read(code.data(), filesize);

    return device.createShaderModule(vk::ShaderModuleCreateInfo {}
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<uint32_t*>(code.data()))
        );
}

}
