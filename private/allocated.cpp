#include "allocated.hpp"
#include "engine.hpp"
#include "bufferBuilder.hpp"
#include "common.hpp"

#include <thread>

namespace ignis {

VmaAllocator BaseAllocated::getAllocator() {
    return IEngine::get().getAllocator();
}

VmaAllocationInfo BaseAllocated::getInfo() {
    VmaAllocationInfo info;
    vmaGetAllocationInfo(getAllocator(), m_allocation, &info);
    return info;
}

vk::ResultValue<void*> BaseAllocated::map() {
    void* data;
    vk::Result result { vmaMapMemory(getAllocator(), m_allocation, &data) };
    return vk::ResultValue { result, data };
}

void BaseAllocated::unmap() {
    vmaUnmapMemory(getAllocator(), m_allocation);
}

vk::Result BaseAllocated::flush() {
    VmaAllocationInfo info = getInfo();
    return vk::Result { vmaFlushAllocation(getAllocator(), m_allocation, info.offset, info.size) };
}

vk::Result BaseAllocated::copyData(const void* data, uint32_t size) {
    vk::ResultValue<void*> mapping = map();
    if (mapping.result != vk::Result::eSuccess) return mapping.result;
    std::memcpy(mapping.value, data, size);
    unmap();
    return flush();
}

vk::Result Allocated<vk::Buffer>::stagedCopyData(const void* data, uint32_t size, vk::Fence fence, vk::Semaphore signalSemaphore) {
    bool async = fence != VK_NULL_HANDLE || signalSemaphore != VK_NULL_HANDLE;

    // using a pointer here so it can be deleted from another thread
    ResourceScope* tempScope = new ResourceScope();
    IEngine& engine = IEngine::get();
    vk::Device device = engine.getDevice();

    if (fence == VK_NULL_HANDLE) {
        fence = device.createFence(vk::FenceCreateInfo {});
        tempScope->addDeferredCleanupFunction([=]() { device.destroyFence(fence); });
    }

    vk::ResultValue<Allocated<vk::Buffer>> stagingBuffer = BufferBuilder { *tempScope }
        .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
        .setBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setSizeBuildAndCopyData(data, size);
    
    if (stagingBuffer.result != vk::Result::eSuccess) {
        delete tempScope;
        return stagingBuffer.result;
    }

    vk::CommandBuffer cmd = engine.beginOneTimeCommandBuffer(vkb::QueueType::graphics);
    cmd.copyBuffer(*stagingBuffer.value, m_inner, vk::BufferCopy {}.setSize(size));

    vk::SubmitInfo submitInfo;
    if (signalSemaphore != VK_NULL_HANDLE) submitInfo.setSignalSemaphores(signalSemaphore);

    engine.submitOneTimeCommandBuffer(cmd, vkb::QueueType::graphics, submitInfo, fence);

    if (async) {
        std::thread cleanUpAndSignalFence ([=]() {
            auto _ = device.waitForFences(fence, true, UINT64_MAX);
            delete tempScope;
        });
        cleanUpAndSignalFence.detach();

        return vk::Result::eSuccess;
    } else {
        vk::Result result = device.waitForFences(fence, true, UINT64_MAX);
        delete tempScope;
        return result;
    }
}

}
