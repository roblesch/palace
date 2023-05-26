#include "vulkandevice.hpp"

namespace graphics::vk_ {

Device::Device(vk::Instance instance, vk::SurfaceKHR surface)
{
    // select a physical device
    auto devices = instance.enumeratePhysicalDevices();
    for (const auto& device : devices) {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures = device.getFeatures();

        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physical = device;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            m_physical = device;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
            m_physical = device;
            break;
        }
    }

    // fill queue family indices
    auto queueFamilies = m_physical.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilyIndices.graphics = i;
        }
        if (m_physical.getSurfaceSupportKHR(i, surface)) {
            queueFamilyIndices.present = i;
        }
    }

    // create a logical device
    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = queueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = new float(1.0f) },
        { .queueFamilyIndex = queueFamilyIndices.present,
            .queueCount = 1,
            .pQueuePriorities = new float(1.0f) }
    };

    vk::PhysicalDeviceFeatures deviceFeatures {};
    std::vector<const char*> extensionNames = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
    };

    vk::DeviceCreateInfo deviceInfo {
        .queueCreateInfoCount = uint32_t(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = uint32_t(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
        .pEnabledFeatures = {}
    };

    m_device = m_physical.createDeviceUnique(deviceInfo);
    m_gqueue = m_device.get().getQueue(queueFamilyIndices.graphics, 0);
    m_pqueue = m_device.get().getQueue(queueFamilyIndices.present, 0);
}

vk::PhysicalDevice Device::physicalDevice()
{
    return m_physical;
}

vk::Device Device::logicalDevice()
{
    return m_device.get();
}

}
