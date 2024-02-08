#pragma once

#include "libraries.hpp"
#include <optional>

namespace ignis {

class Uniform {
    std::vector<vk::DescriptorSet>       m_sets;
    std::vector<vk::DescriptorSetLayout> m_layouts;

public:
    Uniform() = default;

    Uniform(
        std::vector<vk::DescriptorSet>       sets,
        std::vector<vk::DescriptorSetLayout> layouts
    ) : m_sets(sets),
        m_layouts(layouts)
    {}

    void update(
        uint32_t set, uint32_t binding, vk::DescriptorType type,
        std::vector<vk::DescriptorImageInfo> imageInfos = {},
        std::optional<vk::DescriptorBufferInfo> bufferInfo = std::nullopt
    );

    vk::DescriptorSet       getSet(uint32_t index = 0) { return m_sets[index]; }
    vk::DescriptorSetLayout getLayout(uint32_t index = 0) { return m_layouts[index]; }
};

}
