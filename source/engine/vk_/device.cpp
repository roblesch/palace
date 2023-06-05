#include "device.hpp"

namespace vk_ {

Device::Device(vk::Instance& instance, vk::SurfaceKHR& surface, uint32_t concurrentFrames)
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
        .commandBufferCount = concurrentFrames
    };

    m_uniqueCommandBuffers = device.allocateCommandBuffersUnique(commandBufferAllocInfo);

    // sync
    for (size_t i = 0; i < concurrentFrames; i++) {
        m_uniqueSemaphoresImageAvailable.push_back(device.createSemaphoreUnique({}));
        m_uniqueSemaphoresRenderFinished.push_back(device.createSemaphoreUnique({}));
        m_uniqueFencesInFlight.push_back(device.createFenceUnique({ .flags = vk::FenceCreateFlagBits::eSignaled }));
    }
}

vk::PhysicalDevice& Device::physicalDevice()
{
    return m_physicalDevice;
}

vk::Device& Device::device()
{
    return m_uniqueDevice.get();
}

vk::CommandBuffer& Device::commandBuffer(size_t i)
{
    return m_uniqueCommandBuffers[i].get();
}

vk::Semaphore& Device::semaphoreImageAvailable(size_t i)
{
    return m_uniqueSemaphoresImageAvailable[i].get();
}

vk::Semaphore& Device::semaphoreRenderFinished(size_t i)
{
    return m_uniqueSemaphoresRenderFinished[i].get();
}

vk::Fence& Device::fenceInFlight(size_t i)
{
    return m_uniqueFencesInFlight[i].get();
}

vk::Queue& Device::graphicsQueue()
{
    return m_graphicsQueue;
}

void Device::waitIdle()
{
    m_uniqueDevice.get().waitIdle();
}

} // namespace vk_
