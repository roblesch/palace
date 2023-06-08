#pragma once

#include "include.hpp"

namespace vk_ {

class Device {
private:
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;

    struct {
        uint32_t graphics;
    } m_queueFamilyIndices;

    vk::Queue m_graphicsQueue;
    vk::UniqueCommandPool m_commandPool;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffer;

    std::vector<vk::UniqueSemaphore> m_imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> m_renderFinishedSemaphores;
    std::vector<vk::UniqueFence> m_inFlightFences;

public:
    static vk::UniqueCommandBuffer beginSingleUseCommandBuffer(vk::Device& device, vk::CommandPool& commandPool);
    static void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer, vk::Queue& graphicsQueue);

    Device() = default;
    Device(vk::Instance& instance, vk::SurfaceKHR& surface, uint32_t concurrentFrames);

    vk::PhysicalDevice& physicalDevice();
    vk::Device& device();
    vk::CommandPool& commandPool();
    vk::CommandBuffer& commandBuffer(size_t frame);
    vk::Semaphore& imageAvailableSemaphore(size_t frame);
    vk::Semaphore& renderFinishedSemaphore(size_t frame);
    vk::Fence& inFlightFence(size_t frame);
    vk::Queue& graphicsQueue();

    void recreateImageAvailableSemaphore(size_t frame);

    void waitIdle();
};

}
