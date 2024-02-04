#include "descriptorSet.hpp"

namespace ignis {

vk::DescriptorSet DescriptorSetCollection::getSet(uint32_t index) {
    return m_sets[index];
}

vk::DescriptorSetLayout DescriptorSetCollection::getLayout(uint32_t index) {
    return m_layouts[index];
}

}
