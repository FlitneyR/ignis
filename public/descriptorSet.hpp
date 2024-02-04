#pragma once

#include "libraries.hpp"

namespace ignis {

class DescriptorSetCollection {
    std::vector<vk::DescriptorSet>       m_sets;
    std::vector<vk::DescriptorSetLayout> m_layouts;

public:
    DescriptorSetCollection() = default;

    DescriptorSetCollection(
        std::vector<vk::DescriptorSet>       sets,
        std::vector<vk::DescriptorSetLayout> layouts
    ) : m_sets(sets),
        m_layouts(layouts)
    {}

    vk::DescriptorSet       getSet(uint32_t index = 0);
    vk::DescriptorSetLayout getLayout(uint32_t index = 0);
};

}
