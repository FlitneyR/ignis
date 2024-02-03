#include "allocated.hpp"
#include "engine.hpp"
#include "bufferBuilder.hpp"
#include "common.hpp"

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

vk::Result BaseAllocated::copyData(void* data, uint32_t size) {
    vk::ResultValue<void*> mapping = map();
    if (mapping.result != vk::Result::eSuccess) return mapping.result;
    std::memcpy(mapping.value, data, size);
    unmap();
    return flush();
}

vk::Result Allocated<vk::Buffer>::stagedCopyData(void* data, uint32_t size) {
    ResourceScope tempScope;

    vk::ResultValue<Allocated<vk::Buffer>> stagingBuffer = BufferBuilder { tempScope }
        .setAllocationUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
        .setBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
        .setSizeBuildAndCopyData(data, size);
    if (stagingBuffer.result != vk::Result::eSuccess) return stagingBuffer.result;

    IEngine& engine = IEngine::get();
    vk::Device device = engine.getDevice();

    vk::Fence fence = device.createFence(vk::FenceCreateInfo {});
    tempScope.addDeferredCleanupFunction([=]() { device.destroyFence(fence); });

    vk::CommandBuffer cmd = engine.beginOneTimeCommandBuffer(vkb::QueueType::graphics);
    cmd.copyBuffer(*stagingBuffer.value, m_inner, vk::BufferCopy {}.setSize(size));
    engine.submitOneTimeCommandBuffer(cmd, vkb::QueueType::graphics, vk::SubmitInfo {}, fence);

    return device.waitForFences(fence, true, UINT64_MAX);
}

}
