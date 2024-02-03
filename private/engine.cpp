#include "engine.hpp"
#include "common.hpp"
#include <iostream>

namespace ignis {

IEngine* IEngine::s_singleton = nullptr;

IEngine::IEngine() {
    if (s_singleton)
        throw std::runtime_error("Trying to construct an engine when one already exists");
    
    s_singleton = this;
}

IEngine::~IEngine() {}

IEngine& IEngine::get() {
    if (!s_singleton)
        throw std::runtime_error("Trying to get IEngine singleton before it has been constructed");
    
    return *s_singleton;
}

vk::CommandBuffer IEngine::beginOneTimeCommandBuffer(vkb::QueueType queueType) {
    getCommandPool(queueType);
    vk::CommandBuffer cmd = getDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(getCommandPool(queueType))
        .setLevel(vk::CommandBufferLevel::ePrimary))[0];

    cmd.begin(vk::CommandBufferBeginInfo {}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    return cmd;
}

void IEngine::submitOneTimeCommandBuffer(vk::CommandBuffer cmd, vkb::QueueType queueType, vk::SubmitInfo submitInfo, vk::Fence fence) {
    cmd.end();
    getQueue(queueType).submit(submitInfo.setCommandBuffers(cmd), fence);
}

void IEngine::main() {
    init();
    setup();

    m_startTime = std::chrono::high_resolution_clock::now();
    auto lastFrameStart = m_startTime;

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    #define SECONDS_BETWEEN(from, to) static_cast<double>(duration_cast<microseconds>(to - from).count()) / 1'000'000.0

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        m_currentFrameStartTime = std::chrono::high_resolution_clock::now();
        double timeSinceStart = SECONDS_BETWEEN(m_startTime, m_currentFrameStartTime);
        double timeSinceLastFrameStart = SECONDS_BETWEEN(lastFrameStart, m_currentFrameStartTime);

        registerDeltaTime(timeSinceLastFrameStart);
        double deltaTime = getDeltaTime();

        update(deltaTime);
        draw();

        lastFrameStart = m_currentFrameStartTime;
    }

    getDevice().waitIdle();

    getGlobalResourceScope().executeDeferredCleanupFunctions();

    #undef SECONDS_BETWEEN
}

void IEngine::init() {
    auto& grs = getGlobalResourceScope();

    glfwInit();
    grs.addDeferredCleanupFunction([&]() {
        glfwTerminate();
    });
    
    glm::ivec2 initialWindowSize = getInitialWindowSize();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    m_window = glfwCreateWindow(
        initialWindowSize.x, initialWindowSize.y,
        getName().c_str(), nullptr, nullptr
    );
    grs.addDeferredCleanupFunction([&]() {
        glfwDestroyWindow(m_window);
    });

    m_instance = getValue(vkb::InstanceBuilder {}
        .set_app_name(getName().c_str())
        .set_engine_name(getEngineName().c_str())
        .set_app_version(getAppVersion())
        .set_engine_version(getEngineVersion())
        .request_validation_layers()
        .use_default_debug_messenger()
        .build(), "Failed to create a vulkan instance");
    
    grs.addDeferredCleanupFunction([&]() {
        vkb::destroy_instance(m_instance);
    });

    vk::resultCheck(vk::Result { glfwCreateWindowSurface(getInstance(), m_window, nullptr, &m_surface) },
        "Failed to create a surface");
    grs.addDeferredCleanupFunction([&]() {
        getInstance().destroySurfaceKHR(m_surface);
    });

    m_phys_device = getValue(vkb::PhysicalDeviceSelector { m_instance }
        .set_surface(m_surface)
        .set_minimum_version(1, 2)
        .add_required_extensions({
            "VK_KHR_dynamic_rendering",
            "VK_KHR_depth_stencil_resolve",
            "VK_KHR_create_renderpass2",
            "VK_KHR_multiview",
            "VK_KHR_maintenance2"
        })
        .select(), "Failed to select a physical device");

    auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures {}
        .setDynamicRendering(true);

    m_device = getValue(vkb::DeviceBuilder { m_phys_device }
        .add_pNext(&dynamicRenderingFeatures)
        .build(), "Failed to create a logical device");
    
    grs.addDeferredCleanupFunction([&]() {
        vkb::destroy_device(m_device);
    });

    VmaAllocatorCreateInfo allocatorCreatInfo {
        .device = getDevice(),
        .instance = getInstance(),
        .physicalDevice = getPhysicalDevice(),
    };
    vk::resultCheck(vk::Result { vmaCreateAllocator(&allocatorCreatInfo, &m_allocator) }, "Failed to create VMA allocator");
    grs.addDeferredCleanupFunction([&]() {
        vmaDestroyAllocator(getAllocator());
    });
    
    m_graphicsQueue = getValue(m_device.get_queue(vkb::QueueType::graphics), "Failed to find a graphics queue");
    m_presentQueue = getValue(m_device.get_queue(vkb::QueueType::present), "Failed to find a present queue");

    m_graphicsQueueIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
    m_presentQueueIndex = m_device.get_queue_index(vkb::QueueType::present).value();

    m_swapchain = getValue(vkb::SwapchainBuilder {
            getPhysicalDevice(),
            getDevice(),
            getSurface(),
            m_graphicsQueueIndex,
            m_presentQueueIndex
        }
        .build(), "Failed to create a swapchain");
    
    grs.addDeferredCleanupFunction([&]() {
        vkb::destroy_swapchain(m_swapchain);
    });

    auto swapchainImages = getValue(m_swapchain.get_images(), "Failed to get swapchain images");
    auto swapchainImageViews = getValue(m_swapchain.get_image_views(), "Failed to get swapchain image views");

    for (int i = 0; i < m_swapchain.image_count; i++) {
        m_swapchainImages.push_back(Image {
            vk::Image { swapchainImages[i] },
            static_cast<vk::Format>(m_swapchain.image_format),
            { 1280, 720, 1 }
        });
        m_swapchainImageViews.push_back(swapchainImageViews[i]);
    }

    grs.addDeferredCleanupFunction([&]() {
        for (auto& imageView : m_swapchainImageViews) getDevice().destroyImageView(imageView);
    });

    m_graphicsCmdPool = getDevice().createCommandPool(vk::CommandPoolCreateInfo {}.setQueueFamilyIndex(m_graphicsQueueIndex));
    m_presentCmdPool = getDevice().createCommandPool(vk::CommandPoolCreateInfo {}.setQueueFamilyIndex(m_presentQueueIndex));

    grs.addDeferredCleanupFunction([&]() {
        getDevice().destroyCommandPool(m_presentCmdPool);
        getDevice().destroyCommandPool(m_graphicsCmdPool);
    });
    
    for (int i = 0; i < s_framesInFlight; i++) {
        m_imageAcquiredSemaphores.push_back(getDevice().createSemaphore(vk::SemaphoreCreateInfo {}));
        m_renderingFinishedSemaphores.push_back(getDevice().createSemaphore(vk::SemaphoreCreateInfo {}));
        m_frameFinishedFences.push_back(getDevice().createFence(vk::FenceCreateInfo {}
            .setFlags(vk::FenceCreateFlagBits::eSignaled)));
    }

    grs.addDeferredCleanupFunction([&]() {
        for (auto& semaphore : m_imageAcquiredSemaphores)     getDevice().destroySemaphore(semaphore);
        for (auto& semaphore : m_renderingFinishedSemaphores) getDevice().destroySemaphore(semaphore);
        for (auto& fence     : m_frameFinishedFences)         getDevice().destroyFence(fence);
    });

    m_depthImage = ImageBuilder { getGlobalResourceScope() }
        .setFormat(vk::Format::eD32Sfloat)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
        .setSize({ 1280, 720 })
        .build();

    m_depthImageView = ImageViewBuilder { *m_depthImage, getGlobalResourceScope() }
        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
        .build();

    m_dispatchLoaderDynamic = vk::DispatchLoaderDynamic { getInstance(), vkGetInstanceProcAddr, getDevice(), vkGetDeviceProcAddr };
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

double IEngine::getTime() const {
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    #define SECONDS_BETWEEN(from, to) static_cast<double>(duration_cast<microseconds>(to - from).count()) / 1'000'000.0

    return SECONDS_BETWEEN(m_startTime, m_currentFrameStartTime);

    #undef SECONDS_BETWEEN
}

void IEngine::draw() {
    vk::Fence frameFinishedFence = m_frameFinishedFences[getInFlightIndex()];
    vk::Semaphore imageAcquiredSemaphore = m_imageAcquiredSemaphores[getInFlightIndex()];
    vk::Semaphore renderingFinishedSemaphore = m_renderingFinishedSemaphores[getInFlightIndex()];

    vk::resultCheck(getDevice().waitForFences(frameFinishedFence, true, UINT64_MAX), "Frame finished fence timed out");

    getDevice().resetFences(frameFinishedFence);

    vk::ResultValue<uint32_t> imageIndex = getDevice().acquireNextImageKHR(getSwapchain(), UINT64_MAX, imageAcquiredSemaphore, nullptr);
    vk::resultCheck(imageIndex.result, "Failed to acquire next image");

    vk::CommandBuffer cmd = getDevice().allocateCommandBuffers(vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(getCommandPool(vkb::QueueType::graphics))
        )[0];
    
    cmd.begin(vk::CommandBufferBeginInfo {});
    
    m_swapchainImages[imageIndex.value].transitionLayout()
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .execute(cmd);

    auto colorAttachment = vk::RenderingAttachmentInfo {}
        .setImageView(m_swapchainImageViews[imageIndex.value])
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setClearValue(vk::ClearValue {})
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        ;

    auto depthAttachment = vk::RenderingAttachmentInfo {}
        .setImageView(m_depthImageView)
        .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setClearValue(vk::ClearValue {}.setDepthStencil({ 1.0f }))
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        ;

    cmd.beginRendering(vk::RenderingInfo {}
        .setColorAttachments(colorAttachment)
        .setPDepthAttachment(&depthAttachment)
        .setLayerCount(1)
        .setRenderArea({ { 0, 0 }, { 1280, 720 } }),
        m_dispatchLoaderDynamic);

    cmd.setViewport(0, vk::Viewport {}
        .setX(0).setY(0)
        .setWidth(1280).setHeight(720)
        .setMinDepth(0).setMaxDepth(1));
    
    recordDrawCommands(cmd);
    
    cmd.endRendering(m_dispatchLoaderDynamic);
    
    m_swapchainImages[imageIndex.value].transitionLayout()
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
        .execute(cmd);

    cmd.end();

    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    getQueue(vkb::QueueType::graphics).submit(vk::SubmitInfo {}
        .setCommandBuffers(cmd)
        .setWaitDstStageMask(waitDstStageMask)
        .setWaitSemaphores(imageAcquiredSemaphore)
        .setSignalSemaphores(renderingFinishedSemaphore),
        frameFinishedFence);

    vk::SwapchainKHR swapchain = getSwapchain();
    vk::resultCheck(getQueue(vkb::QueueType::present).presentKHR(vk::PresentInfoKHR {}
        .setImageIndices(imageIndex.value)
        .setSwapchains(swapchain)
        .setWaitSemaphores(renderingFinishedSemaphore)),
        "Failed to present rendered image");

    m_inFlightFrameIndex = (m_inFlightFrameIndex + 1) % s_framesInFlight;

    getDevice().waitIdle();
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
