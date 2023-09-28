#pragma once

#include "types.hpp"
#include <vector>

struct Application {
    const char* name = "palace";
    const char* applicationName = "viewer";
    uint32_t apiVersion = VK_API_VERSION_1_3;

    VkApplicationInfo* info()
    {
        return new VkApplicationInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = applicationName,
            .pEngineName = name,
            .apiVersion = apiVersion
        };
    }
};

struct Window {
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = 800;
    int h = 600;
    Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
    SDL_Window* window;

    operator SDL_Window*() { return window; }

    void create(const char* title)
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_Vulkan_LoadLibrary(nullptr);
        window = SDL_CreateWindow(title, x, y, w, h, flags);
        SDL_SetWindowMinimumSize(window, w, h);
    }
};

struct DebugMessenger {
    VkDebugUtilsMessageSeverityFlagsEXT messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    VkDebugUtilsMessageTypeFlagsEXT messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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

    VkDebugUtilsMessengerCreateInfoEXT* info()
    {
        return new VkDebugUtilsMessengerCreateInfoEXT {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = messageSeverity,
            .messageType = messageType,
            .pfnUserCallback = debugCallback
        };
    }
};

struct Instance {
    uint32_t extensionCount;
    std::vector<const char*> extensions;
    VkInstanceCreateFlags flags {};
    VkApplicationInfo applicationInfo;
    VkInstance instance;

    VkInstance* operator&() { return &instance; }

    operator VkInstance() { return instance; }

    VkResult create(SDL_Window* window, const void* pNext, VkApplicationInfo* applicationInfo)
    {
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
        extensions.resize(extensionCount);
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef __APPLE__
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        flags = VkInstanceCreateFlagBits::VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkInstanceCreateInfo instanceInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = pNext,
            .flags = flags,
            .pApplicationInfo = applicationInfo,
            .enabledLayerCount = 1,
            .ppEnabledLayerNames = new const char * ("VK_LAYER_KHRONOS_validation"),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        return vkCreateInstance(&instanceInfo, nullptr, &instance);
    }
};

struct Surface {
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceKHR* operator&() { return &surface; }

    operator VkSurfaceKHR() { return surface; }

    SDL_bool create(SDL_Window* window, VkInstance instance)
    {
        return SDL_Vulkan_CreateSurface(window, instance, &surface);
    }
};

struct PhysicalDevice {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDevice physicalDevice;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    VkPhysicalDevice* operator&() { return &physicalDevice; }
    operator VkPhysicalDevice() { return physicalDevice; }
};

struct GraphicsQueue {
    uint32_t queueFamilyIndex;
    VkQueue queue;
    VkQueue* operator&() { return &queue; }
    operator VkQueue() { return queue; }
};

struct Device {
    std::vector<const char*> extensions;
    VkDevice device;
    VkDevice* operator&() { return &device; }
    operator VkDevice() { return device; }
};

struct Swapchain {
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkExtent2D extent2D;
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkBool32 clipped = VK_TRUE;
    VkSwapchainKHR swapchain;
    VkSwapchainKHR* operator&() { return &swapchain; }
    operator VkSwapchainKHR() { return swapchain; }
};

struct CommandPool {
    VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commandPool;
    VkCommandPool* operator&() { return &commandPool; }
    operator VkCommandPool() { return commandPool; }
};

struct CommandBuffer {
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
};
