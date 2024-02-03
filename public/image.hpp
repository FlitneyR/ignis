#pragma once

#include "libraries.hpp"
#include "builder.hpp"
#include "allocated.hpp"

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

    vk::Image     m_image;
    vk::Format    m_format;
    uint32_t      m_mipLevelCount;
    uint32_t      m_arrayLayerCount;
    vk::Extent3D  m_extent;
    std::optional<VmaAllocation> m_allocation;

public:
    Image() = default;

    Image(
        vk::Image                    image,
        vk::Format                   format,
        vk::Extent3D                 extent,
        uint32_t                     mipLevelCount = 1,
        uint32_t                     arrayLayerCount = 1,
        vk::ImageLayout              initialLayout = vk::ImageLayout::eUndefined
    );

    Image(Image&& other) = default;
    Image& operator =(Image&& other) = default;
    
    Image(const Image& other) = delete;
    Image& operator =(const Image& other) = delete;

    vk::Image                    getImage()           { return m_image; }
    vk::Format                   getFormat()          { return m_format; }
    uint32_t                     getMipLevelCount()   { return m_mipLevelCount; }
    uint32_t                     getArrayLayerCount() { return m_arrayLayerCount; }
    glm::ivec3                   getSize()            { return { m_extent.width, m_extent.height, m_extent.depth };}
    vk::Extent3D                 getExtent()          { return m_extent; }

    ImageLayoutTransition transitionLayout(
        uint32_t baseMipLevel = 0,
        uint32_t levelCount = 1,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = 1);

    vk::ImageLayout& layout(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
};

class ImageBuilder : public IBuilder<Allocated<Image>> {
    VmaAllocator    m_allocator;

public:
    uint32_t              m_mipLevelCount      = 1;
    uint32_t              m_arrayLayerCount    = 1;
    vk::Format            m_format             = vk::Format::eR8G8B8A8Srgb;
    vk::ImageUsageFlags   m_usage              = vk::ImageUsageFlagBits::eSampled;
    std::vector<uint32_t> m_queueFamilyIndices = {};
    vk::Extent3D          m_extent             = { 1, 1, 1 };
    vk::ImageType         m_imageType          = vk::ImageType::e2D;
    VmaMemoryUsage        m_memoryUsage        = VMA_MEMORY_USAGE_GPU_ONLY;

    ImageBuilder(ResourceScope& scope);

    ImageBuilder& setMipLevelCount(uint32_t mipLevelCount);
    ImageBuilder& setArrayLayerCount(uint32_t arrayLayerCount);
    ImageBuilder& setFormat(vk::Format format);
    ImageBuilder& setUsage(vk::ImageUsageFlags usage);
    ImageBuilder& setQueueFamilyIndices(std::vector<uint32_t> indices);
    ImageBuilder& addQueueFamilyIndex(uint32_t index);
    ImageBuilder& setSize(glm::ivec2 size);
    ImageBuilder& setSize(glm::ivec3 size);
    ImageBuilder& setImageType(vk::ImageType type);

    Allocated<Image> build() override;
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

    ImageViewBuilder(Image& image, ResourceScope& scope);

    ImageViewBuilder& setComponentMapping(vk::ComponentMapping mapping);
    ImageViewBuilder& setViewType(vk::ImageViewType viewType);
    ImageViewBuilder& setAspectMask(vk::ImageAspectFlags mask);
    ImageViewBuilder& setArrayLayerRange(uint32_t base, uint32_t count);
    ImageViewBuilder& setMipLevelRange(uint32_t base, uint32_t count);

    vk::ImageView build() override;
};

}
