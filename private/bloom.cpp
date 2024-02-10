#include "bloom.hpp"
#include "engine.hpp"
#include "uniformBuilder.hpp"

namespace ignis {

bool BloomPostProcess::setup(ResourceScope& scope) {
    auto& engine = IEngine::get();
    vk::Device device = engine.getDevice();
    IEngine::GBuffer& gBuffer = engine.getGBuffer();

    auto blurChainImageBuilder = ImageBuilder { scope }
        .setFormat(gBuffer.emissiveImage->getFormat())
        .setSize(gBuffer.emissiveImage->getSize())
        .setMipLevelCount(9)
        .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    m_hBlurChain = blurChainImageBuilder.build();
    m_vBlurChain = blurChainImageBuilder.build();

    for (int i = 0; i < m_hBlurChain->getMipLevelCount(); i++)
        m_hBlurChainViews.push_back(ImageViewBuilder { *m_hBlurChain, scope }
            .setMipLevelRange(i, 1)
            .build());

    for (int i = 0; i < m_vBlurChain->getMipLevelCount(); i++)
        m_vBlurChainViews.push_back(ImageViewBuilder { *m_vBlurChain, scope }
            .setMipLevelRange(i, 1)
            .build());

    uint32_t setCount = 1 + m_hBlurChain->getMipLevelCount() + m_vBlurChain->getMipLevelCount();

    auto pool = DescriptorPoolBuilder { scope }
        .setMaxSetCount(setCount)
        .addPoolSize({ vk::DescriptorType::eCombinedImageSampler, setCount })
        .build();
    
    auto uniformLayout = DescriptorLayoutBuilder { scope }
        .addBinding(vk::DescriptorSetLayoutBinding {}
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment))
        .build();
    
    m_emissiveUniform = UniformBuilder { scope }
        .setPool(pool)
        .addLayouts(uniformLayout)
        .build();

    m_hBlurUniform = UniformBuilder { scope }
        .setPool(pool)
        .addLayouts(uniformLayout, m_hBlurChain->getMipLevelCount())
        .build();

    m_vBlurUniform = UniformBuilder { scope }
        .setPool(pool)
        .addLayouts(uniformLayout, m_vBlurChain->getMipLevelCount())
        .build();
    
    auto sampler = device.createSampler(vk::SamplerCreateInfo {}
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear));
    
    scope.addDeferredCleanupFunction([=]() { device.destroySampler(sampler); });
    
    std::vector<Uniform::Update> uniformUpdates {
        m_emissiveUniform.update(vk::DescriptorType::eCombinedImageSampler, 0, 0)
            .addImageInfo(vk::DescriptorImageInfo {}
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(gBuffer.emissiveImageView)
                .setSampler(sampler))
    };

    for (int i = 0; i < m_hBlurChainViews.size(); i++) {
        uniformUpdates.push_back(m_hBlurUniform.update(vk::DescriptorType::eCombinedImageSampler, i, 0)
            .addImageInfo(vk::DescriptorImageInfo {}
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(m_hBlurChainViews[i])
                .setSampler(sampler)));
    }

    for (int i = 0; i < m_vBlurChainViews.size(); i++) {
        uniformUpdates.push_back(m_vBlurUniform.update(vk::DescriptorType::eCombinedImageSampler, i, 0)
            .addImageInfo(vk::DescriptorImageInfo {}
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(m_vBlurChainViews[i])
                .setSampler(sampler)));
    }

    Uniform::updateUniforms(uniformUpdates);
    
    auto pipelineResult = GraphicsPipelineBuilder { scope }
        .setPipelineLayout(PipelineLayoutBuilder { scope }
            .addSet(uniformLayout)
            .addPushConstantRange(vk::PushConstantRange {}
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                .setSize(sizeof(PassConfig)))
            .build())
        .addStageFromFile("shaders/fullscreen.vert.spv", "main", vk::ShaderStageFlagBits::eVertex)
        .addStageFromFile("shaders/bloom.frag.spv", "main", vk::ShaderStageFlagBits::eFragment)
        .addColorAttachmentFormat(gBuffer.emissiveImage->getFormat())
        .addAttachmentBlendState(vk::PipelineColorBlendAttachmentState
            { GraphicsPipelineBuilder::s_defaultAttachmentBlendState }
                .setDstColorBlendFactor(vk::BlendFactor::eOne))
        .build();
    
    if (pipelineResult.result != vk::Result::eSuccess) return false;

    m_pipeline = pipelineResult.value;

    return true;
}

void BloomPostProcess::beginRenderPass(
    vk::CommandBuffer cmd,
    Image& image, vk::ImageView imageView,
    uint32_t mipLevel,
    vk::AttachmentLoadOp loadOp
) {
    assert(image.layoutIs(vk::ImageLayout::eColorAttachmentOptimal, mipLevel, 1, 0, 1));

    auto colorAttachment = vk::RenderingAttachmentInfo {}
        .setImageLayout(image.getLayout(mipLevel))
        .setImageView(imageView)
        .setClearValue({ { 0.0f, 0.0f, 0.0f, 1.0f } })
        .setLoadOp(loadOp);
    
    glm::uvec2 imageSize { image.getSize(mipLevel) };

    cmd.beginRendering(vk::RenderingInfo {}
        .setColorAttachments(colorAttachment)
        .setRenderArea({ { 0, 0 }, { imageSize.x, imageSize.y } })
        .setLayerCount(1),
        IEngine::get().getDynamicDispatchLoader());
    
    cmd.setViewport(0, vk::Viewport {}
        .setWidth(imageSize.x)
        .setHeight(imageSize.y)
        .setMaxDepth(1.0f));
}

void BloomPostProcess::filterHighlights(vk::CommandBuffer cmd) {
    IEngine::get().getGBuffer().emissiveImage->transitionLayout()
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .execute(cmd);

    m_hBlurChain->transitionLayout()
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .execute(cmd);

    m_vBlurChain->transitionLayout()
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .execute(cmd);

    beginRenderPass(cmd, *m_vBlurChain, m_vBlurChainViews[0]);

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.layout, 0, m_emissiveUniform.getSet(), {});
    
    cmd.pushConstants<PassConfig>(m_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0,
        PassConfig {
            PassConfig::Filter, 0, PassConfig::Horizontal,
            clipping, dispersion, mixing
        });
    
    cmd.draw(3, 1, 0, 0);

    cmd.endRendering(IEngine::get().getDynamicDispatchLoader());
}

void BloomPostProcess::blurDownMipChain(vk::CommandBuffer cmd) {
    assert(m_vBlurChain->layoutIs(vk::ImageLayout::eColorAttachmentOptimal));
    assert(m_hBlurChain->layoutIs(vk::ImageLayout::eColorAttachmentOptimal));

    for (int i = 0; i < m_hBlurChain->getMipLevelCount() - 1; i++) {
        m_vBlurChain->transitionLayout(i, 1)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .execute(cmd);

        m_hBlurChain->transitionLayout(i, 1)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .execute(cmd);

        beginRenderPass(cmd, *m_hBlurChain, m_hBlurChainViews[i], i);

        cmd.pushConstants<PassConfig>(m_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0,
            PassConfig {
                PassConfig::Blur, i, PassConfig::Horizontal,
                clipping, dispersion, mixing
            });

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.layout, 0, m_vBlurUniform.getSet(i), {});
        cmd.draw(3, 1, 0, 0);

        cmd.endRendering(IEngine::get().getDynamicDispatchLoader());

        m_hBlurChain->transitionLayout(i, 1)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .execute(cmd);

        m_vBlurChain->transitionLayout(i + 1, 1)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .execute(cmd);
        
        beginRenderPass(cmd, *m_vBlurChain, m_vBlurChainViews[i + 1], i + 1);

        cmd.pushConstants<PassConfig>(m_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0,
            PassConfig {
                PassConfig::Blur, i, PassConfig::Vertical,
                clipping, dispersion, mixing
            });

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.layout, 0, m_hBlurUniform.getSet(i), {});
        cmd.draw(3, 1, 0, 0);

        cmd.endRendering(IEngine::get().getDynamicDispatchLoader());
    }

    m_vBlurChain->transitionLayout(m_vBlurChain->getMipLevelCount() - 1, 1)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .execute(cmd);
}

void BloomPostProcess::overlayUpMipChain(vk::CommandBuffer cmd) {
    assert(m_vBlurChain->layoutIsConsistent());

    m_vBlurChain->transitionLayout()
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .execute(cmd);

    for (int i = m_vBlurChain->getMipLevelCount() - 2; i >= 0; i--) {
        m_vBlurChain->transitionLayout(i + 1, 1)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .execute(cmd);

        beginRenderPass(cmd, *m_vBlurChain, m_vBlurChainViews[i], i, vk::AttachmentLoadOp::eLoad);

        cmd.pushConstants<PassConfig>(m_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0,
            PassConfig {
                PassConfig::Overlay, i + 1, PassConfig::Horizontal,
                clipping, dispersion, mixing
            });

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.layout, 0, m_vBlurUniform.getSet(i + 1), {});
        cmd.draw(3, 1, 0, 0);

        cmd.endRendering(IEngine::get().getDynamicDispatchLoader());
    }

    IEngine::GBuffer& gBuffer = IEngine::get().getGBuffer();

    m_vBlurChain->transitionLayout(0, 1)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .execute(cmd);

    gBuffer.emissiveImage->transitionLayout()
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .execute(cmd);

    beginRenderPass(cmd, *gBuffer.emissiveImage, gBuffer.emissiveImageView, 0, vk::AttachmentLoadOp::eLoad);

    cmd.pushConstants<PassConfig>(m_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0,
        PassConfig {
            PassConfig::Overlay, 1, PassConfig::Horizontal,
            clipping, dispersion, 1.0f
        });

    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.layout, 0, m_vBlurUniform.getSet(), {});
    cmd.draw(3, 1, 0, 0);

    cmd.endRendering(IEngine::get().getDynamicDispatchLoader());
}

void BloomPostProcess::draw(vk::CommandBuffer cmd) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline);

    filterHighlights(cmd);
    blurDownMipChain(cmd);
    overlayUpMipChain(cmd);
}

}
