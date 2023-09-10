#include "graphics.hpp"

#include <fstream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define VMA_IMPLEMENTATION

/*
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

std::vector<char> readSpirVFile(const std::string& spirVFile)
{
    std::ifstream file(spirVFile, std::ios::binary | std::ios::in | std::ios::ate);

    if (!file.is_open()) {
        printf("(VK_:ERROR) Failed to open spir-v file for reading: %s", spirVFile.c_str());
        exit(1);
    }

    size_t fileSize = file.tellg();
    file.seekg(std::ios::beg);
    std::vector<char> spirVBytes(fileSize);
    file.read(spirVBytes.data(), static_cast<long>(fileSize));
    file.close();

    return spirVBytes;
}

/*
 */

VulkanContext* VulkanContext::singleton = nullptr;

void VulkanContext::init()
{
    singleton = new VulkanContext();
    singleton->createWindow();
    singleton->createInstance();
    singleton->createSurface();
    singleton->createDevice();
    singleton->createCommandPool();
    singleton->createDescriptorLayouts();
    singleton->createVmaAllocator();
    singleton->createSwapchain();
    singleton->createOffScreenResources();
    singleton->createOnScreenResources();
    singleton->createFrameResources();
}

void VulkanContext::cleanup()
{
    delete singleton;
}

VulkanContext* VulkanContext::get()
{
    return singleton;
}

VulkanContext::VulkanContext()
{
}

VulkanContext::~VulkanContext()
{
    for (auto& buffer : vma.buffers)
        vmaDestroyBuffer(vma.allocator, buffer->buffer, buffer->allocation);
    for (auto& image : vma.images)
        vmaDestroyImage(vma.allocator, image->image, image->allocation);

    vmaDestroyAllocator(vma.allocator);

    // TODO: ImGui_ImplVulkan_Shutdown();
    SDL_DestroyWindow(singleton->window);
    SDL_Quit();
}

/*
    Init
*/
void VulkanContext::createWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window = SDL_CreateWindow("Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    SDL_SetWindowMinimumSize(window, 800, 600);
    extent = vk::Extent2D { 800, 600 };
}

void VulkanContext::createInstance()
{
    // sdl2 extensions
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    std::vector<const char*> instanceExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, instanceExtensions.data());

    // molten vk
    vk::InstanceCreateFlagBits flags {};
#ifdef __APPLE__
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    vk::ApplicationInfo appInfo {
        .pApplicationName = "viewer",
        .pEngineName = "palace",
        .apiVersion = VK_API_VERSION_1_1
    };

    std::vector<const char*> validationLayers;
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};

    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    debugInfo = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = debugCallback
    };

    vk::DynamicLoader dl;
    auto getInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

    vk::InstanceCreateInfo instanceInfo {
        .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    instance = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
}

void VulkanContext::createSurface()
{
    VkSurfaceKHR temp;
    SDL_Vulkan_CreateSurface(window, *instance, &temp);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(*instance);
    surface = vk::UniqueSurfaceKHR(temp, deleter);
}

void VulkanContext::createDevice()
{
    for (auto& _physicalDevice : instance->enumeratePhysicalDevices()) {
        vk::PhysicalDeviceProperties deviceProperties = _physicalDevice.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = _physicalDevice.getFeatures();
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            physicalDevice = _physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            physicalDevice = _physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
            physicalDevice = _physicalDevice;
            break;
        }
    }

    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsQueueIndex = i;
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = graphicsQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = new float(0.0f) }
    };

    vk::PhysicalDeviceFeatures deviceFeatures {
        .samplerAnisotropy = VK_TRUE
    };

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
    };

    vk::DeviceCreateInfo deviceInfo {
        .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    device = physicalDevice.createDeviceUnique(deviceInfo);
    graphicsQueue = device->getQueue(graphicsQueueIndex, 0);
}

void VulkanContext::createCommandPool()
{
    vk::CommandPoolCreateInfo commandPoolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueIndex
    };

    commandPool = device->createCommandPoolUnique(commandPoolInfo);
}

void VulkanContext::createDescriptorLayouts()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    vk::DescriptorSetLayoutBinding shadowMapSamplerBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> uboLayoutBindings { uboLayoutBinding, shadowMapSamplerBinding };

    vk::DescriptorSetLayoutCreateInfo uboDescriptorLayoutInfo {
        .bindingCount = static_cast<uint32_t>(uboLayoutBindings.size()),
        .pBindings = uboLayoutBindings.data()
    };

    descriptorLayouts.ubo = device->createDescriptorSetLayoutUnique(uboDescriptorLayoutInfo);

    vk::DescriptorSetLayoutBinding textureSamplerLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    vk::DescriptorSetLayoutBinding normalSamplerLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> materialLayoutBindings { textureSamplerLayoutBinding, normalSamplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo materialDescriptorLayoutInfo {
        .bindingCount = static_cast<uint32_t>(materialLayoutBindings.size()),
        .pBindings = materialLayoutBindings.data()
    };

    descriptorLayouts.material = device->createDescriptorSetLayoutUnique(materialDescriptorLayoutInfo);
}

void VulkanContext::createVmaAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo {
        .physicalDevice = physicalDevice,
        .device = *device,
        .instance = *instance
    };

    vmaCreateAllocator(&allocatorInfo, &vma.allocator);
}

void VulkanContext::createSwapchain(vk::SwapchainKHR oldSwapchain)
{
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    vk::Extent2D swapchainExtent {
        std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    vk::SwapchainCreateInfoKHR swapchainInfo {
        .surface = *surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = colorFormat,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain
    };

    swapchain = device->createSwapchainKHRUnique(swapchainInfo);
}

void VulkanContext::createOffScreenResources()
{
    vk::Extent3D extent { .width = offScreen.shadowResolution, .height = offScreen.shadowResolution, .depth = 1 };

    offScreen.depthImage = createImage(extent, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, 1, vk::SampleCountFlagBits::e1);
    offScreen.depthView = createImageViewUnique(offScreen.depthImage->image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .mipLodBias = 0.0f,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite
    };

    offScreen.depthSampler = device->createSamplerUnique(samplerInfo);

    vk::AttachmentDescription depthAttachment {
        .format = depthFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal
    };

    vk::AttachmentReference depthAttachmentRef {
        .attachment = 0,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency depthWrite {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,
        .dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eShaderRead,
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion
    };

    vk::SubpassDependency shaderRead {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead
    };

    std::array<vk::SubpassDependency, 2> dependencies = { depthWrite, shaderRead };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = 1,
        .pAttachments = &depthAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data()
    };

    offScreen.renderPass = device->createRenderPassUnique(renderPassInfo);

    vk::FramebufferCreateInfo framebufferInfo {
        .renderPass = *offScreen.renderPass,
        .attachmentCount = 1,
        .pAttachments = &offScreen.depthView.get(),
        .width = offScreen.shadowResolution,
        .height = offScreen.shadowResolution,
        .layers = 1
    };

    offScreen.frameBuffer = device->createFramebufferUnique(framebufferInfo);

    std::vector<char> shadowShaderBytes = readSpirVFile("shaders/shadow.spv");

    vk::UniqueShaderModule shadowShaderModule = device->createShaderModuleUnique({ .codeSize = shadowShaderBytes.size(), .pCode = reinterpret_cast<const uint32_t*>(shadowShaderBytes.data()) });

    vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(PushConstants)
    };

    vk::PipelineLayoutCreateInfo shadowPassPipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayouts.ubo.get(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    offScreen.pipelineLayout = device->createPipelineLayoutUnique(shadowPassPipelineLayoutInfo);

    vk::PipelineShaderStageCreateInfo shadowPassStageInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shadowShaderModule,
        .pName = "main"
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .stageCount = 1,
        .pStages = &shadowPassStageInfo,
        //.pVertexInputState = &vertexInputStateInfo,
        //.pInputAssemblyState = &inputAssemblyStateInfo,
        //.pViewportState = &viewportStateInfo,
        //.pRasterizationState = &rasterStateInfo,
        //.pMultisampleState = &multisampleStateInfo,
        //.pDepthStencilState = &depthStencilStateInfo,
        //.pColorBlendState = &colorBlendStateInfo,
        //.pDynamicState = &dynamicStateInfo,
        //.layout = *texturePipeline_.layout,
        //.renderPass = *renderPass_,
        .subpass = 0
    };
    
    // TODO:: OFF-SCREEN PIPELINE
}
/*

    struct OnScreenResources {
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
        VmaImage* depthImage;
        vk::UniqueImageView depthView;
        VmaImage colorImage;
        vk::UniqueImageView colorImageView;
    } onScreen;

*/
void VulkanContext::createOnScreenResources()
{
    vk::AttachmentDescription colorAttachment {
        .format = colorFormat,
        .samples = msaaSampleCount,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .finalLayout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentDescription colorResolve {
        .format = colorFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorResolveRef {
        .attachment = 2,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentDescription depthAttachment {
        .format = depthFormat,
        .samples = msaaSampleCount,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &colorResolveRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorResolve };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    onScreen.renderPass = device->createRenderPassUnique(renderPassInfo);

    // TODO: ON SCREEN PIPELINE LAYOUT, PIPELINE, DEPTH PASS, COLOR IMAGE
}

void VulkanContext::createFrameResources()
{
    uint32_t frameCount = 2;
    frames.resize(frameCount);

    vk::DescriptorPoolSize uboPoolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = frameCount
    };

    vk::DescriptorPoolCreateInfo uboPoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = frameCount,
        .poolSizeCount = 1,
        .pPoolSizes = &uboPoolSize
    };

    descriptorPools.ubo = device->createDescriptorPoolUnique(uboPoolInfo);

    auto swapchainImages = device->getSwapchainImagesKHR(*swapchain);

    for (int i = 0; i < frameCount; i++) {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo {
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        frames[i].commandBuffer = std::move(device->allocateCommandBuffersUnique(commandBufferAllocInfo)[0]);

        frames[i].uniformBuffer.buffer = createBuffer(sizeof(UniformBuffer), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        vk::DescriptorSetAllocateInfo uboDescriptorSetInfo {
            .descriptorPool = *descriptorPools.ubo,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorLayouts.ubo.get()
        };

        frames[i].uniformBuffer.descriptorSet = std::move(device->allocateDescriptorSetsUnique(uboDescriptorSetInfo)[0]);
        vk::DescriptorBufferInfo uboBufferInfo {
            .buffer = frames[i].uniformBuffer.buffer->buffer,
            .offset = 0,
            .range = sizeof(UniformBuffer)
        };

        vk::WriteDescriptorSet sceneUboWriteDescriptor {
            .dstSet = *frames[i].uniformBuffer.descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboBufferInfo
        };

        vk::DescriptorImageInfo shadowMapSamplerInfo {
            .sampler = *offScreen.depthSampler,
            .imageView = *offScreen.depthView,
            .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal
        };

        vk::WriteDescriptorSet shadowMapWriteDescriptor {
            .dstSet = *frames[i].uniformBuffer.descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &shadowMapSamplerInfo
        };

        std::array<vk::WriteDescriptorSet, 2> uboWriteDescriptors = { sceneUboWriteDescriptor, shadowMapWriteDescriptor };
        device->updateDescriptorSets(static_cast<uint32_t>(uboWriteDescriptors.size()), uboWriteDescriptors.data(), 0, nullptr);

        frames[i].swapchainResources.image = swapchainImages[i];
        frames[i].swapchainResources.imageView = createImageViewUnique(swapchainImages[i], colorFormat, vk::ImageAspectFlagBits::eColor, 1);

        // TODO: SWAPCHAIN FRAMEBUFFER, SYNC
    }
}

// TODO: RECREATE SWAPCHAIN

/*
    Utility
*/

vk::UniqueCommandBuffer VulkanContext::beginOneTimeCommandBuffer()
{
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    vk::UniqueCommandBuffer commandBuffer { std::move(device->allocateCommandBuffersUnique(bufferInfo)[0]) };

    commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    return commandBuffer;
}

void VulkanContext::endOneTimeCommandBuffer(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

/*
    Memory Helpers
*/

VmaBuffer* VulkanContext::createBuffer(size_t size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags)
{
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = (VkBufferUsageFlags)usage
    };
    VmaAllocationCreateInfo allocInfo {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    auto buffer = new VmaBuffer;
    buffer->size = size;
    vmaCreateBuffer(vma.allocator, &bufferInfo, &allocInfo, &buffer->buffer, &buffer->allocation, nullptr);
    vma.buffers.push_back(buffer);

    return buffer;
}

void VulkanContext::uploadToBuffer(VmaBuffer* buffer, void* src)
{
    auto staging = createStagingBuffer(buffer->size);

    void* data;
    vmaMapMemory(vma.allocator, staging->allocation, &data);
    memcpy(data, src, buffer->size);
    vmaUnmapMemory(vma.allocator, staging->allocation);

    auto cmd = beginOneTimeCommandBuffer();
    {
        vk::BufferCopy copy {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = buffer->size
        };
        cmd->copyBuffer(staging->buffer, buffer->buffer, 1, &copy);
    }
    endOneTimeCommandBuffer(*cmd);

    vmaDestroyBuffer(vma.allocator, staging->buffer, staging->allocation);
}

void VulkanContext::uploadToBufferDirect(VmaBuffer* buffer, void* src)
{
    void* data;
    vmaMapMemory(vma.allocator, buffer->allocation, &data);
    memcpy(data, src, buffer->size);
    vmaUnmapMemory(vma.allocator, buffer->allocation);
}

VmaImage* VulkanContext::createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels, vk::SampleCountFlagBits samples)
{
    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = (VkFormat)format,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = (VkSampleCountFlagBits)samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = (VkImageUsageFlags)usage
    };
    VmaAllocationCreateInfo imageAllocInfo {
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    auto image = new VmaImage;

    vmaCreateImage(vma.allocator, &imageInfo, &imageAllocInfo, &image->image, &image->allocation, nullptr);
    vma.images.push_back(image);
    return image;
}

VmaImage* VulkanContext::createTextureImage(const void* src, size_t size, vk::Extent3D extent, uint32_t mipLevels)
{
    // upload to staging
    auto staging = createStagingBuffer(size);
    void* data;
    vmaMapMemory(vma.allocator, staging->allocation, &data);
    memcpy(data, src, size);
    vmaUnmapMemory(vma.allocator, staging->allocation);

    // create image
    auto texture = createImage(extent, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, mipLevels, vk::SampleCountFlagBits::e1);

    // transition staging format
    auto cmd = beginOneTimeCommandBuffer();
    {
        auto transferBarrier = imageTransitionBarrier(texture->image, {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, transferBarrier);

        // copy to image
        vk::BufferImageCopy copy {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1 },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = extent
        };
        cmd->copyBufferToImage(staging->buffer, texture->image, vk::ImageLayout::eTransferDstOptimal, copy);

        // check if filter supported
        if (!(physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm).optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        }

        // generate mipmaps
        vk::ImageMemoryBarrier barrier {
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = texture->image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1 }
        };

        uint32_t mipWidth = extent.width;
        uint32_t mipHeight = extent.height;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

            vk::ImageBlit blit {
                .srcSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = i - 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1 },
                .dstSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1 }
            };
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { static_cast<int>(mipWidth), static_cast<int>(mipHeight), 1 };
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { static_cast<int>(mipWidth > 1 ? mipWidth / 2 : 1), static_cast<int>(mipHeight > 1 ? mipHeight / 2 : 1), 1 };

            cmd->blitImage(texture->image, vk::ImageLayout::eTransferSrcOptimal, texture->image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

            mipWidth = mipWidth > 1 ? mipWidth / 2 : mipWidth;
            mipHeight = mipHeight > 1 ? mipHeight / 2 : mipHeight;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
    }
    endOneTimeCommandBuffer(*cmd);
    vmaDestroyBuffer(vma.allocator, staging->buffer, staging->allocation);

    return texture;
}

vk::UniqueImageView VulkanContext::createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask, uint32_t mipLevels)
{
    vk::ImageViewCreateInfo imageViewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    return device->createImageViewUnique(imageViewInfo);
}

vk::UniqueSampler VulkanContext::createTextureSamplerUnique(uint32_t mipLevels)
{
    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = physicalDevice.getProperties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevels),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE
    };

    return device->createSamplerUnique(samplerInfo);
}

VmaBuffer* VulkanContext::createStagingBuffer(size_t size)
{
    VkBufferCreateInfo stagingBufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    auto staging = new VmaBuffer;
    vmaCreateBuffer(vma.allocator, &stagingBufferInfo, &stagingAllocInfo, &staging->buffer, &staging->allocation, nullptr);
    return staging;
}

vk::ImageMemoryBarrier VulkanContext::imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
{
    return {
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };
}

/*
 */

void gfx::init()
{
    VulkanContext::init();
}

void gfx::cleanup()
{
    VulkanContext::cleanup();
}

VulkanContext* gfx::get()
{
    return VulkanContext::get();
}
