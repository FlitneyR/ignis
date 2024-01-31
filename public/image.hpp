#pragma once

#include "libraries.hpp"

namespace ignis {

class Image;

class ImageLayoutTransition {
    Image& m_image;

public:
    ImageLayoutTransition(Image& image) : m_image(image) {}

    vk::PipelineStageFlags    m_srcStageMask     { vk::PipelineStageFlagBits::eTopOfPipe };
    vk::PipelineStageFlags    m_dstStageMask     { vk::PipelineStageFlagBits::eBottomOfPipe };
    vk::ImageLayout           m_oldLayout        { vk::ImageLayout::eUndefined };
    vk::ImageLayout           m_newLayout        { vk::ImageLayout::eUndefined };
    vk::AccessFlags           m_srcAccessMask    {};
    vk::AccessFlags           m_dstAccessMask    {};
    vk::ImageSubresourceRange m_subResourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    ImageLayoutTransition& setSrcStageMask(vk::PipelineStageFlags mask);
    ImageLayoutTransition& setDstStageMask(vk::PipelineStageFlags mask);
    ImageLayoutTransition& setOldLayout(vk::ImageLayout layout);
    ImageLayoutTransition& setNewLayout(vk::ImageLayout layout);
    ImageLayoutTransition& setSrcAccessMask(vk::AccessFlags mask);
    ImageLayoutTransition& setDstAccessMask(vk::AccessFlags mask);
    ImageLayoutTransition& setBaseMipLevel(uint32_t level);
    ImageLayoutTransition& setMipLevelCount(uint32_t count);
    ImageLayoutTransition& setBaseArrayLayer(uint32_t layer);
    ImageLayoutTransition& setArrayLayerCount(uint32_t count);
    ImageLayoutTransition& setAspectMask(vk::ImageAspectFlags mask);

    void execute(vk::CommandBuffer cmd);

};

class Image {
    friend ImageLayoutTransition;

    std::vector<vk::ImageLayout> m_imageLayouts;

public:
    const vk::Image     m_image;
    const vk::ImageView m_view;
    const vk::Format    m_format;
    const uint32_t      m_mipLevelCount;
    const uint32_t      m_arrayLayerCount;

    Image(
        vk::Image image,
        vk::ImageView view,
        vk::Format format,
        uint32_t mipLevelCount = 1,
        uint32_t arrayLayerCount = 1,
        vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined
    ) : m_image(image),
        m_view(view),
        m_format(format),
        m_arrayLayerCount(arrayLayerCount),
        m_mipLevelCount(mipLevelCount),
        m_imageLayouts(mipLevelCount * arrayLayerCount, initialLayout)
    {}

    ImageLayoutTransition transitionLayout(
        uint32_t baseMipLevel = 0,
        uint32_t levelCount = 1,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = 1);

    vk::ImageLayout& imageLayout(uint32_t mipLevel = 0, uint32_t arrayLayer = 0) {
        return m_imageLayouts[mipLevel + arrayLayer * m_mipLevelCount];
    }
};

}
