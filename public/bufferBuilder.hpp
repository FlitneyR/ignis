#pragma once

#include "builder.hpp"
#include "allocated.hpp"

namespace ignis {

class BufferBuilder : public IBuilder<vk::ResultValue<Allocated<vk::Buffer>>> {
    VmaAllocator m_allocator;
    vk::BufferCreateInfo m_bufferCreateInfo        {};
    VmaAllocationCreateInfo m_allocationCreateInfo {};
    std::vector<uint32_t> m_queueFamilyIndices     {};

public:
    BufferBuilder(
        vk::Device device,
        VmaAllocator allocator,
        ResourceScope& scope
    );

    BufferBuilder& addQueueFamilyIndices(std::vector<uint32_t> indices);
    BufferBuilder& setAllocationUsage(VmaMemoryUsage usage);
    BufferBuilder& setSize(uint32_t size);
    BufferBuilder& setBufferUsage(vk::BufferUsageFlags usage);

    vk::ResultValue<Allocated<vk::Buffer>> build() override;

};

}
