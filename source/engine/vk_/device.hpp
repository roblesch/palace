#ifndef PALACE_ENGINE_VK_DEVICE_HPP
#define PALACE_ENGINE_VK_DEVICE_HPP

#include "include.hpp"

namespace vk_ {

class Device {
private:
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_uniqueDevice;

    struct {
        uint32_t graphics;
    } m_queueFamilyIndices;

    vk::Queue m_graphicsQueue;
    vk::UniqueCommandPool m_uniqueCommandPool;
    std::vector<vk::UniqueCommandBuffer> m_uniqueCommandBuffers;

    std::vector<vk::UniqueSemaphore> m_uniqueSemaphoresImageAvailable;
    std::vector<vk::UniqueSemaphore> m_uniqueSemaphoresRenderFinished;
    std::vector<vk::UniqueFence> m_uniqueFencesInFlight;

public:
    static vk::UniqueCommandBuffer beginSingleUseCommandBuffer(vk::Device& device, vk::CommandPool& commandPool);
    static void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer, vk::Queue& graphicsQueue);

    Device() = default;
    Device(vk::Instance& instance, vk::SurfaceKHR& surface, uint32_t concurrentFrames);

    vk::PhysicalDevice& physicalDevice();
    vk::Device& device();
    vk::CommandPool& commandPool();
    vk::CommandBuffer& commandBuffer(size_t i);
    vk::Semaphore& semaphoreImageAvailable(size_t i);
    vk::Semaphore& semaphoreRenderFinished(size_t i);
    vk::Fence& fenceInFlight(size_t i);
    vk::Queue& graphicsQueue();

    void recreateImageAvailableSemaphore(size_t i);

    void waitIdle();
};

} // namespace vk_

#endif
