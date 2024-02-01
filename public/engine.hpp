#pragma once

#include "libraries.hpp"
#include "image.hpp"
#include "resourceScope.hpp"

#include <chrono>

namespace ignis {

class IEngine {
public:
    void main();

    static IEngine& getSingleton();

    VmaAllocator       getAllocator()      const { return m_allocator; }
    vk::Instance       getInstance()       const { return { m_instance }; }
    vk::PhysicalDevice getPhysicalDevice() const { return { m_phys_device }; }
    vk::Device         getDevice()         const { return { m_device }; }
    vk::SurfaceKHR     getSurface()        const { return { m_surface }; }
    vk::SwapchainKHR   getSwapchain()      const { return { m_swapchain }; }
    uint32_t           getInFlightIndex()  const { return m_inFlightFrameIndex; }

    ResourceScope&             getGlobalResourceScope()   { return m_globalResourceScope; }
    vk::DispatchLoaderDynamic& getDynamicDispatchLoader() { return m_dispatchLoaderDynamic; }

protected:
    IEngine();
    virtual ~IEngine();

    virtual std::string getName()       = 0;
    virtual uint32_t    getAppVersion() = 0;

    virtual glm::ivec2 getInitialWindowSize() { return { 1280, 720 }; }

    virtual void setup() = 0;
    virtual void update(double deltaTime) = 0;
    virtual void recordDrawCommands(vk::CommandBuffer cmd) = 0;

    IEngine(IEngine&& other) = delete;
    IEngine(const IEngine& other) = delete;
    IEngine& operator =(IEngine&& other) = delete;
    IEngine& operator =(const IEngine& other) = delete;

    ResourceScope m_globalResourceScope;
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

    Allocated<Image>           m_depthImage;
    vk::ImageView              m_depthImageView;
    std::vector<Image>         m_swapchainImages;
    std::vector<vk::ImageView> m_swapchainImageViews;

    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;

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

    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_currentFrameStartTime;

    std::list<double>    m_lastNDeltaTimes;
    double               m_deltaTimeSum;
    static constexpr int s_deltaTimeSampleCount = 10;

    void   registerDeltaTime(double deltaTime);
    double getDeltaTime() const;
    double getTime() const;

    std::string getEngineName()    { return "Ignis"; };
    uint32_t    getEngineVersion() { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

private:
    static IEngine* s_singleton;

};

}
