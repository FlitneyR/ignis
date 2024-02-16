#pragma once

#include "pipelineBuilder.hpp"
#include "libraries.hpp"
#include "uniform.hpp"
#include "image.hpp"

namespace ignis {

class BloomPostProcess {
    Uniform m_emissiveUniform;
    Uniform m_hBlurUniform;
    Uniform m_vBlurUniform;

    PipelineData m_pipeline;

    Allocated<Image> m_hBlurChain;
    Allocated<Image> m_vBlurChain;

    std::vector<vk::ImageView> m_hBlurChainViews;
    std::vector<vk::ImageView> m_vBlurChainViews;

    void filterHighlights(vk::CommandBuffer cmd);
    void blurDownMipChain(vk::CommandBuffer cmd);
    void overlayUpMipChain(vk::CommandBuffer cmd);

    struct PassConfig {
        enum Operation : int {
            Filter = 0, Blur, Overlay
        };

        enum Direction : int {
            Horizontal = 0, Vertical
        };

        Operation operation;
        int       sourceMipLevel;
        Direction direction;

        float     clipping;
        float     dispersion;
        float     mixing;
    };

    void beginRenderPass(
        vk::CommandBuffer cmd,
        Image& image, vk::ImageView imageView,
        uint32_t mipLevel = 0,
        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear);

public:
    float clipping   = 1.0f;
    float dispersion = 1.0f;
    float mixing     = 0.5f;

    bool setup(ResourceScope& scope);
    void draw(vk::CommandBuffer cmd);
};

}
