#include "graphics.hpp"

#include <algorithm>
#include <vulkan/vulkan_core.h>

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

void GraphicsContext::init()
{
    sdl_window_.create(vk_application_.applicationName);
    vk_instance_.create(sdl_window_, vk_debug_messenger_.info(), vk_application_.info());
    createWindow();
    createInstance();
    createSurface();
    createDevice();
    createSwapchain(nullptr);
    createImGui();
}

void GraphicsContext::cleanup()
{

}

/*
*/

void GraphicsContext::createWindow()
{
    
}

VkResult GraphicsContext::createInstance()
{
    return VK_SUCCESS;
}

SDL_bool GraphicsContext::createSurface()
{
    return vk_surface_khr_.create(sdl_window_, vk_instance_);
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

    uint32_t queuePropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_, &queuePropertyCount, nullptr);
    vk_physical_device_.queueFamilyProperties.resize(queuePropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device_.physicalDevice, &queuePropertyCount, vk_physical_device_.queueFamilyProperties.data());

    for (uint32_t i = 0; i < queuePropertyCount; i++) {
        if (vk_physical_device_.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vk_graphics_queue_.queueFamilyIndex = i;
            break;
        }
    }

    VkDeviceQueueCreateInfo queueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk_graphics_queue_.queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = new float(0.0f)
    };

    vk_device_.extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    vk_device_.extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#ifdef __APPLE__
    vk_device_.extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE
    };

    VkDeviceCreateInfo deviceInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRendering,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(vk_device_.extensions.size()),
        .ppEnabledExtensionNames = vk_device_.extensions.data(),
    };

    VkResult result = vkCreateDevice(vk_physical_device_, &deviceInfo, nullptr, &vk_device_);
    vkGetDeviceQueue(vk_device_, vk_graphics_queue_.queueFamilyIndex, 0, &vk_graphics_queue_);
    return result;
}

VkResult GraphicsContext::createSwapchain(VkSwapchainKHR oldSwapchain)
{
    SDL_GetWindowSize(sdl_window_.window, &sdl_window_.w, &sdl_window_.h);
    VkExtent2D& minExtent = vk_surface_khr_.capabilities.minImageExtent;
    VkExtent2D& maxExtent = vk_surface_khr_.capabilities.maxImageExtent;

    vk_swapchain_khr_.extent2D = VkExtent2D {
        .width = std::clamp(static_cast<uint32_t>(sdl_window_.w), minExtent.width, maxExtent.width),
        .height = std::clamp(static_cast<uint32_t>(sdl_window_.h), minExtent.height, maxExtent.height)
    };

    VkSwapchainCreateInfoKHR swapchainInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_surface_khr_,
        .minImageCount = vk_surface_khr_.capabilities.minImageCount,
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

VkResult GraphicsContext::createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = vk_command_pool_.flags,
        .queueFamilyIndex = vk_graphics_queue_.queueFamilyIndex,
    };

    return vkCreateCommandPool(vk_device_, &commandPoolInfo, nullptr, &vk_command_pool_);
}

void GraphicsContext::createImGui()
{
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = new VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1 }
    };

    vkCreateDescriptorPool(vk_device_, &pool_info, nullptr, &imgui_.descriptorPool);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(sdl_window_);

    ImGui_ImplVulkan_InitInfo imguiInfo {
        .Instance = vk_instance_,
        .PhysicalDevice = vk_physical_device_,
        .Device = vk_device_,
        .Queue = vk_graphics_queue_,
        .DescriptorPool = imgui_.descriptorPool,
        .MinImageCount = vk_surface_khr_.capabilities.minImageCount,
        .ImageCount = vk_surface_khr_.capabilities.minImageCount,
        .MSAASamples = VK_SAMPLE_COUNT_4_BIT,
        .UseDynamicRendering = true,
        .ColorAttachmentFormat = vk_swapchain_khr_.colorFormat
    };

    ImGui_ImplVulkan_Init(&imguiInfo, nullptr);
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
