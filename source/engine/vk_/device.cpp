#include "device.hpp"

namespace vk_ {

Device::Device(const vk::Instance& instance, const vk::SurfaceKHR& surface)
{
    // physical device
    auto devices = instance.enumeratePhysicalDevices();
    for (const auto& device : devices) {
        auto deviceProperties = device.getProperties();
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
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilyIndices.graphics = i;
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = queueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = new float(0.0f) }
    };

    // logical device
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
    m_graphicsQueue = m_uniqueDevice.get().getQueue(queueFamilyIndices.graphics, 0);
}

vk::PhysicalDevice Device::getPhysical()
{
    return m_physicalDevice;
}

vk::Device Device::get()
{
    return m_uniqueDevice.get();
}

} // namespace vk_
