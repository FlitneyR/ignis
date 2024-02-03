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

    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndCopyData(void* data, uint32_t size) {
        setSize(size);

        vk::ResultValue<Allocated<vk::Buffer>> ret = build();
        if (ret.result == vk::Result::eSuccess)
            ret.result = ret.value.copyData(data, size);

        return ret;
    }

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndCopyData(T* data, uint32_t count) {
        return setSizeBuildAndCopyData(static_cast<void*>(data), count * sizeof(T));
    }

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndCopyData(T& data) {
        return setSizeBuildAndCopyData(&data, 1);
    }

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndCopyData(std::vector<T>& data) {
        return setSizeBuildAndCopyData(data.data(), data.size());
    }

    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndStagedCopyData(void* data, uint32_t size) {
        setSize(size);

        m_bufferCreateInfo.usage |= vk::BufferUsageFlagBits::eTransferDst;
        vk::ResultValue<Allocated<vk::Buffer>> ret = build();

        if (ret.result == vk::Result::eSuccess)
            ret.result = ret.value.stagedCopyData(data, size);

        return ret;
    }

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndStagedCopyData(T* data, uint32_t count) {
        return setSizeBuildAndStagedCopyData(static_cast<void*>(data), count * sizeof(T));
    }

    template<typename T>
    vk::ResultValue<Allocated<vk::Buffer>> setSizeBuildAndStagedCopyData(std::vector<T>& data) {
        return setSizeBuildAndStagedCopyData(data.data(), data.size());
    }

    template<typename T>
    BufferBuilder& setSize(uint32_t count = 1) {
        return setSize(sizeof(T) * count);
    }

    vk::ResultValue<Allocated<vk::Buffer>> build() override;

};

}
