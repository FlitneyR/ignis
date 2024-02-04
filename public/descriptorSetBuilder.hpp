#include "libraries.hpp"
#include "builder.hpp"
#include "descriptorSet.hpp"

namespace ignis {

class DescriptorPoolBuilder : public IBuilder<vk::DescriptorPool> {
    uint32_t m_maxSetCount;
    std::vector<vk::DescriptorPoolSize> m_poolSizes;
    vk::DescriptorPoolCreateFlags m_flags;

public:
    DescriptorPoolBuilder(ResourceScope& scope) : IBuilder(scope) {}

    DescriptorPoolBuilder& setMaxSetCount(uint32_t count);
    DescriptorPoolBuilder& setPoolSizes(std::vector<vk::DescriptorPoolSize> sizes);
    DescriptorPoolBuilder& addPoolSizes(std::vector<vk::DescriptorPoolSize> sizes);
    DescriptorPoolBuilder& addPoolSize(vk::DescriptorPoolSize size);
    DescriptorPoolBuilder& addFlags(vk::DescriptorPoolCreateFlags flags);

    vk::DescriptorPool build() override;
};

class DescriptorLayoutBuilder : public IBuilder<vk::DescriptorSetLayout> {
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

public:
    DescriptorLayoutBuilder(ResourceScope& scope) : IBuilder(scope) {}

    DescriptorLayoutBuilder& setBindings(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    DescriptorLayoutBuilder& addBindings(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    DescriptorLayoutBuilder& addBinding(vk::DescriptorSetLayoutBinding binding);

    vk::DescriptorSetLayout build() override;
};

class DescriptorSetBuilder : public IBuilder<DescriptorSetCollection> {
    vk::DescriptorPool m_pool;
    std::vector<vk::DescriptorSetLayout> m_layouts;

public:
    DescriptorSetBuilder(
        ResourceScope& scope,
        vk::DescriptorPool pool = {}
    ) : IBuilder(scope),
        m_pool(pool)
    {}

    DescriptorSetBuilder& setPool(vk::DescriptorPool pool);
    DescriptorSetBuilder& setLayouts(std::vector<vk::DescriptorSetLayout> layouts);
    DescriptorSetBuilder& addLayouts(std::vector<vk::DescriptorSetLayout> layouts);
    DescriptorSetBuilder& addLayouts(vk::DescriptorSetLayout layout, uint32_t count = 1);

    DescriptorSetCollection build() override;
};

}