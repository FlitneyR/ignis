#pragma once

#include "libraries.hpp"
#include "resourceScope.hpp"
#include <optional>

namespace ignis {

class PipelineLayoutBuilder {
    vk::Device m_device;

public:
    std::vector<vk::DescriptorSetLayout> m_setLayouts {};
    std::vector<vk::PushConstantRange> m_pushConstantRanges {};

    PipelineLayoutBuilder(vk::Device device);

    PipelineLayoutBuilder& addSet(vk::DescriptorSetLayout setLayout);
    PipelineLayoutBuilder& addPushConstantRange(vk::PushConstantRange pcRange);

    vk::ShaderModule loadShaderModule(const char* filename);

    vk::PipelineLayout build();
};

struct PipelineData {
    std::vector<vk::ShaderModule> shaderModules;
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
};

class IPipelineBuilder {
protected:
    vk::Device m_device;
    vk::PipelineLayout m_layout;
    std::vector<vk::ShaderModule> m_modules;

public:
    IPipelineBuilder(vk::Device device);
    IPipelineBuilder(vk::Device device, vk::PipelineLayout layout);

    vk::ShaderModule loadShaderModule(const char* filename);

    virtual vk::ResultValue<PipelineData> build() = 0;
};

class ComputePipelineBuilder : public IPipelineBuilder {
public:
    std::string m_functionName;
    vk::ShaderModule m_shaderModule;

    ComputePipelineBuilder(vk::Device device);
    ComputePipelineBuilder(vk::Device device, vk::PipelineLayout layout);

    ComputePipelineBuilder& setPipelineLayout(vk::PipelineLayout pipelineLayout);
    ComputePipelineBuilder& setShaderModule(vk::ShaderModule shaderModule);
    ComputePipelineBuilder& setSHaderModule(const char* filename);
    ComputePipelineBuilder& setFunctionName(const char* functionName);

    ComputePipelineBuilder& modify(std::function<void(ComputePipelineBuilder&)> func) { func(*this); return *this; }

    vk::ResultValue<PipelineData> build() override;
};

class GraphicsPipelineBuilder : public IPipelineBuilder {
public:
    std::vector<vk::DynamicState> m_dynamicStates {};
    vk::PipelineDynamicStateCreateInfo m_dynamicState {};
    std::vector<vk::VertexInputAttributeDescription> m_vertexAttributes;
    std::vector<vk::VertexInputBindingDescription> m_vertexBindings;
    vk::PipelineVertexInputStateCreateInfo m_vertexInputState {};
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState {};
    std::optional<vk::PipelineDepthStencilStateCreateInfo> m_depthStencilState {};
    std::optional<vk::PipelineTessellationStateCreateInfo> m_tessellationState {};
    vk::PipelineRasterizationStateCreateInfo m_rasterizationState {};
    vk::PipelineMultisampleStateCreateInfo m_multisampleState;
    std::vector<vk::PipelineColorBlendAttachmentState> m_attachmentBlendStates;
    vk::PipelineColorBlendStateCreateInfo m_colorBlendState {};
    std::vector<vk::Format> m_colorAttachmentFormats {};
    vk::PipelineRenderingCreateInfoKHR m_renderingCreateInfo {};
    std::vector<vk::Rect2D> m_scissors {};
    std::vector<vk::Viewport> m_viewports {};
    vk::PipelineViewportStateCreateInfo m_viewportState {};
    std::vector<vk::PipelineShaderStageCreateInfo> m_stages {};

    GraphicsPipelineBuilder(vk::Device device);
    GraphicsPipelineBuilder(vk::Device device, vk::PipelineLayout);

    static vk::PipelineColorBlendAttachmentState defaultAttachmentBlendState();

    GraphicsPipelineBuilder& useDefaults();

    GraphicsPipelineBuilder& setPipelineLayout(vk::PipelineLayout pl);
    GraphicsPipelineBuilder& addDynamicState(vk::DynamicState ds);
    GraphicsPipelineBuilder& setVertexInputState(vk::PipelineVertexInputStateCreateInfo vis);
    GraphicsPipelineBuilder& addVertexAttribute(vk::VertexInputAttributeDescription attribute);
    GraphicsPipelineBuilder& addVertexBinding(vk::VertexInputBindingDescription binding);
    GraphicsPipelineBuilder& setInputAssemblyState(vk::PipelineInputAssemblyStateCreateInfo ias);
    GraphicsPipelineBuilder& setDepthStencilState(vk::PipelineDepthStencilStateCreateInfo dss);
    GraphicsPipelineBuilder& setTessellationState(std::optional<vk::PipelineTessellationStateCreateInfo> ts);
    GraphicsPipelineBuilder& setRasterizationState(vk::PipelineRasterizationStateCreateInfo rs);
    GraphicsPipelineBuilder& addAttachmentBlendState(vk::PipelineColorBlendAttachmentState abs);
    GraphicsPipelineBuilder& addScissor(vk::Rect2D scissor);
    GraphicsPipelineBuilder& addViewport(vk::Viewport viewport);
    GraphicsPipelineBuilder& addStage(vk::PipelineShaderStageCreateInfo stage);
    GraphicsPipelineBuilder& addStageFromFile(const char* filename, const char* funcName, vk::ShaderStageFlagBits shaderStage);
    GraphicsPipelineBuilder& setRenderingCreateInfo(vk::PipelineRenderingCreateInfoKHR rci);
    GraphicsPipelineBuilder& addColorAttachmentFormat(vk::Format format);

    GraphicsPipelineBuilder& modify(std::function<void(GraphicsPipelineBuilder&)> func) { func(*this); return *this; }

    vk::ResultValue<PipelineData> build() override;
};

}
