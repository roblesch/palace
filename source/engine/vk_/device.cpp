#include "device.hpp"

namespace vk_ {

Device::Device(vk::Instance& instance, vk::SurfaceKHR& surface)
{
    // physical device
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    for (auto& device : devices) {
        vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physicalDevice = device;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            m_physicalDevice = device;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
            m_physicalDevice = device;
            break;
        }
    }

    // queue families
    std::vector<vk::QueueFamilyProperties> queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_queueFamilyIndices.graphics = i;
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = m_queueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = new float(0.0f) }
    };

    // device
    vk::PhysicalDeviceFeatures deviceFeatures {};
    std::vector<const char*> extensionNames = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
    };

    vk::DeviceCreateInfo deviceInfo {
        .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
        .pEnabledFeatures = {}
    };

    m_uniqueDevice = m_physicalDevice.createDeviceUnique(deviceInfo);
    auto device = m_uniqueDevice.get();

    // graphics queue
    m_graphicsQueue = device.getQueue(m_queueFamilyIndices.graphics, 0);

    // command pool
    vk::CommandPoolCreateInfo commandPoolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_queueFamilyIndices.graphics
    };

    m_uniqueCommandPool = device.createCommandPoolUnique(commandPoolInfo);

    // command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo {
        .commandPool = m_uniqueCommandPool.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    m_uniqueCommandBuffers = device.allocateCommandBuffersUnique(commandBufferAllocInfo);

    // sync
    m_imageAvailableUniqueSemaphore = device.createSemaphoreUnique({});
    m_renderFinishedUniqueSemaphore = device.createSemaphoreUnique({});

    vk::FenceCreateInfo fenceInfo {
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    m_inFlightUniqueFence = device.createFenceUnique(fenceInfo);
}

vk::Device& Device::getDevice()
{
    return m_uniqueDevice.get();
}

void Device::beginCommandBuffer(uint32_t bufferIndex)
{
    vk::CommandBufferBeginInfo beginInfo {};
    m_uniqueCommandBuffers[bufferIndex].get().begin(beginInfo);
}

void Device::endCommandBuffer(uint32_t bufferIndex)
{
    m_uniqueCommandBuffers[bufferIndex].get().end();
}

void Device::beginRenderPass(vk::RenderPass& renderPass, vk::Framebuffer& framebuffer, vk::Extent2D extent2D, uint32_t bufferIndex)
{
    vk::ClearValue clearColor { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };
    vk::RenderPassBeginInfo renderPassInfo
    {
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = extent2D
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };
    m_uniqueCommandBuffers[bufferIndex].get().beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void Device::endRenderPass(uint32_t bufferIndex)
{
    m_uniqueCommandBuffers[bufferIndex].get().endRenderPass();
}

void Device::beginDraw(vk::Pipeline& graphicsPipeline, vk::Extent2D extent2D, uint32_t bufferIndex)
{
    m_uniqueCommandBuffers[bufferIndex].get().bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent2D.width),
        .height = static_cast<float>(extent2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    m_uniqueCommandBuffers[bufferIndex].get().setViewport(0, 1, &viewport);

    vk::Rect2D scissor {
        .offset = { 0, 0 },
        .extent = extent2D
    };

    m_uniqueCommandBuffers[bufferIndex].get().setScissor(0, 1, &scissor);
    m_uniqueCommandBuffers[bufferIndex].get().draw(3, 1, 0, 0);
}

void Device::drawFrame(vk::RenderPass& renderPass, std::vector<vk::UniqueFramebuffer>& framebuffers, vk::Extent2D extent2D, vk::Pipeline& graphicsPipeline, vk::SwapchainKHR& swapchain, uint32_t bufferIndex)
{
    auto device = m_uniqueDevice.get();
    auto inFlightFence = m_inFlightUniqueFence.get();
    auto imageAvailable = m_imageAvailableUniqueSemaphore.get();
    auto commandBuffer = m_uniqueCommandBuffers[bufferIndex].get();
    auto renderFinished = m_renderFinishedUniqueSemaphore.get();

    while (device.waitForFences(inFlightFence, VK_TRUE, UINT64_MAX) == vk::Result::eTimeout)
        ;
    device.resetFences(inFlightFence);

    auto [result, imageIndex] = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE);
    commandBuffer.reset();

    beginCommandBuffer();
    beginRenderPass(renderPass, framebuffers[imageIndex].get(), extent2D);
    beginDraw(graphicsPipeline, extent2D);
    endRenderPass();
    endCommandBuffer();

    vk::PipelineStageFlags waitDstStageMask { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailable,
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinished
    };

    m_graphicsQueue.submit(submitInfo, inFlightFence);

    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    m_graphicsQueue.presentKHR(presentInfo);
}

void Device::waitIdle()
{
    m_uniqueDevice.get().waitIdle();
}

} // namespace vk_
