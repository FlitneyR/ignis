#pragma once

#include "libraries.hpp"
#include <list>

namespace ignis {

class IEngine {
protected:
    std::list<std::function<void()>> m_deferredCleanupCommands;

    GLFWwindow* m_window;
    VmaAllocator m_allocator;

    vkb::Instance       m_instance;
    vkb::PhysicalDevice m_phys_device;
    vkb::Device         m_device;
    vkb::Swapchain      m_swapchain;
    VkSurfaceKHR        m_surface;

    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;
    vk::Queue m_computeQueue;

    uint32_t m_graphicsQueueIndex;
    uint32_t m_presentQueueIndex;
    uint32_t m_computeQueueIndex;

    vk::CommandPool m_graphicsCmdPool;
    vk::CommandPool m_presentCmdPool;
    vk::CommandPool m_computeCmdPool;

    VmaAllocator       getAllocator()      const { return m_allocator; }
    vk::Instance       getInstance()       const { return { m_instance }; }
    vk::PhysicalDevice getPhysicalDevice() const { return { m_phys_device }; }
    vk::Device         getDevice()         const { return { m_device }; }
    vk::SurfaceKHR     getSurface()        const { return { m_surface }; }
    vk::SwapchainKHR   getSwapchain()      const { return { m_swapchain }; }

    std::vector<vk::Image>     m_swapchainImages;
    std::vector<vk::ImageView> m_swapchainImageViews;

    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;

    IEngine();
    virtual ~IEngine();

    IEngine(const IEngine& other) = delete;
    IEngine(const IEngine&& other) = delete;
    IEngine& operator =(const IEngine& other) = delete;
    IEngine& operator =(const IEngine&& other) = delete;

    static constexpr uint8_t s_framesInFlight = 3;
    std::vector<vk::Semaphore> m_imageAcquiredSemaphores;
    std::vector<vk::Semaphore> m_renderingFinishedSemaphores;
    std::vector<vk::Fence> m_frameFinishedFences;
    uint8_t m_inFlightFrameIndex = 0;

    vk::Queue       getQueue(vkb::QueueType queueType) const;
    uint32_t        getQueueIndex(vkb::QueueType queueType) const;
    vk::CommandPool getCommandPool(vkb::QueueType queueType) const;

    void init();
    void draw();
    void executeDeferredCleanup();

    std::list<double>    m_lastNDeltaTimes;
    double               m_deltaTimeSum;
    static constexpr int s_deltaTimeSampleCount = 10;

    void   registerDeltaTime(double deltaTime);

public:
    void mainLoop();

    std::string getEngineName()    { return "Ignis"; };
    uint32_t    getEngineVersion() { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

    double getDeltaTime() const;

    void addDeferredCleanupCommand(std::function<void()> command) {
        m_deferredCleanupCommands.push_back(command);
    }

    virtual std::string getName()       = 0;
    virtual uint32_t    getAppVersion() = 0;

    virtual glm::ivec2 getInitialWindowSize() { return { 1280, 720 }; }

protected:
    virtual void setup() = 0;
    virtual void update(double deltaTime) = 0;
    virtual void recordDrawCommands(vk::CommandBuffer cmd) = 0;

};

}
