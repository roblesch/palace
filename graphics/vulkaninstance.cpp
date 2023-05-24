#include "vulkaninstance.hpp"

namespace graphics {

VulkanInstance VulkanInstance::create(SDL_Window* sdlWindow, bool validationEnabled)
{
    VulkanInstance VkInst;

    // sdl2 extensions
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount, nullptr);
    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount, extensionNames.data());

    // molten vk
    vk::InstanceCreateFlagBits flags {};
#ifdef __APPLE__
    extensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    extensionNames.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // validation
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};
    std::vector<const char*> validationLayers;

    if (validationEnabled) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        debugInfo = vkdebug::createInfo();
    }

    // application
    vk::ApplicationInfo appInfo {
        .pApplicationName   = "viewer",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName        = "palace",
        .engineVersion      = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion         = VK_API_VERSION_1_0
    };

    // instance
    vk::InstanceCreateInfo instanceInfo {
        .pNext                   = &debugInfo,
        .flags                   = flags,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = uint32_t(validationLayers.size()),
        .ppEnabledLayerNames     = validationLayers.data(),
        .enabledExtensionCount   = uint32_t(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data()
    };

    VkInst.instance = vk::createInstanceUnique(instanceInfo, nullptr);
    return VkInst;
}

vk::Instance VulkanInstance::get()
{
    return instance.get();
}

}
