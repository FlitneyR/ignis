#include "engine.hpp"
#include <chrono>

namespace ignis {

IEngine::IEngine() {}

IEngine::~IEngine() {
    executeDeferredCleanup();
}

void IEngine::init() {
    glfwInit();
    addDeferredCleanupCommand([=]() { glfwTerminate(); });
    
    glm::ivec2 initialWindowSize = getInitialWindowSize();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    m_window = glfwCreateWindow(
        initialWindowSize.x, initialWindowSize.y,
        getName().c_str(), nullptr, nullptr
    );
    addDeferredCleanupCommand([=]() { glfwDestroyWindow(m_window); });

    vkb::Result<vkb::Instance> instance_result = vkb::InstanceBuilder {}
        .set_app_name(getName().c_str())
        .set_engine_name(getEngineName().c_str())
        .set_app_version(getAppVersion())
        .set_engine_version(getEngineVersion())
        .request_validation_layers()
        .use_default_debug_messenger()
        .build();
    
    if (!instance_result) throw std::runtime_error("Failed to create a vulkan instance");
    m_instance = instance_result.value();
    addDeferredCleanupCommand([=]() { vkb::destroy_instance(m_instance); });

    vk::resultCheck(vk::Result { glfwCreateWindowSurface(getInstance(), m_window, nullptr, &m_surface) },
        "Failed to create a surface");
    addDeferredCleanupCommand([=]() { getInstance().destroySurfaceKHR(m_surface); });

    vkb::Result<vkb::PhysicalDevice> phys_device_result = vkb::PhysicalDeviceSelector { m_instance }
        .set_surface(m_surface)
        .set_minimum_version(1, 2)
        .add_required_extensions({
            "VK_KHR_dynamic_rendering",
            "VK_KHR_depth_stencil_resolve",
            "VK_KHR_create_renderpass2",
            "VK_KHR_multiview",
            "VK_KHR_maintenance2"
        })
        .select();
    
    if (!phys_device_result) throw std::runtime_error("Failed to select a physical device");

    m_phys_device = phys_device_result.value();

    auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures {}
        .setDynamicRendering(true);

    vkb::Result<vkb::Device> device_result = vkb::DeviceBuilder { m_phys_device }
        .add_pNext(&dynamicRenderingFeatures)
        .build();
    
    if (!device_result) throw std::runtime_error("Failed to create a logical device");
    m_device = device_result.value();
    addDeferredCleanupCommand([=]() { vkb::destroy_device(m_device); });

    VmaAllocatorCreateInfo allocatorCreatInfo {
        .device = getDevice(),
        .instance = getInstance(),
        .physicalDevice = getPhysicalDevice(),
    };
    vk::resultCheck(vk::Result { vmaCreateAllocator(&allocatorCreatInfo, &m_allocator) }, "Failed to create VMA allocator");
    addDeferredCleanupCommand([&]() { vmaDestroyAllocator(getAllocator()); });
    
    vkb::Result<VkQueue> graphics_queue_result = m_device.get_queue(vkb::QueueType::graphics);
    vkb::Result<VkQueue> present_queue_result = m_device.get_queue(vkb::QueueType::present);
    // vkb::Result<VkQueue> compute_queue_result = m_device.get_queue(vkb::QueueType::compute);
    
    if (!graphics_queue_result) throw std::runtime_error("Failed to find a graphics queue");
    if (!present_queue_result) throw std::runtime_error("Failed to find a present queue");
    // if (!compute_queue_result) throw std::runtime_error("Failed to find a compute queue");

    m_graphicsQueue = graphics_queue_result.value();
    m_presentQueue = present_queue_result.value();
    // m_computeQueue = compute_queue_result.value();

    m_graphicsQueueIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
    m_presentQueueIndex = m_device.get_queue_index(vkb::QueueType::present).value();
    // m_computeQueueIndex = m_device.get_queue_index(vkb::QueueType::compute).value();

    vkb::Result<vkb::Swapchain> swapchain_result = vkb::SwapchainBuilder {
            getPhysicalDevice(),
            getDevice(),
            getSurface(),
            m_graphicsQueueIndex,
            m_presentQueueIndex
        }
        .build();
    
    if (!swapchain_result) throw std::runtime_error("Failed to create a swapchain");
    m_swapchain = swapchain_result.value();
    addDeferredCleanupCommand([=]() { vkb::destroy_swapchain(m_swapchain); });

    auto swapchainImages_result = m_swapchain.get_images();
    auto swapchainImageViews_result = m_swapchain.get_image_views();

    if (!swapchainImages_result) throw std::runtime_error("Failed to get swapchain images");
    if (!swapchainImageViews_result) throw std::runtime_error("Failed to get swapchain image views");

    for (int i = 0; i < m_swapchain.image_count; i++) {
        m_swapchainImages.push_back(swapchainImages_result.value()[i]);
        m_swapchainImageViews.push_back(swapchainImageViews_result.value()[i]);
    }

    addDeferredCleanupCommand([=]() {
        for (auto& imageView : m_swapchainImageViews) getDevice().destroyImageView(imageView);
    });

    m_graphicsCmdPool = getDevice().createCommandPool(vk::CommandPoolCreateInfo {}.setQueueFamilyIndex(m_graphicsQueueIndex));
    // m_computeCmdPool = getDevice().createCommandPool(vk::CommandPoolCreateInfo {}.setQueueFamilyIndex(m_computeQueueIndex));
    m_presentCmdPool = getDevice().createCommandPool(vk::CommandPoolCreateInfo {}.setQueueFamilyIndex(m_presentQueueIndex));

    addDeferredCleanupCommand([=]() {
        getDevice().destroyCommandPool(m_presentCmdPool);
        // getDevice().destroyCommandPool(m_computeCmdPool);
        getDevice().destroyCommandPool(m_graphicsCmdPool);
    });
    
    for (int i = 0; i < s_framesInFlight; i++) {
        m_imageAcquiredSemaphores.push_back(getDevice().createSemaphore(vk::SemaphoreCreateInfo {}));
        m_renderingFinishedSemaphores.push_back(getDevice().createSemaphore(vk::SemaphoreCreateInfo {}));
        m_frameFinishedFences.push_back(getDevice().createFence(vk::FenceCreateInfo {}
            .setFlags(vk::FenceCreateFlagBits::eSignaled)));
    }

    addDeferredCleanupCommand([=]() {
        for (auto& semaphore : m_imageAcquiredSemaphores) getDevice().destroySemaphore(semaphore);
        for (auto& semaphore : m_renderingFinishedSemaphores) getDevice().destroySemaphore(semaphore);
        for (auto& fence : m_frameFinishedFences) getDevice().destroyFence(fence);
    });

    m_dispatchLoaderDynamic = vk::DispatchLoaderDynamic { getInstance(), vkGetInstanceProcAddr, getDevice(), vkGetDeviceProcAddr };
}

void IEngine::mainLoop() {
    auto loopStart = std::chrono::high_resolution_clock::now();
    auto lastFrameStart = loopStart;

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    #define SECONDS_BETWEEN(from, to) static_cast<double>(duration_cast<microseconds>(to - from).count()) / 1'000'000.0

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        auto thisFrameStart = std::chrono::high_resolution_clock::now();
        double timeSinceStart = SECONDS_BETWEEN(loopStart, thisFrameStart);
        double timeSinceLastFrameStart = SECONDS_BETWEEN(lastFrameStart, thisFrameStart);

        registerDeltaTime(timeSinceLastFrameStart);
        double deltaTime = getDeltaTime();

        update(deltaTime);
        draw();

        lastFrameStart = thisFrameStart;
    }

    getDevice().waitIdle();

    #undef SECONDS_BETWEEN
}

void IEngine::registerDeltaTime(double deltaTime) {
    m_lastNDeltaTimes.push_back(deltaTime);
    m_deltaTimeSum += deltaTime;

    if (m_lastNDeltaTimes.size() > s_deltaTimeSampleCount) {
        m_deltaTimeSum -= m_lastNDeltaTimes.front();
        m_lastNDeltaTimes.pop_front();
    }
}

double IEngine::getDeltaTime() const {
    return m_deltaTimeSum / m_lastNDeltaTimes.size();
}

void IEngine::draw() {
    vk::resultCheck(getDevice().waitForFences(m_frameFinishedFences[m_inFlightFrameIndex], true, UINT64_MAX),
        "Frame finished fence timed out");

    getDevice().resetFences(m_frameFinishedFences[m_inFlightFrameIndex]);

    vk::ResultValue<uint32_t> imageIndex = getDevice().acquireNextImageKHR(getSwapchain(), UINT64_MAX, m_imageAcquiredSemaphores[m_inFlightFrameIndex], nullptr);
    vk::resultCheck(imageIndex.result, "Failed to acquire next image");

    vk::CommandBuffer cmd = getDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(getCommandPool(vkb::QueueType::graphics))
        )[0];
    
    cmd.begin(vk::CommandBufferBeginInfo {});

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {}, {}, {},
        vk::ImageMemoryBarrier {}
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setImage(m_swapchainImages[imageIndex.value])
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setLayerCount(1)
                .setLevelCount(1))
        );

    auto colorAttachment = vk::RenderingAttachmentInfoKHR {}
        .setImageView(m_swapchainImageViews[imageIndex.value])
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setClearValue(vk::ClearValue {})
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        ;

    cmd.beginRenderingKHR(vk::RenderingInfo {}
        .setColorAttachments(colorAttachment)
        .setLayerCount(1)
        .setRenderArea({ { 0, 0 }, { 1280, 720 } }),
        m_dispatchLoaderDynamic);

    cmd.setViewport(0, vk::Viewport {}
        .setX(0).setY(0)
        .setWidth(1280).setHeight(720)
        .setMinDepth(0).setMaxDepth(1));
    recordDrawCommands(cmd);
    
    cmd.endRenderingKHR(m_dispatchLoaderDynamic);

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {}, {}, {},
        vk::ImageMemoryBarrier {}
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setImage(m_swapchainImages[imageIndex.value])
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setLayerCount(1)
                .setLevelCount(1))
        );

    cmd.end();

    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    getQueue(vkb::QueueType::graphics).submit(vk::SubmitInfo {}
        .setCommandBuffers(cmd)
        .setWaitDstStageMask(waitDstStageMask)
        .setWaitSemaphores(m_imageAcquiredSemaphores[m_inFlightFrameIndex])
        .setSignalSemaphores(m_renderingFinishedSemaphores[m_inFlightFrameIndex]),
        m_frameFinishedFences[m_inFlightFrameIndex]);

    vk::SwapchainKHR swapchain = getSwapchain();
    vk::resultCheck(getQueue(vkb::QueueType::present).presentKHR(vk::PresentInfoKHR {}
        .setImageIndices(imageIndex.value)
        .setSwapchains(swapchain)
        .setWaitSemaphores(m_renderingFinishedSemaphores[m_inFlightFrameIndex])),
        "Failed to present rendered image");

    m_inFlightFrameIndex = (m_inFlightFrameIndex + 1) % s_framesInFlight;

    getDevice().waitIdle();
}

void IEngine::executeDeferredCleanup() {
    while (!m_deferredCleanupCommands.empty()) {
        m_deferredCleanupCommands.back()();
        m_deferredCleanupCommands.pop_back();
    }
}

vk::Queue IEngine::getQueue(vkb::QueueType queueType) const {
    switch (queueType) {
    case vkb::QueueType::graphics: return m_graphicsQueue;
    case vkb::QueueType::compute: return m_computeQueue;
    case vkb::QueueType::present: return m_presentQueue;
    default: throw std::runtime_error("Requested queue type does not exist");
    }
}

uint32_t IEngine::getQueueIndex(vkb::QueueType queueType) const {
    switch (queueType) {
    case vkb::QueueType::graphics: return m_graphicsQueueIndex;
    case vkb::QueueType::compute: return m_computeQueueIndex;
    case vkb::QueueType::present: return m_presentQueueIndex;
    default: throw std::runtime_error("Requested queue type does not exist");
    }
}

vk::CommandPool IEngine::getCommandPool(vkb::QueueType queueType) const {
    switch (queueType) {
    case vkb::QueueType::graphics: return m_graphicsCmdPool;
    case vkb::QueueType::compute: return m_computeCmdPool;
    case vkb::QueueType::present: return m_presentCmdPool;
    default: throw std::runtime_error("Requested queue type does not exist");
    }
}

}
