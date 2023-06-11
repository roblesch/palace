#include "device.hpp"

namespace vk_ {

vk::UniqueCommandBuffer Device::beginSingleUseCommandBuffer(vk::Device& device, vk::CommandPool& commandPool)
{
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::UniqueCommandBuffer commandBuffer { std::move(device.allocateCommandBuffersUnique(bufferInfo)[0]) };
    commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return commandBuffer;
}

void Device::endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer, vk::Queue& graphicsQueue)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

Device::Device(vk::Instance& instance, vk::SurfaceKHR& surface, uint32_t concurrentFrames)
{
    // physical device
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    for (auto& device : devices) {
        vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();
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
    vk::PhysicalDeviceFeatures deviceFeatures { .samplerAnisotropy = VK_TRUE };
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
        .pEnabledFeatures = &deviceFeatures
    };

    m_device = m_physicalDevice.createDeviceUnique(deviceInfo);

    // graphics queue
    m_graphicsQueue = device().getQueue(m_queueFamilyIndices.graphics, 0);

    // command pool
    vk::CommandPoolCreateInfo commandPoolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_queueFamilyIndices.graphics
    };

    m_commandPool = device().createCommandPoolUnique(commandPoolInfo);

    // command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo {
        .commandPool = commandPool(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = concurrentFrames
    };

    m_commandBuffer = device().allocateCommandBuffersUnique(commandBufferAllocInfo);

    // sync
    for (size_t i = 0; i < concurrentFrames; i++) {
        m_imageAvailableSemaphores.push_back(device().createSemaphoreUnique({}));
        m_renderFinishedSemaphores.push_back(device().createSemaphoreUnique({}));
        m_inFlightFences.push_back(device().createFenceUnique({ .flags = vk::FenceCreateFlagBits::eSignaled }));
    }
}

vk::PhysicalDevice& Device::physicalDevice()
{
    return m_physicalDevice;
}

vk::Device& Device::device()
{
    return *m_device;
}

vk::CommandPool& Device::commandPool()
{
    return *m_commandPool;
}

vk::CommandBuffer& Device::commandBuffer(size_t frame)
{
    return *m_commandBuffer[frame];
}

vk::Semaphore& Device::imageAvailableSemaphore(size_t frame)
{
    return *m_imageAvailableSemaphores[frame];
}

vk::Semaphore& Device::renderFinishedSemaphore(size_t frame)
{
    return *m_renderFinishedSemaphores[frame];
}

vk::Fence& Device::inFlightFence(size_t frame)
{
    return *m_inFlightFences[frame];
}

vk::Queue& Device::graphicsQueue()
{
    return m_graphicsQueue;
}

void Device::waitIdle()
{
    m_device->waitIdle();
}

void Device::recreateImageAvailableSemaphore(size_t frame)
{
    m_imageAvailableSemaphores[frame] = m_device->createSemaphoreUnique({});
}

}
