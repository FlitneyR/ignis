#include "uniform.hpp"
#include "engine.hpp"

namespace ignis {

void Uniform::update(
    uint32_t set, uint32_t binding, vk::DescriptorType type,
    std::vector<vk::DescriptorImageInfo> imageInfos,
    std::optional<vk::DescriptorBufferInfo> bufferInfo
) {
    auto& engine = IEngine::get();
    vk::Device device = engine.getDevice();

    auto write = vk::WriteDescriptorSet {}
        .setDstSet(getSet(set))
        .setDescriptorType(type)
        .setDstBinding(binding)
        .setDescriptorCount(1);
    
    if (imageInfos.size() > 0) write.setImageInfo(imageInfos);
    if (bufferInfo) write.setPBufferInfo(&*bufferInfo);

    device.updateDescriptorSets(write, {});
}

}
