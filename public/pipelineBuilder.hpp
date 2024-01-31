#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

namespace ignis {

class PipelineLayoutBuilder {
    vk::Device device;

public:
    std::vector<vk::DescriptorSetLayout> setLayouts {};
    std::vector<vk::PushConstantRange> pushConstantRanges {};

    PipelineLayoutBuilder(vk::Device device);

    PipelineLayoutBuilder& addSet(vk::DescriptorSetLayout setLayout);
    PipelineLayoutBuilder& addPushConstantRange(vk::PushConstantRange pcRange);

    vk::PipelineLayout build();
};

class GraphicsPipelineBuilder {
    vk::Device device;

public:
    vk::PipelineLayout pipelineLayout;

    std::vector<vk::DynamicState> dynamicStates {};
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    vk::PipelineVertexInputStateCreateInfo vertexInputState {};
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState {};
    std::optional<vk::PipelineDepthStencilStateCreateInfo> depthStencilState {};
    std::optional<vk::PipelineTessellationStateCreateInfo> tessellationState {};
    vk::PipelineRasterizationStateCreateInfo rasterizationState {};
    vk::PipelineMultisampleStateCreateInfo multisampleState;
    std::vector<vk::PipelineColorBlendAttachmentState> attachmentBlendStates;
    vk::PipelineColorBlendStateCreateInfo colorBlendState {};
    std::vector<vk::Format> colorAttachmentFormats {};
    vk::PipelineRenderingCreateInfoKHR renderingCreateInfo {};
    std::vector<vk::Rect2D> scissors {};
    std::vector<vk::Viewport> viewports {};
    vk::PipelineViewportStateCreateInfo viewportState {};
    std::vector<vk::ShaderModule> modules;
    std::vector<vk::PipelineShaderStageCreateInfo> stages {};

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

    vk::ShaderModule loadShaderModule(const char* filename);

    GraphicsPipelineBuilder& modify(std::function<void(GraphicsPipelineBuilder&)> func) { func(*this); return *this; }

    struct PipelineData {
        std::vector<vk::ShaderModule> shaderModules;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
    };

    vk::ResultValue<PipelineData> build();
};

}
