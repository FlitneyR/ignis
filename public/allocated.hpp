#pragma once

#include "libraries.hpp"

namespace ignis {

class BaseAllocated {
protected:
    VmaAllocator getAllocator();

public:
    BaseAllocated() = default;
    BaseAllocated(VmaAllocation allocation) : m_allocation(allocation) {}

    VmaAllocation m_allocation;

    VmaAllocationInfo getInfo();
    vk::ResultValue<void*> map();
    void unmap();
    vk::Result flush();
    vk::Result copyData(void* data, uint32_t size);

    template<typename T>
    vk::Result copyData(T& data) {
        return copyData(&data, sizeof(T));
    }

    template<typename T>
    vk::Result copyData(std::vector<T>& data) {
        return copyData(data.data(), data.size() * sizeof(T));
    }
};

/**
 * @brief Wrapper for a resource of type `Inner` to store data related to VMA allocations
 */
template<typename Inner>
struct Allocated : public BaseAllocated {
    Inner m_inner;

    Allocated() = default;

    Allocated(
        Inner&& inner,
        VmaAllocation allocation
    ) : BaseAllocated(allocation),
        m_inner(std::move(inner))
    {}

    Inner& operator  *() { return  m_inner; }
    Inner* operator ->() { return &m_inner; }
};

template<>
struct Allocated<vk::Buffer> : public BaseAllocated {
    vk::Buffer m_inner;

    Allocated() = default;

    Allocated(
        vk::Buffer inner,
        VmaAllocation allocation
    ) : BaseAllocated(allocation),
        m_inner(inner)
    {}

    vk::Buffer& operator  *() { return  m_inner; }
    vk::Buffer* operator ->() { return &m_inner; }

    vk::Result stagedCopyData(void* data, uint32_t size);

    template<typename T>
    vk::Result stagedCopyData(T* data, uint32_t count) {
        return stagedCopyData(static_cast<void*>(data), count * sizeof(T));
    }

    template<typename T>
    vk::Result stagedCopyData(std::vector<T> data) {
        return stagedCopyData<T>(data.data(), data.size());
    }
};

}
