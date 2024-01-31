#include "image.hpp"

namespace ignis {

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

ImageLayoutTransition& ImageLayoutTransition::setBaseMipLevel(uint32_t level) {
    m_subResourceRange.setBaseMipLevel(level);
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setMipLevelCount(uint32_t count) {
    m_subResourceRange.setLevelCount(count);
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setBaseArrayLayer(uint32_t layer) {
    m_subResourceRange.setBaseArrayLayer(layer);
    return *this;
}

ImageLayoutTransition& ImageLayoutTransition::setArrayLayerCount(uint32_t count) {
    m_subResourceRange.setLevelCount(count);
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
            .setImage(m_image.m_image)
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
        m_image.imageLayout(mipLevel, arrayLayer) = m_newLayout;
    }
}

ImageLayoutTransition Image::transitionLayout(uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount) {
    return ImageLayoutTransition { *this }
        .setBaseMipLevel(baseMipLevel)
        .setMipLevelCount(levelCount)
        .setBaseArrayLayer(baseArrayLayer)
        .setArrayLayerCount(layerCount)
        .setOldLayout(imageLayout(baseMipLevel, baseArrayLayer));
}

}
