#include "uniformBuilder.hpp"
#include "engine.hpp"

namespace ignis {

DescriptorPoolBuilder& DescriptorPoolBuilder::setMaxSetCount(uint32_t count) {
    m_maxSetCount = count;
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::setPoolSizes(std::vector<vk::DescriptorPoolSize> sizes) {
    m_poolSizes = sizes;
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::addPoolSizes(std::vector<vk::DescriptorPoolSize> sizes) {
    m_poolSizes.insert(m_poolSizes.end(), sizes.begin(), sizes.end());
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::addPoolSize(vk::DescriptorPoolSize size) {
    m_poolSizes.push_back(size);
    return *this;
}

DescriptorPoolBuilder& DescriptorPoolBuilder::addFlags(vk::DescriptorPoolCreateFlags flags) {
    m_flags |= flags;
    return *this;
}

vk::DescriptorPool DescriptorPoolBuilder::build() {
    vk::DescriptorPool pool = getDevice().createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setFlags(m_flags)
        .setMaxSets(m_maxSetCount)
        .setPoolSizes(m_poolSizes));
    
    r_scope.addDeferredCleanupFunction([=, device = getDevice()]() {
        device.destroyDescriptorPool(pool);
    });

    return pool;
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::setBindings(std::vector<vk::DescriptorSetLayoutBinding> bindings) {
    m_bindings = bindings;
    return *this;
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::addBindings(std::vector<vk::DescriptorSetLayoutBinding> bindings) {
    m_bindings.insert(m_bindings.end(), bindings.begin(), bindings.end());
    return *this;
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::addBinding(vk::DescriptorSetLayoutBinding binding) {
    m_bindings.push_back(binding);
    return *this;
}

vk::DescriptorSetLayout DescriptorLayoutBuilder::build() {
    vk::DescriptorSetLayout layout = getDevice().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(m_bindings));

    r_scope.addDeferredCleanupFunction([=, device = getDevice()]() {
        device.destroyDescriptorSetLayout(layout);
    });

    return layout;
}

UniformBuilder& UniformBuilder::setPool(vk::DescriptorPool pool) {
    m_pool = pool;
    return *this;
}

UniformBuilder& UniformBuilder::setLayouts(std::vector<vk::DescriptorSetLayout> layouts) {
    m_layouts = layouts;
    return *this;
}

UniformBuilder& UniformBuilder::addLayouts(std::vector<vk::DescriptorSetLayout> layouts) {
    m_layouts.insert(m_layouts.end(), layouts.begin(), layouts.end());
    return *this;
}

UniformBuilder& UniformBuilder::addLayouts(vk::DescriptorSetLayout layout, uint32_t count) {
    m_layouts.insert(m_layouts.end(), count, layout);
    return *this;
}

Uniform UniformBuilder::build() {
    return { getDevice().allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_pool)
        .setSetLayouts(m_layouts)), m_layouts };
}

}