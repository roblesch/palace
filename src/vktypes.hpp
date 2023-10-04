#pragma once

#include "types.hpp"
#include <algorithm>
#include <fstream>
#include <span>
#include <vector>

#define TIMEOUT 1000000000

struct Application;
struct Window;
struct DebugMessenger;
struct Instance;
struct PhsyicalDevice;
struct Surface;
struct GraphicsQueue;
struct Device;
struct Semaphore;
struct Fence;
struct Swapchain;
struct CommandPool;
struct CommandBuffer;
struct Pipeline;

struct Application {
    const char* engineName = "palace";
    const char* applicationName = "viewer";
    uint32_t apiVersion = VK_API_VERSION_1_3;

    VkApplicationInfo* createInfo()
    {
        return new VkApplicationInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = applicationName,
            .pEngineName = engineName,
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

    void destroy()
    {
        SDL_DestroyWindow(window);
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

    VkDebugUtilsMessengerCreateInfoEXT* createInfo()
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

    VkResult create(Window& window, DebugMessenger& debug, Application& application)
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

        VkInstanceCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = debug.createInfo(),
            .flags = flags,
            .pApplicationInfo = application.createInfo(),
            .enabledLayerCount = 1,
            .ppEnabledLayerNames = new const char * ("VK_LAYER_KHRONOS_validation"),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        return vkCreateInstance(&createInfo, nullptr, &instance);
    }

    void destroy()
    {
        vkDestroyInstance(instance, nullptr);
    }
};

struct PhysicalDevice {
    uint32_t count;
    std::vector<VkPhysicalDevice> physicalDevices;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDevice physicalDevice;

    VkPhysicalDevice* operator&() { return &physicalDevice; }

    operator VkPhysicalDevice() { return physicalDevice; }

    VkResult create(Instance& instance)
    {
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        physicalDevices.resize(count);
        vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data());

        for (VkPhysicalDevice physicalDeviceItr : physicalDevices) {
            vkGetPhysicalDeviceProperties(physicalDeviceItr, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physicalDevice = physicalDeviceItr;
                return VK_SUCCESS;
            }
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                physicalDevice = physicalDeviceItr;
                return VK_SUCCESS;
            }
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
                physicalDevice = physicalDeviceItr;
                return VK_SUCCESS;
            }
        }

        return VK_ERROR_INITIALIZATION_FAILED;
    }
};

struct Surface {
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceKHR* operator&() { return &surface; }

    operator VkSurfaceKHR() { return surface; }

    SDL_bool create(Window& window, Instance& instance, PhysicalDevice& physicalDevice)
    {
        SDL_bool result = SDL_Vulkan_CreateSurface(window, instance, &surface);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        return result;
    }

    void destroy(Instance& instance)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
};

struct GraphicsQueue {
    uint32_t familyCount;
    std::vector<VkQueueFamilyProperties> familyProperties;
    uint32_t familyIndex;
    VkQueue graphicsQueue;

    VkQueue* operator&() { return &graphicsQueue; }

    operator VkQueue() { return graphicsQueue; }

    VkDeviceQueueCreateInfo* info()
    {
        return new VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = familyIndex,
            .queueCount = 1,
            .pQueuePriorities = new float(0.0f)
        };
    }

    VkResult create(PhysicalDevice& physicalDevice)
    {
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
        familyProperties.resize(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, familyProperties.data());

        for (uint32_t i = 0; i < familyCount; i++) {
            if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                familyIndex = i;
                return VK_SUCCESS;
            }
        }

        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult submit(Semaphore& waitSemaphore, CommandBuffer& commandBuffer, Semaphore& signalSemaphore, Fence& fence);
    VkResult present(Semaphore& waitSemaphore, Swapchain& swapchain, uint32_t imageIndex);
};

struct Device {
    std::vector<const char*> extensions;
    VkDevice device;

    VkDevice* operator&() { return &device; }

    operator VkDevice() { return device; }

    VkResult create(PhysicalDevice& physicalDevice, GraphicsQueue& queue)
    {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#ifdef __APPLE__
        deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .dynamicRendering = VK_TRUE
        };

        VkDeviceCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &dynamicRendering,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = queue.info(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        vkGetDeviceQueue(device, queue.familyIndex, 0, &queue.graphicsQueue);
        return result;
    }

    void destroy()
    {
        vkDestroyDevice(device, nullptr);
    }
};

struct Semaphore {
    VkSemaphore semaphore;

    VkSemaphore* operator&() { return &semaphore; }

    operator VkSemaphore() { return semaphore; }

    VkResult create(Device& device)
    {
        VkSemaphoreCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = 0
        };

        return vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
    }

    void destroy(Device& device)
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
};

struct Swapchain {
    uint32_t imageCount;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkClearValue clearValue = { 0.0f };
    VkExtent2D extent2D;
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkBool32 clipped = VK_TRUE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    VkSwapchainKHR* operator&() { return &swapchain; }

    operator VkSwapchainKHR() { return swapchain; }

    VkResult create(Device& device, Window& window, Surface& surface)
    {
        SDL_GetWindowSize(window.window, &window.w, &window.h);
        VkExtent2D& minExtent = surface.capabilities.minImageExtent;
        VkExtent2D& maxExtent = surface.capabilities.maxImageExtent;

        extent2D = VkExtent2D {
            .width = std::clamp(static_cast<uint32_t>(window.w), minExtent.width, maxExtent.width),
            .height = std::clamp(static_cast<uint32_t>(window.h), minExtent.height, maxExtent.height)
        };

        imageCount = surface.capabilities.minImageCount;
        images.resize(imageCount);
        imageViews.resize(imageCount);

        VkSwapchainCreateInfoKHR createInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = colorFormat,
            .imageExtent = extent2D,
            .imageArrayLayers = 1,
            .imageUsage = imageUsage,
            .imageSharingMode = imageSharingMode,
            .preTransform = preTransform,
            .compositeAlpha = compositeAlpha,
            .presentMode = presentMode,
            .clipped = clipped,
            .oldSwapchain = swapchain
        };

        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

        for (int i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo imageViewInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = colorFormat,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1 }
            };

            result = vkCreateImageView(device, &imageViewInfo, nullptr, &imageViews[i]);
        }

        return result;
    }

    void destroy(Device& device)
    {
        for (int i = 0; i < imageCount; i++) {
            vkDestroyImageView(device, imageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    VkResult acquireNextImage(Device& device, Semaphore& semaphore, uint32_t* index);

    VkRenderingInfoKHR* renderingInfo(uint32_t imageIndex)
    {
        return new VkRenderingInfoKHR {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = {
                .offset = { 0, 0 },
                .extent = extent2D },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = new VkRenderingAttachmentInfoKHR {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = imageViews[imageIndex],
                .imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = clearValue
            }
        };
    }

    VkImageMemoryBarrier* imagePresentBarrier(uint32_t imageIndex)
    {
        return new VkImageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = images[imageIndex],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };
    }

    VkImageMemoryBarrier* imageWriteBarrier(uint32_t imageIndex)
    {
        return new VkImageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = images[imageIndex],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };
    }
};

struct ShaderModule {
    VkShaderModule shaderModule;
    VkShaderStageFlagBits stage;

    VkShaderModule* operator&() { return &shaderModule; }

    operator VkShaderModule() { return shaderModule; }

    VkResult create(Device& device, const char* filename, VkShaderStageFlagBits stage)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);

        if (!file.is_open())
            return VK_ERROR_INVALID_SHADER_NV;

        size_t size = file.tellg();
        file.seekg(std::ios::beg);
        std::vector<char> buffer(size);
        file.read(buffer.data(), static_cast<long>(size));
        file.close();

        VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buffer.size(),
            .pCode = reinterpret_cast<const uint32_t*>(buffer.data())
        };

        this->stage = stage;

        return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    }

    void destroy(Device& device)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo()
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage,
            .module = shaderModule,
            .pName = "main"
        };
    }
};

struct PipelineLayout {
    VkPipelineLayout pipelineLayout;

    VkPipelineLayout* operator&() { return &pipelineLayout; }

    operator VkPipelineLayout() { return pipelineLayout; }

    VkResult create(Device& device)
    {
        VkPipelineLayoutCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        return vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout);
    }

    void destroy(Device& device)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
};

struct Viewport {
    VkViewport viewport;
    VkRect2D scissor;

    void create(VkExtent2D extent)
    {
        viewport = {
            .x = 0.0f,
            .y = (float)extent.height,
            .width = (float)extent.width,
            .height = (float)extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        scissor = {
            .offset = { 0, 0 },
            .extent = extent
        };
    }
};

struct Pipeline {
    VkPipeline pipeline;

    VkResult create(Device& device, PipelineLayout& layout, Viewport& viewport, VkFormat colorFormat, std::vector<VkPipelineShaderStageCreateInfo> shaderStages)
    {
        VkPipelineVertexInputStateCreateInfo vertexStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        VkPipelineRasterizationStateCreateInfo rasterStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampleStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        VkPipelineViewportStateCreateInfo viewportStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport.viewport,
            .scissorCount = 1,
            .pScissors = &viewport.scissor,
        };

        VkPipelineColorBlendStateCreateInfo colorBlendStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };

        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorFormat
        };

        VkGraphicsPipelineCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipelineRenderingInfo,
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexStateInfo,
            .pInputAssemblyState = &inputAssemblyStateInfo,
            .pViewportState = &viewportStateInfo,
            .pRasterizationState = &rasterStateInfo,
            .pMultisampleState = &multisampleStateInfo,
            .pColorBlendState = &colorBlendStateInfo,
            .layout = layout,
            .renderPass = nullptr
        };

        return vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline);
    }

    void destroy(Device& device)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
};

struct CommandPool {
    VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commandPool;

    VkCommandPool* operator&() { return &commandPool; }

    operator VkCommandPool() { return commandPool; }

    VkResult create(Device& device, uint32_t queueFamilyIndex)
    {
        VkCommandPoolCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex
        };

        return vkCreateCommandPool(device, &createInfo, nullptr, &commandPool);
    }

    void destroy(Device& device)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }
};

struct CommandBuffer {
    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VkCommandBuffer commandBuffer;

    VkCommandBuffer* operator&() { return &commandBuffer; }

    operator VkCommandBuffer() { return commandBuffer; }

    VkResult create(Device& device, Instance& instance, CommandPool& commandPool, VkCommandBufferLevel level)
    {
        vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
        vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");

        VkCommandBufferAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = level,
            .commandBufferCount = 1
        };

        return vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);
    }

    VkResult reset()
    {
        return vkResetCommandBuffer(commandBuffer, 0);
    }

    VkResult begin()
    {
        VkCommandBufferBeginInfo beginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        return vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }

    void beginRendering(VkRenderingInfo* renderingInfo)
    {
        vkCmdBeginRendering(commandBuffer, renderingInfo);
    }

    void endRendering()
    {
        vkCmdEndRendering(commandBuffer);
    }

    void writeBarrier(VkImageMemoryBarrier* imageMemoryBarrier)
    {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, imageMemoryBarrier);
    }

    void presentBarrier(VkImageMemoryBarrier* imageMemoryBarrier)
    {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, imageMemoryBarrier);
    }

    VkResult end()
    {
        return vkEndCommandBuffer(commandBuffer);
    }
};

struct Fence {
    VkFence fence;

    VkFence* operator&() { return &fence; }

    operator VkFence() { return fence; }

    VkResult create(Device& device)
    {
        VkFenceCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };

        return vkCreateFence(device, &createInfo, nullptr, &fence);
    }

    void destroy(Device& device)
    {
        vkDestroyFence(device, fence, nullptr);
    }

    VkResult wait(Device& device)
    {
        return vkWaitForFences(device, 1, &fence, true, TIMEOUT);
    }

    VkResult reset(Device& device)
    {
        return vkResetFences(device, 1, &fence);
    }
};
