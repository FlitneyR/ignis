#pragma once

#include "builder.hpp"
#include "allocated.hpp"

namespace ignis {

class BufferBuilder : public IBuilder<vk::ResultValue<Allocated<vk::Buffer>>> {
    vk::BufferCreateInfo m_bufferCreateInfo        {};
    VmaAllocationCreateInfo m_allocationCreateInfo {};
    std::vector<uint32_t> m_queueFamilyIndices     {};

public:
    BufferBuilder(
        ResourceScope& scope
    );

    BufferBuilder& addQueueFamilyIndices(std::vector<uint32_t> indices);
    BufferBuilder& setAllocationUsage(VmaMemoryUsage usage);
    BufferBuilder& setSize(uint32_t size);
    BufferBuilder& setBufferUsage(vk::BufferUsageFlags usage);

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndCopyData(std::vector<T>& data) {
        setSize<T>(data.size());
        vk::ResultValue<Allocated<vk::Buffer>> ret = build();

        if (ret.result == vk::Result::eSuccess)
            ret.result = ret.value.copyData(data);

        return ret;
    }

    template<typename T>
    BufferBuilder& setSize(uint32_t count = 1) {
        return setSize(sizeof(T) * count);
    }

    vk::ResultValue<Allocated<vk::Buffer>> build() override;

};

}
