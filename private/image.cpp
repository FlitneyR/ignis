#include "image.hpp"
#include "engine.hpp"

namespace ignis {

ImageLayoutTransition::ImageLayoutTransition(
    Image& image
) : r_image(image)
{}

ImageLayoutTransition& ImageLayoutTransition::setSrcStageMask(vk::PipelineStageFlags mask) {
    m_srcStageMask = mask;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setDstStageMask(vk::PipelineStageFlags mask) {
    m_dstStageMask = mask;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setOldLayout(vk::ImageLayout layout) {
    m_oldLayout = layout;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setNewLayout(vk::ImageLayout layout) {
    m_newLayout = layout;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setSrcAccessMask(vk::AccessFlags mask) {
    m_srcAccessMask = mask;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setDstAccessMask(vk::AccessFlags mask) {
    m_dstAccessMask = mask;
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setArrayLayerRange(uint32_t base, uint32_t count) {
    m_subResourceRange
        .setBaseArrayLayer(base)
        .setLayerCount(count);
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setMipLevelRange(uint32_t base, uint32_t count) {
    m_subResourceRange
        .setBaseMipLevel(base)
        .setLevelCount(count);
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setAspectMask(vk::ImageAspectFlags mask) {
    m_subResourceRange.setAspectMask(mask);
    return *this;
}

void ImageLayoutTransition::execute(vk::CommandBuffer cmd) {
    cmd.pipelineBarrier(
        m_srcStageMask,
        m_dstStageMask,
        {}, {}, {},
        vk::ImageMemoryBarrier {}
            .setImage(r_image.m_image)
            .setOldLayout(m_oldLayout)
            .setNewLayout(m_newLayout)
            .setSrcAccessMask(m_srcAccessMask)
            .setDstAccessMask(m_dstAccessMask)
            .setSubresourceRange(m_subResourceRange)
    );

    uint32_t mipLevelFrom = m_subResourceRange.baseMipLevel;
    uint32_t mipLevelTo = mipLevelFrom + m_subResourceRange.levelCount;
    uint32_t arrayLayerFrom = m_subResourceRange.baseArrayLayer;
    uint32_t arrayLayerTo = arrayLayerFrom + m_subResourceRange.layerCount;

    for (uint32_t mipLevel = mipLevelFrom; mipLevel < mipLevelTo; mipLevel++)
    for (uint32_t arrayLayer = arrayLayerFrom; arrayLayer < arrayLayerTo; arrayLayer++) {
        r_image.layout(mipLevel, arrayLayer) = m_newLayout;
    }
}

ImageLayoutTransition Image::transitionLayout(uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    return ImageLayoutTransition { *this }
        .setMipLevelRange(baseMipLevel, levelCount)
        .setArrayLayerRange(baseArrayLayer, layerCount)
        .setOldLayout(layout(baseMipLevel, baseArrayLayer));
}

Image::Image(
    vk::Image       image,
    vk::Format      format,
    vk::Extent3D    extent,
    uint32_t        mipLevelCount,
    uint32_t        arrayLayerCount,
    vk::ImageLayout initialLayout
) : m_image(image),
    m_format(format),
    m_extent(extent),
    m_arrayLayerCount(arrayLayerCount),
    m_mipLevelCount(mipLevelCount),
    m_imageLayouts(mipLevelCount * arrayLayerCount, initialLayout)
{}
vk::ImageLayout& Image::layout(uint32_t mipLevel, uint32_t arrayLayer) {
    return m_imageLayouts[mipLevel + arrayLayer * m_mipLevelCount];
}

ImageBuilder::ImageBuilder(
    ResourceScope& scope
) : IBuilder(scope)
{}

ImageBuilder& ImageBuilder::setMipLevelCount(uint32_t mipLevelCount) {
    m_mipLevelCount = mipLevelCount;
    return *this;
}

ImageBuilder& ImageBuilder::setArrayLayerCount(uint32_t arrayLayerCount) {
    m_arrayLayerCount = arrayLayerCount;
    return *this;
}

ImageBuilder& ImageBuilder::setFormat(vk::Format format) {
    m_format = format;
    return *this;
}

ImageBuilder& ImageBuilder::setUsage(vk::ImageUsageFlags usage) {
    m_usage = usage;
    return *this;
}

ImageBuilder& ImageBuilder::setQueueFamilyIndices(std::vector<uint32_t> indices) {
    m_queueFamilyIndices = indices;
    return *this;
}

ImageBuilder& ImageBuilder::addQueueFamilyIndex(uint32_t index) {
    m_queueFamilyIndices.push_back(index);
    return *this;
}

ImageBuilder& ImageBuilder::setSize(glm::ivec2 size) {
    return setSize({ size.x, size.y, 1 });
}

ImageBuilder& ImageBuilder::setSize(glm::ivec3 size) {
    m_extent
        .setWidth(size.x)
        .setHeight(size.y)
        .setDepth(size.z);
    return *this;
}

ImageBuilder& ImageBuilder::setImageType(vk::ImageType type) {
    m_imageType = type;
    return *this;
}

Allocated<Image> ImageBuilder::build() {
    VkImageCreateInfo imageCreateInfo = vk::ImageCreateInfo {}
        .setMipLevels(m_mipLevelCount)
        .setArrayLayers(m_arrayLayerCount)
        .setFormat(m_format)
        .setQueueFamilyIndices(m_queueFamilyIndices)
        .setUsage(m_usage)
        .setExtent(m_extent)
        .setImageType(m_imageType)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1);

    VmaAllocationCreateInfo allocationCreateInfo {
        .usage = m_memoryUsage,
    };

    VkImage image;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    vmaCreateImage(getAllocator(), &imageCreateInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo);

    r_scope.addDeferredCleanupFunction([=, allocator = getAllocator()]() {
        vmaDestroyImage(allocator, image, allocation);
    });

    return Allocated { Image {
        image,
        m_format,
        m_extent,
        m_mipLevelCount,
        m_arrayLayerCount
    }, allocation };
}

ImageViewBuilder::ImageViewBuilder(
    Image& image,
    ResourceScope& scope
) : IBuilder(scope),
    r_image(image)
{
    m_components
        .setR(vk::ComponentSwizzle::eR)
        .setG(vk::ComponentSwizzle::eG)
        .setB(vk::ComponentSwizzle::eB)
        .setA(vk::ComponentSwizzle::eA);
    
    m_arrayLayerCount = image.getArrayLayerCount();
    m_mipLevelCount = image.getMipLevelCount();
}

ImageViewBuilder& ImageViewBuilder::setComponentMapping(vk::ComponentMapping mapping) {
    m_components = mapping;
    return *this;
}

ImageViewBuilder& ImageViewBuilder::setViewType(vk::ImageViewType viewType) {
    m_viewType = viewType;
    return *this;
}

ImageViewBuilder& ImageViewBuilder::setAspectMask(vk::ImageAspectFlags mask) {
    m_aspectMask = mask;
    return *this;
}

ImageViewBuilder& ImageViewBuilder::setArrayLayerRange(uint32_t base, uint32_t count) {
    m_baseArrayLayer = base;
    m_arrayLayerCount = count;
    return *this;
}

ImageViewBuilder& ImageViewBuilder::setMipLevelRange(uint32_t base, uint32_t count) {
    m_baseMipLevel = base;
    m_mipLevelCount = count;
    return *this;
}

vk::ImageView ImageViewBuilder::build() {
    vk::ImageView imageView = getDevice().createImageView(vk::ImageViewCreateInfo {}
        .setFormat(r_image.getFormat())
        .setImage(r_image.getImage())
        .setComponents(m_components)
        .setViewType(m_viewType)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(m_aspectMask)
            .setBaseArrayLayer(m_baseArrayLayer)
            .setBaseMipLevel(m_baseMipLevel)
            .setLayerCount(m_arrayLayerCount)
            .setLevelCount(m_mipLevelCount))
        );
    
    r_scope.addDeferredCleanupFunction([=, device = getDevice()]() {
        device.destroyImageView(imageView);
    });
    
    return imageView;
}

}
