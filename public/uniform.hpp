#pragma once

#include "libraries.hpp"
#include <optional>

namespace ignis {

class UniformUpdate;

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

    vk::DescriptorSet       getSet(uint32_t index = 0) { return m_sets[index]; }
    vk::DescriptorSetLayout getLayout(uint32_t index = 0) { return m_layouts[index]; }

    class Update {
        Uniform& r_uniform;

        vk::WriteDescriptorSet m_writeStruct {};
        std::vector<vk::DescriptorImageInfo> m_imageInfos;
        std::vector<vk::DescriptorBufferInfo> m_bufferInfos;

    public:
        Update(Uniform& uniform) : r_uniform(uniform) {}

        Update& setSet(uint32_t set);
        Update& setBinding(uint32_t binding);
        Update& setArrayElement(uint32_t index);
        Update& setType(vk::DescriptorType type);
        Update& addImageInfo(vk::DescriptorImageInfo imageInfo);
        Update& addBufferInfo(vk::DescriptorBufferInfo bufferInfo);
        Update& setImageInfos(std::vector<vk::DescriptorImageInfo> imageInfos);
        Update& setBufferInfos(std::vector<vk::DescriptorBufferInfo> bufferInfos);

        vk::WriteDescriptorSet getWriteStruct();
    };

    Update update(vk::DescriptorType type, uint32_t set = 0, uint32_t binding = 0);

    static void updateUniforms(std::vector<Update> updates);
};

}
