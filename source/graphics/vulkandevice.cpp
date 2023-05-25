#include "vulkandevice.hpp"

namespace graphics::vulkan {

device::device(vk::Instance instance)
{
    // select a physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_pdevice = device;
            break;
        } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            m_pdevice = device;
            break;
        } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            m_pdevice = device;
            break;
        }
    }

    // fill queue family indices
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_pdevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_pdevice, &queueFamilyCount, queueFamilies.data());

    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices.graphics = i;
            break;
        }
    }

    // create a logical device
    vk::DeviceQueueCreateInfo queueInfo {
        .queueFamilyIndex = queueFamilyIndices.graphics,
        .queueCount = 1,
        .pQueuePriorities = new float(1.0f)
    };

    vk::PhysicalDeviceFeatures deviceFeatures {};

    uint32_t extensionCount = 0;
    std::vector<const char*> extensionNames;

#ifdef __APPLE__
    extensionCount = 1;
    extensionNames.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    vk::DeviceCreateInfo deviceInfo {
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensionNames.data(),
        .pEnabledFeatures = {}
    };

    m_ldevice = m_pdevice.createDeviceUnique(deviceInfo);
}

}
