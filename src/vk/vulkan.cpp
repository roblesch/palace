#include "vulkan.hpp"

#include "util/log.hpp"
#include "gltf/scene.hpp"
#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace pl {

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
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
        printf("(VK_:ERROR) Pipeline failed to open spir-v file for reading: %s", spirVFile.c_str());
        exit(1);
    }

    size_t fileSize = file.tellg();
    file.seekg(std::ios::beg);
    std::vector<char> spirVBytes(fileSize);
    file.read(spirVBytes.data(), fileSize);
    file.close();

    return spirVBytes;
}

Vulkan::Vulkan(bool enableValidation)
    : m_isValidationEnabled(enableValidation)
{
    // dynamic dispatch loader
    auto vkGetInstanceProcAddr = m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    m_window = SDL_CreateWindow("palace", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        s_windowWidth, s_windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    SDL_SetWindowMinimumSize(m_window, 400, 300);
    m_extent2D = vk::Extent2D {
        .width = s_windowWidth,
        .height = s_windowHeight
    };

    // sdl2
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, nullptr);
    std::vector<const char*> instanceExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, instanceExtensions.data());

    // molten vk
    vk::InstanceCreateFlagBits flags {};
#ifdef __APPLE__
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // application
    vk::ApplicationInfo appInfo {
        .pApplicationName = "viewer",
        .pEngineName = "palace",
        .apiVersion = VK_API_VERSION_1_0
    };

    

    // validation
    std::vector<const char*> validationLayers;
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};

    if (m_isValidationEnabled) {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        debugInfo = {
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debugCallback
        };
    }

    // instance
    vk::InstanceCreateInfo instanceInfo {
        .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };
    m_instance = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

    // surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, *m_instance, &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(*m_instance);
    m_surface = vk::UniqueSurfaceKHR(surface, deleter);

    // physical device
    for (auto& physicalDevice : m_instance->enumeratePhysicalDevices()) {
        vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            m_physicalDevice = physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            m_physicalDevice = physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
            m_physicalDevice = physicalDevice;
            break;
        }
    }

    // queue families
    std::vector<vk::QueueFamilyProperties> queueFamilies = m_physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            m_queueFamilyIndices.graphics = i;
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = m_queueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = new float(0.0f) }
    };

    // device
    vk::PhysicalDeviceFeatures deviceFeatures { .samplerAnisotropy = VK_TRUE };
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

    m_device = m_physicalDevice.createDeviceUnique(deviceInfo);

    // graphics queue
    m_graphicsQueue = m_device->getQueue(m_queueFamilyIndices.graphics, 0);

    // command pool
    vk::CommandPoolCreateInfo commandPoolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_queueFamilyIndices.graphics
    };

    m_commandPool = m_device->createCommandPoolUnique(commandPoolInfo);

    // command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo {
        .commandPool = *m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = s_concurrentFrames
    };

    m_commandBuffers = m_device->allocateCommandBuffersUnique(commandBufferAllocInfo);

    // sync
    for (size_t i = 0; i < s_concurrentFrames; i++) {
        m_imageAvailableSemaphores.push_back(m_device->createSemaphoreUnique({}));
        m_renderFinishedSemaphores.push_back(m_device->createSemaphoreUnique({}));
        m_inFlightFences.push_back(m_device->createFenceUnique({ .flags = vk::FenceCreateFlagBits::eSignaled }));
    }

    // uniform modelview transforms
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    // image sampler
    vk::DescriptorSetLayoutBinding samplerLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    // layout
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings { uboLayoutBinding, samplerLayoutBinding };
    vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo {
        .bindingCount = 2,
        .pBindings = bindings.data()
    };

    m_descriptorSetLayout = m_device->createDescriptorSetLayoutUnique(descriptorLayoutInfo);

    // descriptor pools
    vk::DescriptorPoolSize uboSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = s_concurrentFrames
    };

    vk::DescriptorPoolSize samplerSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = s_concurrentFrames
    };

    std::array<vk::DescriptorPoolSize, 2> pipelinePoolSizes { uboSize, samplerSize };

    vk::DescriptorPoolCreateInfo pipelinePoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = s_concurrentFrames,
        .poolSizeCount = 2,
        .pPoolSizes = pipelinePoolSizes.data()
    };

    m_descriptorPool = m_device->createDescriptorPoolUnique(pipelinePoolInfo);

    // descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(s_concurrentFrames, *m_descriptorSetLayout);

    vk::DescriptorSetAllocateInfo descriptorSetInfo {
        .descriptorPool = *m_descriptorPool,
        .descriptorSetCount = s_concurrentFrames,
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets = m_device->allocateDescriptorSetsUnique(descriptorSetInfo);

    // shaders
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    auto vertexShaderModule = m_device->createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    auto fragmentShaderModule = m_device->createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.data()) });

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos {
        { .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertexShaderModule,
            .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragmentShaderModule,
            .pName = "main" }
    };

    // vertex input
    vk::VertexInputBindingDescription vertexBindingDescription {
        .binding = 0,
        .stride = sizeof(pl::Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };

    vk::VertexInputAttributeDescription posDescription {
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, pos)
    };
    vk::VertexInputAttributeDescription colorDescription {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, color)
    };
    vk::VertexInputAttributeDescription texCoordDescription {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(pl::Vertex, texCoord)
    };
    std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescriptions { posDescription, colorDescription, texCoordDescription };

    vk::PipelineVertexInputStateCreateInfo vertexStateInfo {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingDescription,
        .vertexAttributeDescriptionCount = vertexAttributeDescriptions.size(),
        .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
    };

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport state
    vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_extent2D.width),
        .height = static_cast<float>(m_extent2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor {
        .offset = { 0, 0 },
        .extent = m_extent2D
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // rasterization state
    // TODO: debugging toggle mode
    vk::PipelineRasterizationStateCreateInfo rasterStateInfo {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // multisample state
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE
    };

    // color blend state
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo {
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // depth state
    vk::PipelineDepthStencilStateCreateInfo depthStateInfo {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // dynamic state
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &(*m_descriptorSetLayout)
    };
    m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

    // attachments
    vk::AttachmentDescription colorAttachment {
        .format = vk::Format::eB8G8R8A8Unorm,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentDescription depthAttachment {
        .format = vk::Format::eD32Sfloat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    // subpass
    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    // renderpass
    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    m_renderPass = m_device->createRenderPassUnique(renderPassInfo);

    // pipeline
    vk::GraphicsPipelineCreateInfo graphicsPipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .pVertexInputState = &vertexStateInfo,
        .pInputAssemblyState = &assemblyStateInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterStateInfo,
        .pMultisampleState = &multisampleStateInfo,
        .pDepthStencilState = &depthStateInfo,
        .pColorBlendState = &colorBlendStateInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = *m_pipelineLayout,
        .renderPass = *m_renderPass,
        .subpass = 0
    };

    m_pipeline = m_device->createGraphicsPipelineUnique(nullptr, graphicsPipelineInfo).value;

    // swapchain
    createSwapchain();

    // imgui
    vk::DescriptorPoolSize imguiPoolSizes[] = {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };

    vk::DescriptorPoolCreateInfo imguiPoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = std::size(imguiPoolSizes),
        .pPoolSizes = imguiPoolSizes
    };

    m_imguiDescriptorPool = m_device->createDescriptorPoolUnique(imguiPoolInfo);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(m_window);

    ImGui_ImplVulkan_InitInfo imguiInfo {
        .Instance = *m_instance,
        .PhysicalDevice = m_physicalDevice,
        .Device = *m_device,
        .Queue = m_graphicsQueue,
        .DescriptorPool = *m_imguiDescriptorPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    ImGui_ImplVulkan_Init(&imguiInfo, *m_renderPass);
    auto cmd = beginSingleUseCommandBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(*cmd);
    endSingleUseCommandBuffer(*cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    m_isInitialized = true;
}

Vulkan::~Vulkan()
{
    ImGui_ImplVulkan_Shutdown();
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo commandBufferInfo {
        .commandPool = *m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    auto cmd = beginSingleUseCommandBuffer();
    cmd->copyBuffer(src, dst, { { .size = size } });
    endSingleUseCommandBuffer(*cmd);
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, const vk::MemoryPropertyFlags memPropertyFlags)
{
    vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memPropertyFlags) == memPropertyFlags) {
            return i;
        }
    }
    return 0;
}

vk::UniqueBuffer Vulkan::createBufferUnique(vk::DeviceSize& size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    return m_device->createBufferUnique(bufferInfo);
}

vk::UniqueBuffer Vulkan::createStagingBufferUnique(vk::DeviceSize& size)
{
    return createBufferUnique(size, vk::BufferUsageFlagBits::eTransferSrc);
}

vk::UniqueDeviceMemory Vulkan::createDeviceMemoryUnique(const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags)
{
    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = requirements.size,
        .memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, memoryFlags)
    };

    return m_device->allocateMemoryUnique(memoryInfo);
}

vk::UniqueDeviceMemory Vulkan::createStagingMemoryUnique(vk::Buffer& buffer, vk::DeviceSize& size)
{
    return createDeviceMemoryUnique(m_device->getBufferMemoryRequirements(buffer), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void Vulkan::bindVertexBuffer(const std::vector<pl::Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_vertexCount = vertices.size();
    m_indexCount = indices.size();

    // vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    auto vertexStagingBuffer = createStagingBufferUnique(vertexBufferSize);
    auto vertexStagingMemory = createStagingMemoryUnique(*vertexStagingBuffer, vertexBufferSize);
    m_device->bindBufferMemory(*vertexStagingBuffer, *vertexStagingMemory, 0);

    void* vertexMemoryPtr = m_device->mapMemory(*vertexStagingMemory, 0, vertexBufferSize);
    memcpy(vertexMemoryPtr, vertices.data(), vertexBufferSize);
    m_device->unmapMemory(*vertexStagingMemory);

    m_vertexBuffer = createBufferUnique(vertexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
    m_vertexMemory = createDeviceMemoryUnique(m_device->getBufferMemoryRequirements(*m_vertexBuffer), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device->bindBufferMemory(*m_vertexBuffer, *m_vertexMemory, 0);
    copyBuffer(*vertexStagingBuffer, *m_vertexBuffer, vertexBufferSize);

    // index buffer
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    auto indexStagingBuffer = createStagingBufferUnique(indexBufferSize);
    auto indexStagingMemory = createStagingMemoryUnique(*indexStagingBuffer, indexBufferSize);
    m_device->bindBufferMemory(*indexStagingBuffer, *indexStagingMemory, 0);

    void* indexMemoryPtr = m_device->mapMemory(*indexStagingMemory, 0, indexBufferSize);
    memcpy(indexMemoryPtr, indices.data(), indexBufferSize);
    m_device->unmapMemory(*indexStagingMemory);

    m_indexBuffer = createBufferUnique(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    m_indexMemory = createDeviceMemoryUnique(m_device->getBufferMemoryRequirements(*m_indexBuffer), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device->bindBufferMemory(*m_indexBuffer, *m_indexMemory, 0);
    copyBuffer(*indexStagingBuffer, *m_indexBuffer, indexBufferSize);

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(s_concurrentFrames);
    m_uniformMemorys.resize(s_concurrentFrames);
    m_uniformPtrs.resize(s_concurrentFrames);

    for (size_t i = 0; i < s_concurrentFrames; i++) {
        m_uniformBuffers[i] = createBufferUnique(uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
        m_uniformMemorys[i] = createDeviceMemoryUnique(m_device->getBufferMemoryRequirements(*m_uniformBuffers[i]), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_device->bindBufferMemory(*m_uniformBuffers[i], *m_uniformMemorys[i], 0);
        m_uniformPtrs[i] = m_device->mapMemory(*m_uniformMemorys[i], 0, uniformBufferSize);
    }

    m_isVerticesBound = true;
}

void Vulkan::loadTextureImage(const char* path)
{
    if (!m_isVerticesBound) {
        LOG_ERROR("Failed to load texture: no vertices bound", "GFX");
        return;
    }

    vk::Extent2D extent;
    unsigned char* px = pl::stbLoadTexture(path, &extent.width, &extent.height);

    vk::DeviceSize imageSize = extent.width * extent.height * 4;
    vk::UniqueBuffer stagingBuffer = createStagingBufferUnique(imageSize);
    vk::UniqueDeviceMemory stagingMemory = createStagingMemoryUnique(*stagingBuffer, imageSize);
    m_device->bindBufferMemory(*stagingBuffer, *stagingMemory, 0);

    void* ptr = m_device->mapMemory(*stagingMemory, 0, imageSize);
    memcpy(ptr, px, imageSize);
    m_device->unmapMemory(*stagingMemory);

    // image
    auto imageFormat = vk::Format::eR8G8B8A8Unorm;

    m_texture = createImageUnique(extent, imageFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
    m_textureMemory = createDeviceMemoryUnique(m_device->getImageMemoryRequirements(*m_texture), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device->bindImageMemory(*m_texture, *m_textureMemory, 0);

    transitionImageLayout(*m_texture, imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1 },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { extent.width, extent.height, 1 }
    };

    auto cmd = beginSingleUseCommandBuffer();
    cmd->copyBufferToImage(*stagingBuffer, *m_texture, vk::ImageLayout::eTransferDstOptimal, region);
    endSingleUseCommandBuffer(*cmd);

    transitionImageLayout(*m_texture, imageFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    // image view
    m_textureView = createImageViewUnique(*m_texture, imageFormat, vk::ImageAspectFlagBits::eColor);

    // sampler
    vk::SamplerCreateInfo samplerInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = m_physicalDevice.getProperties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE
    };

    m_textureSampler = m_device->createSamplerUnique(samplerInfo);

    // descriptor sets
    m_descriptorSets.resize(s_concurrentFrames);

    for (size_t i = 0; i < s_concurrentFrames; i++) {
        vk::DescriptorBufferInfo uboInfo {
            .buffer = *m_uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        vk::DescriptorImageInfo samplerInfo {
            .sampler = *m_textureSampler,
            .imageView = *m_textureView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet uboWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboInfo
        };

        vk::WriteDescriptorSet samplerWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &samplerInfo
        };

        std::array<vk::WriteDescriptorSet, 2> writeDescriptors { uboWriteDescriptor, samplerWriteDescriptor };
        m_device->updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
    }

    pl::stbFreeTexture(px);
    m_isTextureLoaded = true;
}

vk::UniqueImage Vulkan::createImageUnique(vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage)
{
    vk::ImageCreateInfo imageInfo {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    return m_device->createImageUnique(imageInfo);
}

vk::UniqueImageView Vulkan::createImageViewUnique(vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask)
{
    vk::ImageViewCreateInfo imageViewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    return m_device->createImageViewUnique(imageViewInfo);
}

void Vulkan::transitionImageLayout(vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout)
{
    vk::PipelineStageFlags srcStageMask;
    vk::PipelineStageFlags dstStageMask;
    vk::AccessFlags srcAccessMask;
    vk::AccessFlags dstAccessMask;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        srcAccessMask = {};
        dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStageMask = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        dstAccessMask = vk::AccessFlagBits::eShaderRead;
        srcStageMask = vk::PipelineStageFlagBits::eTransfer;
        dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    }

    vk::ImageMemoryBarrier barrier {
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
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    auto cmd = beginSingleUseCommandBuffer();
    cmd->pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
    endSingleUseCommandBuffer(*cmd);
}

void Vulkan::createSwapchain(vk::SwapchainKHR oldSwapchain)
{
    // swapchain
    vk::SurfaceCapabilitiesKHR capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    vk::Extent2D swapchainExtent {
        std::clamp(m_extent2D.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(m_extent2D.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    auto imageFormat = vk::Format::eB8G8R8A8Unorm;

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = *m_surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = imageFormat,
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

    m_swapchain = m_device->createSwapchainKHRUnique(swapChainInfo);
    m_images = m_device->getSwapchainImagesKHR(*m_swapchain);

    // image views
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        m_imageViews[i] = createImageViewUnique(m_images[i], imageFormat, vk::ImageAspectFlagBits::eColor);
    }

    // depth buffer
    auto depthFormat = vk::Format::eD32Sfloat;

    m_depthImage = createImageUnique(swapchainExtent, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    m_depthMemory = createDeviceMemoryUnique(m_device->getImageMemoryRequirements(*m_depthImage), vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_device->bindImageMemory(*m_depthImage, *m_depthMemory, 0);

    m_depthView = createImageViewUnique(*m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

    // framebuffers
    m_framebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        std::array<vk::ImageView, 2> attachments = { *m_imageViews[i], *m_depthView };
        vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = *m_renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapchainExtent.width,
            .height = swapchainExtent.height,
            .layers = 1
        };
        m_framebuffers[i] = m_device->createFramebufferUnique(framebufferInfo);
    }
}

vk::UniqueCommandBuffer Vulkan::beginSingleUseCommandBuffer()
{
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = *m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::UniqueCommandBuffer commandBuffer { std::move(m_device->allocateCommandBuffersUnique(bufferInfo)[0]) };
    commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return commandBuffer;
}

void Vulkan::endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    m_graphicsQueue.submit(submitInfo);
    m_graphicsQueue.waitIdle();
}

void Vulkan::recreateSwapchain()
{
    m_device->waitIdle();
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    m_extent2D = vk::Extent2D {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    createSwapchain(*m_swapchain);
}

void Vulkan::recordCommandBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    vk::CommandBufferBeginInfo commandBufferInfo {};
    vk::ClearValue clearColor { .color = std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };
    vk::ClearValue clearDepthValue { .depthStencil = vk::ClearDepthStencilValue { 1.0f } };
    std::array<vk::ClearValue, 2> clearValues { clearColor, clearDepthValue };

    vk::RenderPassBeginInfo renderPassInfo {
        .renderPass = *m_renderPass,
        .framebuffer = *m_framebuffers[imageIndex],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = m_extent2D },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };

    commandBuffer.begin(commandBufferInfo);
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);

        vk::Viewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(m_extent2D.width),
            .height = static_cast<float>(m_extent2D.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor {
            .offset = { 0, 0 },
            .extent = m_extent2D
        };
        commandBuffer.setScissor(0, 1, &scissor);

        commandBuffer.bindVertexBuffers(0, *m_vertexBuffer, { 0 });
        commandBuffer.bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, 1, &(*m_descriptorSets[m_currentFrame]), 0, nullptr);
        commandBuffer.drawIndexed(m_indexCount, 1, 0, 0, 0);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }
    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void Vulkan::drawFrame()
{
    auto device = *m_device;
    auto inFlight = *m_inFlightFences[m_currentFrame];
    auto imageAvailable = *m_imageAvailableSemaphores[m_currentFrame];
    auto renderFinished = *m_renderFinishedSemaphores[m_currentFrame];
    auto swapchain = *m_swapchain;
    auto commandBuffer = *m_commandBuffers[m_currentFrame];
    auto graphicsQueue = m_graphicsQueue;

    auto result = m_device->waitForFences(inFlight, true, UINT64_MAX);

    uint32_t imageIndex;

    try {
        std::tie(result, imageIndex) = m_device->acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE);
        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
            m_imageAvailableSemaphores[m_currentFrame] = m_device->createSemaphoreUnique({});
            recreateSwapchain();
            return;
        }
    } catch (vk::OutOfDateKHRError&) {
        recreateSwapchain();
        return;
    }

    m_device->resetFences(inFlight);

    commandBuffer.reset();
    recordCommandBuffer(commandBuffer, imageIndex);

    vk::PipelineStageFlags waitDstStageMask { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailable,
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinished
    };

    graphicsQueue.submit(submitInfo, inFlight);

    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    try {
        result = graphicsQueue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || m_isResized) {
            recreateSwapchain();
            m_isResized = false;
        }
    } catch (vk::OutOfDateKHRError&) {
        recreateSwapchain();
        m_isResized = false;
    }

    m_currentFrame = (m_currentFrame + 1) % s_concurrentFrames;
}

void Vulkan::modelViewProj()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        .view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.15f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        .proj = glm::perspective(glm::radians(45.0f), m_extent2D.width / (float)m_extent2D.height, 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1;

    memcpy(m_uniformPtrs[m_currentFrame], &ubo, sizeof(ubo));
}

void Vulkan::run()
{
    if (!m_isInitialized) {
        LOG_ERROR("Failed to run: Vulkan not initialized.", "GFX");
        return;
    }
    if (!m_isTextureLoaded) {
        LOG_ERROR("Failed to run: no texture loaded.", "GFX");
        return;
    }
    if (!m_isVerticesBound) {
        LOG_ERROR("Failed to run: no vertices bound.", "GFX");
        return;
    }

    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                break;
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_isResized = true;
                }
            }
        }

        if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
            continue;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(m_window);
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        modelViewProj();
        drawFrame();
    }

    m_device->waitIdle();
}

}
