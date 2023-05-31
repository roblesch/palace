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

    vk::UniqueSemaphore m_imageAvailableUniqueSemaphore;
    vk::UniqueSemaphore m_renderFinishedUniqueSemaphore;
    vk::UniqueFence m_inFlightUniqueFence;

public:
    Device() = default;
    Device(vk::Instance& instance, vk::SurfaceKHR& surface);

    vk::Device& getDevice();

    void beginCommandBuffer(uint32_t bufferIndex = 0);
    void endCommandBuffer(uint32_t bufferIndex = 0);
    void beginRenderPass(vk::RenderPass& renderPass, vk::Framebuffer& framebuffer, vk::Extent2D extent2D, uint32_t bufferIndex = 0);
    void endRenderPass(uint32_t bufferIndex = 0);
    void beginDraw(vk::Pipeline& graphicsPipeline, vk::Extent2D extent2D, uint32_t bufferIndex = 0);
    void drawFrame(vk::RenderPass& renderPass, std::vector<vk::UniqueFramebuffer>& framebuffers, vk::Extent2D extent2D, vk::Pipeline& graphicsPipeline, vk::SwapchainKHR& swapchain, uint32_t bufferIndex = 0);
    void waitIdle();
};

} // namespace vk_

#endif
