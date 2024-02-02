#pragma once

#include "libraries.hpp"
#include "resourceScope.hpp"
#include "builder.hpp"
#include <optional>

namespace ignis {

class PipelineLayoutBuilder : IBuilder<vk::PipelineLayout> {
public:
    std::vector<vk::DescriptorSetLayout> m_setLayouts {};
    std::vector<vk::PushConstantRange> m_pushConstantRanges {};

    PipelineLayoutBuilder(ResourceScope& scope);

    PipelineLayoutBuilder& addSet(vk::DescriptorSetLayout setLayout);
    PipelineLayoutBuilder& addPushConstantRange(vk::PushConstantRange pcRange);

    vk::PipelineLayout build();
};

class IPipelineBuilder : public IBuilder<vk::ResultValue<vk::Pipeline>> {
protected:
    vk::PipelineLayout m_layout;
    std::vector<vk::ShaderModule> m_modules;

public:
    IPipelineBuilder(ResourceScope& scope);
    IPipelineBuilder(vk::PipelineLayout layout, ResourceScope& scope);

    vk::ShaderModule loadShaderModule(const char* filename);
};

class ComputePipelineBuilder : public IPipelineBuilder {
public:
    std::string m_functionName;
    vk::ShaderModule m_shaderModule;

    ComputePipelineBuilder(ResourceScope& scope);
    ComputePipelineBuilder(vk::PipelineLayout layout, ResourceScope& scope);

    ComputePipelineBuilder& setPipelineLayout(vk::PipelineLayout pipelineLayout);
    ComputePipelineBuilder& setShaderModule(vk::ShaderModule shaderModule);
    ComputePipelineBuilder& setSHaderModule(const char* filename);
    ComputePipelineBuilder& setFunctionName(const char* functionName);

    ComputePipelineBuilder& modify(std::function<void(ComputePipelineBuilder&)> func) { func(*this); return *this; }

    vk::ResultValue<vk::Pipeline> build() override;
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

    GraphicsPipelineBuilder(ResourceScope& scope);
    GraphicsPipelineBuilder(vk::PipelineLayout layout, ResourceScope& scope);

    static vk::PipelineColorBlendAttachmentState   defaultAttachmentBlendState();
    static vk::PipelineDepthStencilStateCreateInfo defaultDepthStencilState();

    GraphicsPipelineBuilder& useDefaults();

    GraphicsPipelineBuilder& setPipelineLayout(vk::PipelineLayout pl);
    GraphicsPipelineBuilder& addDynamicState(vk::DynamicState ds);
    GraphicsPipelineBuilder& setVertexInputState(vk::PipelineVertexInputStateCreateInfo vis);
    GraphicsPipelineBuilder& addVertexBinding(vk::VertexInputBindingDescription binding);
    GraphicsPipelineBuilder& addVertexBinding(uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex);
    GraphicsPipelineBuilder& addInstanceBinding(uint32_t binding, uint32_t stride);
    GraphicsPipelineBuilder& addVertexAttribute(vk::VertexInputAttributeDescription attribute);
    GraphicsPipelineBuilder& addVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset, vk::Format format);
    template<typename T>
    GraphicsPipelineBuilder& addVertexAttribute(uint32_t binding, uint32_t location, uint32_t offset);
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
    GraphicsPipelineBuilder& setDepthAttachmentFormat(vk::Format format);
    GraphicsPipelineBuilder& addColorAttachmentFormat(VkFormat format);
    GraphicsPipelineBuilder& setDepthAttachmentFormat(VkFormat format);

    template<typename T>
    GraphicsPipelineBuilder& addVertexBinding(uint32_t binding, vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex) {
        return addVertexBinding(binding, sizeof(T), inputRate);
    }

    template<typename T>
    GraphicsPipelineBuilder& addInstanceBinding(uint32_t binding) {
        return addInstanceBinding(binding, sizeof(T));
    }

    GraphicsPipelineBuilder& modify(std::function<void(GraphicsPipelineBuilder&)> func) { func(*this); return *this; }

    vk::ResultValue<vk::Pipeline> build() override;
};

}
