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
    vk::Result copyData(const void* data, uint32_t size);

    template<typename T>
    vk::Result copyData(const T& data) {
        return copyData(&data, sizeof(T));
    }

    template<typename T>
    vk::Result copyData(const std::vector<T>& data) {
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

    /**
     * @brief Copies data via a staging buffer
     * 
     * @param fence The fence to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     * @param signalSemaphore The semaphore to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     */
    vk::Result stagedCopyData(const void* data, uint32_t size, vk::Fence fence = {}, vk::Semaphore signalSemaphore = {});

    /**
     * @brief Copies data via a staging buffer
     * 
     * @param fence The fence to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     * @param signalSemaphore The semaphore to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     */
    template<typename T>
    vk::Result stagedCopyData(const T* data, uint32_t count, vk::Fence fence = {}, vk::Semaphore signalSemaphore = {}) {
        return stagedCopyData(static_cast<void*>(data), count * sizeof(T), fence, signalSemaphore);
    }

    /**
     * @brief Copies data via a staging buffer
     * 
     * @param fence The fence to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     * @param signalSemaphore The semaphore to signal for an asynchronous staged copy.
     *  If one is provided, the function will be called asynchronously
     */
    template<typename T>
    vk::Result stagedCopyData(const std::vector<T>& data, vk::Fence fence = {}, vk::Semaphore signalSemaphore = {}) {
        return stagedCopyData<T>(data.data(), data.size(), fence, signalSemaphore);
    }
};

}
