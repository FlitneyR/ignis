#pragma once

#include "libraries.hpp"
#include "builder.hpp"

namespace ignis {

class Image;

class ImageLayoutTransition {
    Image& r_image;

public:
    ImageLayoutTransition(Image& image);

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
    ImageLayoutTransition& setArrayLayerRange(uint32_t base, uint32_t count);
    ImageLayoutTransition& setMipLevelRange(uint32_t base, uint32_t count);
    ImageLayoutTransition& setAspectMask(vk::ImageAspectFlags mask);

    void execute(vk::CommandBuffer cmd);

};

class Image {
    friend ImageLayoutTransition;

    std::vector<vk::ImageLayout> m_imageLayouts;

public:
    const vk::Image     m_image;
    const vk::Format    m_format;
    const uint32_t      m_mipLevelCount;
    const uint32_t      m_arrayLayerCount;
    const vk::Extent3D  m_extent;
    std::optional<VmaAllocation> m_allocation;

    Image(
        vk::Image                    image,
        vk::Format                   format,
        vk::Extent3D                 extent,
        uint32_t                     mipLevelCount = 1,
        uint32_t                     arrayLayerCount = 1,
        std::optional<VmaAllocation> allocation = {},
        vk::ImageLayout              initialLayout = vk::ImageLayout::eUndefined
    );

    ImageLayoutTransition transitionLayout(
        uint32_t baseMipLevel = 0,
        uint32_t levelCount = 1,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = 1);

    vk::ImageLayout& layoutAt(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
};

class ImageBuilder : public IBuilder<Image> {
    VmaAllocator    m_allocator;

public:
    uint32_t              m_mipLevelCount;
    uint32_t              m_arrayLayerCount;
    vk::Format            m_format;
    vk::ImageUsageFlags   m_usage;
    std::vector<uint32_t> m_queueFamilyIndices;
    vk::Extent3D          m_extent;
    vk::ImageType         m_imageType;

    ImageBuilder(vk::Device device, VmaAllocator allocator, ResourceScope& scope);

    ImageBuilder& setMipLevelCount(uint32_t mipLevelCount);
    ImageBuilder& setArrayLayerCount(uint32_t arrayLayerCount);
    ImageBuilder& setFormat(vk::Format format);
    ImageBuilder& setUsage(vk::ImageUsageFlags usage);
    ImageBuilder& setQueueFamilyIndices(std::vector<uint32_t> indices);
    ImageBuilder& addQueueFamilyIndex(uint32_t index);
    ImageBuilder& setSize(glm::ivec2 size);
    ImageBuilder& setSize(glm::ivec3 size);
    ImageBuilder& setImageType(vk::ImageType type);

    Image build() override;
};

class ImageViewBuilder : public IBuilder<vk::ImageView> {
    Image& r_image;

public:
    vk::ComponentMapping m_components      = {};
    vk::ImageViewType    m_viewType        = vk::ImageViewType::e2D;
    vk::ImageAspectFlags m_aspectMask      = vk::ImageAspectFlagBits::eColor;
    uint32_t             m_baseArrayLayer  = 0;
    uint32_t             m_baseMipLevel    = 0;
    uint32_t             m_arrayLayerCount = 1;
    uint32_t             m_mipLevelCount   = 1;

    ImageViewBuilder(vk::Device device, Image& image, ResourceScope& scope);

    ImageViewBuilder& setComponentMapping(vk::ComponentMapping mapping);
    ImageViewBuilder& setViewType(vk::ImageViewType viewType);
    ImageViewBuilder& setAspectMask(vk::ImageAspectFlags mask);
    ImageViewBuilder& setArrayLayerRange(uint32_t base, uint32_t count);
    ImageViewBuilder& setMipLevelRange(uint32_t base, uint32_t count);

    vk::ImageView build() override;
};

}
