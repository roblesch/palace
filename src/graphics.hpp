#pragma once

#include "types.hpp"
#include <vector>
#include <array>

class GraphicsContext {
    struct {
        const char* name = "palace";
        const char* applicationName = "viewer";
        uint32_t apiVersion = VK_API_VERSION_1_1;
    } vk_application_info_;
    struct {
        int x = SDL_WINDOWPOS_UNDEFINED;
        int y = SDL_WINDOWPOS_UNDEFINED;
        int w = 800;
        int h = 600;
        Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
        SDL_Window* window;
        SDL_Window* operator&() { return window; }
    } sdl_window_;
    struct {
        VkDebugUtilsMessageSeverityFlagsEXT messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        VkDebugUtilsMessageTypeFlagsEXT messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        const char* extensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    } vk_debug_messenger_;
    struct {
        std::vector<const char*> extensions;
        VkApplicationInfo applicationInfo;
        VkInstance instance;
        VkInstance* operator&() { return &instance; }
        operator VkInstance() { return instance; }
    } vk_instance_;
    struct {
        VkSurfaceKHR surface;
        VkSurfaceCapabilitiesKHR capabilities;
        uint32_t minWidth;
        uint32_t minHeight;
        uint32_t maxWidth;
        uint32_t maxHeight;
        VkSurfaceKHR* operator&() { return &surface; }
        operator VkSurfaceKHR() { return surface; }
    } vk_surface_khr_;
    struct {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDevice physicalDevice;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkPhysicalDevice* operator&() { return &physicalDevice; }
        operator VkPhysicalDevice() { return physicalDevice; }
    } vk_physical_device_;
    struct {
        uint32_t index;
        VkQueue queue;
        VkQueue* operator&() { return &queue; }
        operator VkQueue() { return queue; }
    } vk_graphics_queue_;
    struct {
        std::vector<const char*> extensions;
        VkDevice device;
        VkDevice* operator&() { return &device; }
        operator VkDevice() { return device; }
    } vk_device_;
    struct {
        uint32_t minImageCount;
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
    } vk_swapchain_khr_;

public:
    GraphicsContext();
    ~GraphicsContext();

    void init();

private:
    void createWindow();
    VkResult createInstance();
    SDL_bool createSurface();
    VkResult createDevice();
    VkResult createSwapchain(VkSwapchainKHR oldSwapchain);
};

class Graphics {
    static GraphicsContext* graphics_context_;

public:
    static void bind(GraphicsContext* graphics_context);
    static void unbind();
    static bool ready();
};
