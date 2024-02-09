#include "uniform.hpp"
#include "engine.hpp"

namespace ignis {

vk::WriteDescriptorSet Uniform::Update::getWriteStruct() {
    if (m_imageInfos.size() > 0)  m_writeStruct.setImageInfo(m_imageInfos);
    if (m_bufferInfos.size() > 0) m_writeStruct.setBufferInfo(m_bufferInfos);
    return m_writeStruct;
}

Uniform::Update& Uniform::Update::setSet(uint32_t set) {
    m_writeStruct.setDstSet(r_uniform.getSet(set));
    return *this;
}

Uniform::Update& Uniform::Update::setBinding(uint32_t binding) {
    m_writeStruct.setDstBinding(binding);
    return *this;
}

Uniform::Update& Uniform::Update::setArrayElement(uint32_t index) {
    m_writeStruct.setDstArrayElement(index);
    return *this;
}

Uniform::Update& Uniform::Update::setType(vk::DescriptorType type) {
    m_writeStruct.setDescriptorType(type);
    return *this;
}

Uniform::Update& Uniform::Update::addImageInfo(vk::DescriptorImageInfo imageInfo) {
    m_imageInfos.push_back(imageInfo);
    m_bufferInfos = {};
    return *this;
}

Uniform::Update& Uniform::Update::addBufferInfo(vk::DescriptorBufferInfo bufferInfo) {
    m_bufferInfos.push_back(bufferInfo);
    m_imageInfos = {};
    return *this;
}

Uniform::Update& Uniform::Update::setImageInfos(std::vector<vk::DescriptorImageInfo> imageInfos) {
    m_imageInfos = imageInfos;
    m_bufferInfos = {};
    return *this;
}

Uniform::Update& Uniform::Update::setBufferInfos(std::vector<vk::DescriptorBufferInfo> bufferInfos) {
    m_bufferInfos = bufferInfos;
    m_imageInfos = {};
    return *this;
}

Uniform::Update Uniform::update(vk::DescriptorType type, uint32_t set, uint32_t binding) {
    return Uniform::Update { *this }
        .setType(type)
        .setSet(set)
        .setBinding(binding);
}

void Uniform::updateUniforms(std::vector<Uniform::Update> updates) {
    std::vector<vk::WriteDescriptorSet> descriptorWrites;
    for (auto& update : updates) descriptorWrites.push_back(update.getWriteStruct());
    IEngine::get().getDevice().updateDescriptorSets(descriptorWrites, {});
}

}
