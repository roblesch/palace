#include "vulkandevice.hpp"

namespace graphics::vulkan {

device::device(vk::Instance instance, vk::SurfaceKHR surface)
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

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_pdevice, i, surface, &presentSupport);

        if (presentSupport) {
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

    std::vector<const char*> extensionNames;
#ifdef __APPLE__
    extensionNames.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    vk::DeviceCreateInfo deviceInfo {
        .queueCreateInfoCount = uint32_t(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = uint32_t(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
        .pEnabledFeatures = {}
    };

    m_ldevice = m_pdevice.createDeviceUnique(deviceInfo);

    vkGetDeviceQueue(m_ldevice.get(), queueFamilyIndices.graphics, 0, (VkQueue*)&m_gqueue);
    vkGetDeviceQueue(m_ldevice.get(), queueFamilyIndices.present, 0, (VkQueue*)&m_pqueue);
}

}
