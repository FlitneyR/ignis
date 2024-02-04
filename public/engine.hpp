#pragma once

#include "libraries.hpp"
#include "image.hpp"
#include "resourceScope.hpp"
#include "log.hpp"

#include <chrono>

namespace ignis {

class IEngine {
public:
    void main();

    /**
     * @brief Get the engine singleton
     */
    static IEngine& get();

    Log& getLog() { return m_log; }

    ResourceScope& getGlobalResourceScope()        { return m_globalResourceScope; }
    ResourceScope& getUntilWindowSizeChangeScope() { return m_untilWindowSizeChangeScope; }

    vk::DispatchLoaderDynamic& getDynamicDispatchLoader() { return m_dispatchLoaderDynamic; }

    GLFWwindow*        getWindow()         const { return m_window; }
    VmaAllocator       getAllocator()      const { return m_allocator; }
    vk::Instance       getInstance()       const { return { m_instance }; }
    vk::PhysicalDevice getPhysicalDevice() const { return { m_phys_device }; }
    vk::Device         getDevice()         const { return { m_device }; }
    vk::SurfaceKHR     getSurface()        const { return { m_surface }; }
    vkb::Swapchain     getVkbSwapchain()   const { return m_swapchain; }
    vk::SwapchainKHR   getSwapchain()      const { return { m_swapchain }; }
    uint32_t           getInFlightIndex()  const { return m_inFlightFrameIndex; }
    ImGuiContext*      getImGuiContext()   const { return m_imGuiContext; }
    Allocated<Image>&  getDepthBuffer()          { return m_depthImage; }

    vk::Queue       getQueue(vkb::QueueType queueType) const;
    uint32_t        getQueueIndex(vkb::QueueType queueType) const;
    vk::CommandPool getCommandPool(vkb::QueueType queueType) const;

    /**
     * @brief Create a command buffer that can be used for one time actions. e.g. staged copying
     */
    vk::CommandBuffer beginOneTimeCommandBuffer(vkb::QueueType queueType);

    /**
     * @brief Submit a command buffer created by IEngine::beginOneTimeCommandBuffer
     */
    void submitOneTimeCommandBuffer(vk::CommandBuffer cmd, vkb::QueueType queueType, vk::SubmitInfo submitInfo, vk::Fence fence = {});

    double getDeltaTime() const;
    double getTime()      const;

    virtual std::string getName()       = 0;
    virtual uint32_t    getAppVersion() = 0;

    static constexpr uint8_t s_framesInFlight = 3;

    IEngine(IEngine&& other) = delete;
    IEngine(const IEngine& other) = delete;
    IEngine& operator =(IEngine&& other) = delete;
    IEngine& operator =(const IEngine& other) = delete;

protected:
    IEngine();
    virtual ~IEngine();

    virtual glm::ivec2 getInitialWindowSize() { return { 1280, 720 }; }

    virtual void setup() = 0;
    virtual void update(double deltaTime) = 0;
    virtual void recordDrawCommands(vk::CommandBuffer cmd) = 0;
    virtual void onWindowSizeChanged(glm::vec<2, uint32_t> size) {}

private:
    static IEngine* s_singleton;

    std::string getEngineName()    { return "Ignis"; };
    uint32_t    getEngineVersion() { return VK_MAKE_API_VERSION(0, 1, 0, 0); }

    void init();
    void draw();
    void windowSizeChanged();
    
    void registerDeltaTime(double deltaTime);

    Log m_log;

    ResourceScope m_globalResourceScope        { "Global" };
    ResourceScope m_untilWindowSizeChangeScope { "Until window size change" };

    GLFWwindow*   m_window;
    VmaAllocator  m_allocator;
    ImGuiContext* m_imGuiContext;

    vk::DispatchLoaderDynamic m_dispatchLoaderDynamic;

    vkb::Instance       m_instance;
    vkb::PhysicalDevice m_phys_device;
    vkb::Device         m_device;
    vkb::Swapchain      m_swapchain;
    VkSurfaceKHR        m_surface;

    Allocated<Image>           m_depthImage;
    vk::ImageView              m_depthImageView;
    std::vector<Image>         m_swapchainImages;
    std::vector<vk::ImageView> m_swapchainImageViews;

    std::vector<vk::Semaphore> m_imageAcquiredSemaphores;
    std::vector<vk::Semaphore> m_renderingFinishedSemaphores;
    std::vector<vk::Fence>     m_frameFinishedFences;
    uint8_t                    m_inFlightFrameIndex = 0;

    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_currentFrameStartTime;

    std::list<double>    m_lastNDeltaTimes;
    double               m_deltaTimeSum;
    static constexpr int s_deltaTimeSampleCount = 10;

    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;

    uint32_t m_graphicsQueueIndex;
    uint32_t m_presentQueueIndex;

    vk::CommandPool m_graphicsCmdPool;
    vk::CommandPool m_presentCmdPool;

};

}
