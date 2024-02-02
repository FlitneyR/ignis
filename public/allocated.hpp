#pragma once

#include "libraries.hpp"

namespace ignis {

class BaseAllocated {
protected:
    VmaAllocator getAllocator();
};

/**
 * @brief Wrapper for a resource of type `Inner` to store data related to VMA allocations
 */
template<typename Inner>
struct Allocated : public BaseAllocated {
    Inner m_inner;
    VmaAllocation m_allocation;

    Allocated() = default;

    Allocated(
        Inner&& inner,
        VmaAllocation allocation
    ) : m_inner(std::move(inner)),
        m_allocation(allocation)
    {}

    Inner& operator  *() { return  m_inner; }
    Inner* operator ->() { return &m_inner; }

    VmaAllocationInfo getInfo() {
        VmaAllocationInfo info;
        vmaGetAllocationInfo(getAllocator(), m_allocation, &info);
        return info;
    }

    vk::ResultValue<void*> map() {
        void* data;
        vk::Result result { vmaMapMemory(getAllocator(), m_allocation, &data) };
        return vk::ResultValue { result, data };
    }

    void unmap() {
        vmaUnmapMemory(getAllocator(), m_allocation);
    }

    vk::Result flush() {
        VmaAllocationInfo info = getInfo();
        return vmaFlushAllocation(getAllocator(), m_allocation, info.offset, info.size);
    }

    vk::Result copyData(void* data, uint32_t size) {
        vk::ResultValue<void*> mapping = map();
        if (mapping.result != vk::Result::eSuccess) return mapping.result;

        std::memcpy(mapping.value, data, size);

        vk::Result result = flush();
        unmap();

        return result;
    }

    template<typename T>
    vk::Result copyData(std::vector<T> data) {
        return copyData(data.data(), data.size() * sizeof(T));
    }
};

}
