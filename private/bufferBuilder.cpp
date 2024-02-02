#include "bufferBuilder.hpp"

namespace ignis {

BufferBuilder::BufferBuilder(
    ResourceScope& scope
) : IBuilder(scope)
{}

BufferBuilder& BufferBuilder::addQueueFamilyIndices(std::vector<uint32_t> indices) {
    m_queueFamilyIndices.insert(m_queueFamilyIndices.end(), indices.begin(), indices.end());
    return *this;
}

BufferBuilder& BufferBuilder::setBufferUsage(vk::BufferUsageFlags usage) {
    m_bufferCreateInfo.setUsage(usage);
    return *this;
}

BufferBuilder& BufferBuilder::setAllocationUsage(VmaMemoryUsage usage) {
    m_allocationCreateInfo.usage = usage;
    return *this;
}

BufferBuilder& BufferBuilder::setSize(uint32_t size) {
    m_bufferCreateInfo.setSize(size);
    return *this;
}

vk::ResultValue<Allocated<vk::Buffer>> BufferBuilder::build() {
    Allocated<vk::Buffer> value;

    VkBufferCreateInfo bufferCreateInfo = m_bufferCreateInfo;
    VkBuffer buffer;

    vk::Result result { vmaCreateBuffer(getAllocator(), &bufferCreateInfo, &m_allocationCreateInfo, &buffer, &value.m_allocation, nullptr) };
    *value = buffer;

    r_scope.addDeferredCleanupFunction([=, allocator = getAllocator(), allocation = value.m_allocation]() {
        vmaDestroyBuffer(allocator, buffer, allocation);
    });

    return { result, value };
}

}
