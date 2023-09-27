#include "graphics.hpp"

#include <span>
#include <utility>

/*
 *
 */

GraphicsContext::GraphicsContext()
{
}

GraphicsContext::~GraphicsContext()
{
}

/*
 *
 */

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    char prefix[64] = "VK:";

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        strcat(prefix, "VERBOSE:");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        strcat(prefix, "INFO:");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        strcat(prefix, "WARNING:");
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        strcat(prefix, "ERROR:");
    }

    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        strcat(prefix, "GENERAL");
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        strcat(prefix, "VALIDATION");
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        strcat(prefix, "PERFORMANCE");
    }

    printf("(%s) %s\n", prefix, pCallbackData->pMessage);
    return VK_FALSE;
}

void GraphicsContext::init()
{
    createWindow();
    createInstance();
    createSurface();
    createDevice();
    createSwapchain(nullptr);
}

void GraphicsContext::createWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);

    sdl_window_.window = SDL_CreateWindow(vk_application_info_.applicationName, sdl_window_.x, sdl_window_.y, sdl_window_.w, sdl_window_.h, sdl_window_.flags);
    SDL_SetWindowMinimumSize(&sdl_window_, 800, 600);
}

VkResult GraphicsContext::createInstance()
{
    uint32_t extensionCount;
    SDL_Vulkan_GetInstanceExtensions(&sdl_window_, &extensionCount, nullptr);
    vk_instance_.extensions.resize(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(&sdl_window_, &extensionCount, vk_instance_.extensions.data());

    VkInstanceCreateFlagBits flags {};
#ifdef __APPLE__
    vk_instance_.extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    vk_instance_.extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = VkInstanceCreateFlagBits::VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    vk_instance_.extensions.push_back(vk_debug_messenger_.extensionName);

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = vk_debug_messenger_.messageSeverity,
        .messageType = vk_debug_messenger_.messageType,
        .pfnUserCallback = debugCallback
    };

    vk_instance_.applicationInfo = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = vk_application_info_.applicationName,
        .pEngineName = vk_application_info_.name,
        .apiVersion = vk_application_info_.apiVersion
    };

    VkInstanceCreateInfo instanceInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &vk_instance_.applicationInfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = new const char*("VK_LAYER_KHRONOS_validation"),
        .enabledExtensionCount = static_cast<uint32_t>(vk_instance_.extensions.size()),
        .ppEnabledExtensionNames = vk_instance_.extensions.data()
    };

    return vkCreateInstance(&instanceInfo, nullptr, &vk_instance_);
}

SDL_bool GraphicsContext::createSurface()
{
    return SDL_Vulkan_CreateSurface(&sdl_window_, vk_instance_, &vk_surface_khr_);
}

VkResult GraphicsContext::createDevice()
{
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(vk_instance_, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vk_instance_, &physicalDeviceCount, physicalDevices.data());

    for (VkPhysicalDevice physicalDevice : physicalDevices) {
        vkGetPhysicalDeviceProperties(physicalDevice, &vk_physical_device_.properties);
        VkPhysicalDeviceType deviceType = vk_physical_device_.properties.deviceType;
        if (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vk_physical_device_.physicalDevice = physicalDevice;
            break;
        }
        if (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            vk_physical_device_.physicalDevice = physicalDevice;
            break;
        }
        if (deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            vk_physical_device_.physicalDevice = physicalDevice;
            break;
        }
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_khr_, &vk_surface_khr_.capabilities);
    vk_surface_khr_.minWidth = vk_surface_khr_.capabilities.minImageExtent.width;
    vk_surface_khr_.minHeight = vk_surface_khr_.capabilities.minImageExtent.height;
    vk_surface_khr_.maxWidth = vk_surface_khr_.capabilities.maxImageExtent.width;
    vk_surface_khr_.maxHeight = vk_surface_khr_.capabilities.maxImageExtent.height;

    uint32_t queuePropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queuePropertyCount, nullptr);
    vk_physical_device_.queueFamilyProperties.resize(queuePropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_.physicalDevice, &queuePropertyCount, vk_physical_device_.queueFamilyProperties.data());

    for (uint32_t i = 0; i < queuePropertyCount; i++) {
        if (vk_physical_device_.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vk_graphics_queue_.index = i;
            break;
        }
    }

    VkDeviceQueueCreateInfo queueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk_graphics_queue_.index,
        .queueCount = 1,
        .pQueuePriorities = new float(0.0f)
    };

    vk_device_.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef __APPLE__
    vk_device_.extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    VkDeviceCreateInfo deviceInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(vk_device_.extensions.size()),
        .ppEnabledExtensionNames = vk_device_.extensions.data(),
    };

    VkResult result = vkCreateDevice(vk_physical_device_, &deviceInfo, nullptr, &vk_device_);
    vkGetDeviceQueue(vk_device_, vk_graphics_queue_.index, 0, &vk_graphics_queue_);
    return result;
}

VkResult GraphicsContext::createSwapchain(VkSwapchainKHR oldSwapchain)
{
    SDL_GetWindowSize(sdl_window_.window, &sdl_window_.w, &sdl_window_.h);
    vk_swapchain_khr_.minImageCount = vk_surface_khr_.capabilities.minImageCount;

    vk_swapchain_khr_.extent2D = VkExtent2D {
        .width = std::clamp(static_cast<uint32_t>(sdl_window_.w), vk_surface_khr_.minWidth, vk_surface_khr_.maxHeight),
        .height = std::clamp(static_cast<uint32_t>(sdl_window_.h), vk_surface_khr_.minHeight, vk_surface_khr_.maxHeight)
    };

    VkSwapchainCreateInfoKHR swapchainInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_surface_khr_,
        .minImageCount = vk_swapchain_khr_.minImageCount,
        .imageFormat = vk_swapchain_khr_.colorFormat,
        .imageExtent = vk_swapchain_khr_.extent2D,
        .imageArrayLayers = 1,
        .imageUsage = vk_swapchain_khr_.imageUsage,
        .imageSharingMode = vk_swapchain_khr_.imageSharingMode,
        .preTransform = vk_swapchain_khr_.preTransform,
        .compositeAlpha = vk_swapchain_khr_.compositeAlpha,
        .presentMode = vk_swapchain_khr_.presentMode,
        .clipped = vk_swapchain_khr_.clipped,
        .oldSwapchain = oldSwapchain
    };

    return vkCreateSwapchainKHR(vk_device_, &swapchainInfo, nullptr, &vk_swapchain_khr_);
}
/*
 *
 */

GraphicsContext* Graphics::graphics_context_ = nullptr;

void Graphics::bind(GraphicsContext* context)
{
    graphics_context_ = context;
}

void Graphics::unbind()
{
    graphics_context_ = nullptr;
}

bool Graphics::ready()
{
    return graphics_context_ != nullptr;
}
