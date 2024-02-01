#pragma once

#include "libraries.hpp"

namespace ignis {

/**
 * @brief Wrapper for a resource of type `Inner` to store data related to VMA allocations
 */
template<typename Inner>
struct Allocated {
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

    VmaAllocationInfo getInfo(VmaAllocator allocator) {
        VmaAllocationInfo info;
        vmaGetAllocationInfo(allocator, m_allocation, &info);
        return info;
    }

    vk::ResultValue<void*> map(VmaAllocator allocator) {
        void* data;
        vk::Result result { vmaMapMemory(allocator, m_allocation, &data) };
        return vk::ResultValue { result, data };
    }

    void unmap(VmaAllocator allocator) {
        vmaUnmapMemory(allocator, m_allocation);
    }

    void flush(VmaAllocator allocator) {
        VmaAllocationInfo info = getInfo(allocator);
        vmaFlushAllocation(allocator, m_allocation, info.offset, info.size);
    }

    vk::Result copyData(VmaAllocator allocator, void* data, uint32_t size) {
        vk::ResultValue<void*> mapping = map(allocator);
        if (mapping.result != vk::Result::eSuccess) return mapping.result;

        std::memcpy(mapping.value, data, size);

        flush(allocator);
        unmap(allocator);

        return vk::Result::eSuccess;
    }

    template<typename T>
    vk::Result copyData(VmaAllocator allocator, std::vector<T> data) {
        return copyData(allocator, data.data(), data.size() * sizeof(T));
    }
};

}
