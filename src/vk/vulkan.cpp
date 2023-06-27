#include "vulkan.hpp"

#include "util/log.hpp"
#include <chrono>
#include <fstream>

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
    : isValidationEnabled_(enableValidation)
{
    // dynamic dispatch loader
    auto vkGetInstanceProcAddr = dynamicLoader_.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window_ = SDL_CreateWindow("palace", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        sWidth_, sHeight_, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    SDL_SetWindowMinimumSize(window_, 400, 300);
    extent_ = vk::Extent2D {
        .width = sWidth_,
        .height = sHeight_
    };

    // sdl2
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, nullptr);
    std::vector<const char*> instanceExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, instanceExtensions.data());

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

    if (isValidationEnabled_) {
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
    instance_ = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

    // surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window_, *instance_, &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(*instance_);
    surface_ = vk::UniqueSurfaceKHR(surface, deleter);

    // physical device
    for (auto& _physicalDevice : instance_->enumeratePhysicalDevices()) {
        vk::PhysicalDeviceProperties deviceProperties = _physicalDevice.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = _physicalDevice.getFeatures();
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            physicalDevice_ = _physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            physicalDevice_ = _physicalDevice;
            break;
        } else if (deviceProperties.deviceType == vk::PhysicalDeviceType::eCpu) {
            physicalDevice_ = _physicalDevice;
            break;
        }
    }

    // queue families
    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice_.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilyIndices_.graphics = i;
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queueInfos {
        { .queueFamilyIndex = queueFamilyIndices_.graphics,
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

    device_ = physicalDevice_.createDeviceUnique(deviceInfo);

    // graphics queue
    graphicsQueue_ = device_->getQueue(queueFamilyIndices_.graphics, 0);

    // command pool
    vk::CommandPoolCreateInfo commandPoolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndices_.graphics
    };

    commandPool_ = device_->createCommandPoolUnique(commandPoolInfo);

    // command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo {
        .commandPool = *commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = sConcurrentFrames_
    };

    commandBuffers_ = device_->allocateCommandBuffersUnique(commandBufferAllocInfo);

    // sync
    for (size_t i = 0; i < sConcurrentFrames_; i++) {
        imageAvailableSemaphores_.push_back(device_->createSemaphoreUnique({}));
        renderFinishedSemaphores_.push_back(device_->createSemaphoreUnique({}));
        inFlightFences_.push_back(device_->createFenceUnique({ .flags = vk::FenceCreateFlagBits::eSignaled }));
    }

    // TODO: descriptor sets
    // uniform modelview transforms
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo {
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    pipelineDescriptorLayout_ = device_->createDescriptorSetLayoutUnique(descriptorLayoutInfo);

    // descriptor pools
    vk::DescriptorPoolSize uboSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = sConcurrentFrames_
    };

    vk::DescriptorPoolCreateInfo pipelinePoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = sConcurrentFrames_,
        .poolSizeCount = 1,
        .pPoolSizes = &uboSize
    };

    pipelineDescriptorPool_ = device_->createDescriptorPoolUnique(pipelinePoolInfo);

    // shaders
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    auto vertexShaderModule = device_->createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    auto fragmentShaderModule = device_->createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
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
    vk::VertexInputAttributeDescription normalDescription {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, normal)
    };
    vk::VertexInputAttributeDescription colorDescription {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, color)
    };
    vk::VertexInputAttributeDescription uvDescription {
        .location = 3,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(pl::Vertex, uv)
    };
    std::array<vk::VertexInputAttributeDescription, 4> vertexAttributeDescriptions { posDescription, normalDescription, colorDescription, uvDescription };

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
        .width = static_cast<float>(extent_.width),
        .height = static_cast<float>(extent_.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor {
        .offset = { 0, 0 },
        .extent = extent_
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // rasterization state
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
    vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &pipelineDescriptorLayout_.get(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    pipelineLayout_ = device_->createPipelineLayoutUnique(pipelineLayoutInfo);

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

    renderPass_ = device_->createRenderPassUnique(renderPassInfo);

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
        .layout = *pipelineLayout_,
        .renderPass = *renderPass_,
        .subpass = 0
    };

    vertColorPipeline_ = device_->createGraphicsPipelineUnique(nullptr, graphicsPipelineInfo).value;

    // uniform buffers
    vk::DeviceSize uniformBufferSize = sizeof(UniformBuffer);

    uniformBuffers_.resize(sConcurrentFrames_);
    uniformMemories_.resize(sConcurrentFrames_);
    uniformPtrs_.resize(sConcurrentFrames_);

    for (size_t i = 0; i < sConcurrentFrames_; i++) {
        uniformBuffers_[i] = createBufferUnique(uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer);
        uniformMemories_[i] = createDeviceMemoryUnique(device_->getBufferMemoryRequirements(*uniformBuffers_[i]), vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        device_->bindBufferMemory(*uniformBuffers_[i], *uniformMemories_[i], 0);
        uniformPtrs_[i] = device_->mapMemory(*uniformMemories_[i], 0, uniformBufferSize);
    }

    // descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(sConcurrentFrames_, *pipelineDescriptorLayout_);

    vk::DescriptorSetAllocateInfo descriptorSetInfo {
        .descriptorPool = *pipelineDescriptorPool_,
        .descriptorSetCount = sConcurrentFrames_,
        .pSetLayouts = layouts.data()
    };

    pipelineDescriptorSets_ = device_->allocateDescriptorSetsUnique(descriptorSetInfo);

    for (size_t i = 0; i < sConcurrentFrames_; i++) {
        vk::DescriptorBufferInfo uboInfo {
            .buffer = *uniformBuffers_[i],
            .offset = 0,
            .range = sizeof(UniformBuffer)
        };

        vk::WriteDescriptorSet uboWriteDescriptor {
            .dstSet = *pipelineDescriptorSets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboInfo
        };

        device_->updateDescriptorSets(1, &uboWriteDescriptor, 0, nullptr);
    }

    // swapchain
    createSwapchain();

    // memory
    pl::MemoryCreateInfo memoryInfo {
        .physicalDevice = physicalDevice_,
        .device = *device_,
        .instance = *instance_,
        .commandPool = *commandPool_,
        .graphicsQueue = graphicsQueue_
    };

    memory_ = pl::createMemoryUnique(memoryInfo);

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

    imguiDescriptorPool_ = device_->createDescriptorPoolUnique(imguiPoolInfo);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(window_);

    ImGui_ImplVulkan_InitInfo imguiInfo {
        .Instance = *instance_,
        .PhysicalDevice = physicalDevice_,
        .Device = *device_,
        .Queue = graphicsQueue_,
        .DescriptorPool = *imguiDescriptorPool_,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    ImGui_ImplVulkan_Init(&imguiInfo, *renderPass_);
    auto cmd = memory_->beginSingleUseCommandBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(*cmd);
    memory_->endSingleUseCommandBuffer(*cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // camera
    camera_ = Camera { .fovy = 45.0f, .znear = 0.1f, .zfar = 100.0f };
    camera_.lookAt({ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f });
    camera_.resize((float)extent_.width / extent_.height);

    isInitialized_ = true;
}

Vulkan::~Vulkan()
{
    ImGui_ImplVulkan_Shutdown();
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Vulkan::loadGltfModel(const char* path)
{
    model_ = pl::createGltfModelUnique({ path, memory_.get() });
    isSceneLoaded_ = true;
}

void Vulkan::run()
{
    if (!isInitialized_) {
        LOG_ERROR("Failed to run: Vulkan not initialized.", "GFX");
        return;
    }
    if (!isSceneLoaded_) {
        LOG_ERROR("Failed to run: no scene loaded.", "GFX");
        return;
    }

    bool quit = false;
    ticks = SDL_GetTicks64();

    glm::ivec4 wasd {};
    glm::ivec2 spacelctrl {};
    float speed = 1.0f;

    glm::ivec2 center {};
    glm::ivec2 mouse {};
    glm::ivec2 mouse_ {};

    const Uint8* keyStates = SDL_GetKeyboardState(nullptr);

    while (!quit) {
        Uint64 start = SDL_GetPerformanceCounter();
        dt = SDL_GetTicks64() - ticks;
        ticks += dt;

        center = { extent_.width / 2, extent_.height / 2 };

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {

            case SDL_QUIT:
                quit = true;
                break;

            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    isResized_ = true;
                    break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                case SDL_BUTTON_MIDDLE:
                    camera_.reset();
                    break;
                case SDL_BUTTON_RIGHT:
                    SDL_GetMouseState(&mouse_.x, &mouse_.y);
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    SDL_WarpMouseInWindow(window_, center.x, center.y);
                    break;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                case SDL_BUTTON_RIGHT:
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_WarpMouseInWindow(window_, mouse_.x, mouse_.y);
                    break;
                }
                break;

            case SDL_MOUSEWHEEL:
                camera_.zoom(event.wheel.y);
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_w:
                    wasd[0] = 1;
                    break;
                case SDLK_a:
                    wasd[1] = 1;
                    break;
                case SDLK_s:
                    wasd[2] = 1;
                    break;
                case SDLK_d:
                    wasd[3] = 1;
                    break;
                case SDLK_SPACE:
                    spacelctrl[0] = 1;
                    break;
                case SDLK_LCTRL:
                    spacelctrl[1] = 1;
                    break;
                case SDLK_LSHIFT:
                    speed = 0.25f;
                    break;
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_w:
                    wasd[0] = 0;
                    break;
                case SDLK_a:
                    wasd[1] = 0;
                    break;
                case SDLK_s:
                    wasd[2] = 0;
                    break;
                case SDLK_d:
                    wasd[3] = 0;
                    break;
                case SDLK_SPACE:
                    spacelctrl[0] = 0;
                    break;
                case SDLK_LCTRL:
                    spacelctrl[1] = 0;
                    break;
                case SDLK_LSHIFT:
                    speed = 1.0f;
                    break;
                }
                break;
            }
        }

        if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)
            continue;

        if (SDL_GetRelativeMouseMode()) {
            SDL_GetMouseState(&mouse.x, &mouse.y);
            camera_.rotate(mouse.x - center.x, mouse.y - center.y);
            SDL_WarpMouseInWindow(window_, center.x, center.y);
        }

        keyStates = SDL_GetKeyboardState(nullptr);

        if (keyStates[SDL_SCANCODE_W] || keyStates[SDL_SCANCODE_A] || keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_D] || keyStates[SDL_SCANCODE_SPACE] || keyStates[SDL_SCANCODE_LCTRL] || keyStates[SDL_SCANCODE_LSHIFT])
            camera_.move(wasd, spacelctrl, speed);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(window_);
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        updateUniformBuffers(0);
        drawFrame();

        Uint64 end = SDL_GetPerformanceCounter();
        float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency();

        printf("\33[2KMS: %f\r", elapsedMS);
    }

    device_->waitIdle();
}

void Vulkan::drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node)
{
    if (node->mesh != nullptr && node->mesh->primitives.size() > 0) {
        glm::mat4 matrix = node->matrix;
        pl::Node* parent = node->parent;
        while (parent != nullptr) {
            matrix = parent->matrix * matrix;
            parent = parent->parent;
        }
        commandBuffer.pushConstants(*pipelineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &matrix);
        for (const auto& _primitive : node->mesh->primitives) {
            if (_primitive->indexCount > 0) {
                commandBuffer.drawIndexed(_primitive->indexCount, 1, _primitive->firstIndex, 0, 0);
            }
        }
    }
    for (const auto& _node : node->children) {
        drawNode(commandBuffer, _node);
    }
}

void Vulkan::updateUniformBuffers(int dx)
{
    ubo_.model = glm::rotate(ubo_.model, 0.0001f * dx, camera_.up);
    ubo_.view = camera_.view;
    ubo_.proj = camera_.proj;

    memcpy(uniformPtrs_[currentFrame_], &ubo_, sizeof(ubo_));
}

void Vulkan::drawFrame()
{
    auto inFlight = *inFlightFences_[currentFrame_];
    auto imageAvailable = *imageAvailableSemaphores_[currentFrame_];
    auto renderFinished = *renderFinishedSemaphores_[currentFrame_];
    auto commandBuffer = *commandBuffers_[currentFrame_];

    auto result = device_->waitForFences(inFlight, true, UINT64_MAX);

    uint32_t imageIndex;

    try {
        std::tie(result, imageIndex) = device_->acquireNextImageKHR(*swapchain_, UINT64_MAX, imageAvailable, VK_NULL_HANDLE);
        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
            imageAvailableSemaphores_[currentFrame_] = device_->createSemaphoreUnique({});
            recreateSwapchain();
            return;
        }
    } catch (vk::OutOfDateKHRError&) {
        recreateSwapchain();
        return;
    }

    device_->resetFences(inFlight);

    commandBuffer.reset();
    vk::CommandBufferBeginInfo commandBufferInfo {};
    vk::ClearValue clearColor { .color = std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };
    vk::ClearValue clearDepthValue { .depthStencil = vk::ClearDepthStencilValue { 1.0f } };
    std::array<vk::ClearValue, 2> clearValues { clearColor, clearDepthValue };

    vk::RenderPassBeginInfo renderPassInfo {
        .renderPass = *renderPass_,
        .framebuffer = *swapchainFramebuffers_[imageIndex],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = extent_ },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };

    commandBuffer.begin(commandBufferInfo);
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *vertColorPipeline_);

        vk::Viewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent_.width),
            .height = static_cast<float>(extent_.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor {
            .offset = { 0, 0 },
            .extent = extent_
        };
        commandBuffer.setScissor(0, 1, &scissor);

        commandBuffer.bindVertexBuffers(0, vk::Buffer(model_->vertexBuffer->buffer), { 0 });
        commandBuffer.bindIndexBuffer(vk::Buffer(model_->indexBuffer->buffer), 0, vk::IndexType::eUint32);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout_, 0, 1, &pipelineDescriptorSets_[currentFrame_].get(), 0, nullptr);

        for (const auto& _node : model_->defaultScene->nodes) {
            drawNode(commandBuffer, _node);
        }

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }
    commandBuffer.endRenderPass();
    commandBuffer.end();

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

    graphicsQueue_.submit(submitInfo, inFlight);

    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_.get(),
        .pImageIndices = &imageIndex
    };

    try {
        result = graphicsQueue_.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || isResized_) {
            recreateSwapchain();
            isResized_ = false;
        }
    } catch (vk::OutOfDateKHRError&) {
        recreateSwapchain();
        isResized_ = false;
    }

    currentFrame_ = (currentFrame_ + 1) % sConcurrentFrames_;
}

vk::UniqueBuffer Vulkan::createBufferUnique(vk::DeviceSize& size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo bufferInfo {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    return device_->createBufferUnique(bufferInfo);
}

vk::UniqueDeviceMemory Vulkan::createDeviceMemoryUnique(const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice_.getMemoryProperties();
    uint32_t memoryTypeIndex = 0;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags) {
            memoryTypeIndex = i;
        }
    }

    vk::MemoryAllocateInfo memoryInfo {
        .allocationSize = requirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    return device_->allocateMemoryUnique(memoryInfo);
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

    return device_->createImageUnique(imageInfo);
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

    return device_->createImageViewUnique(imageViewInfo);
}

void Vulkan::createSwapchain(vk::SwapchainKHR oldSwapchain)
{
    // swapchain
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);
    vk::Extent2D swapchainExtent {
        std::clamp(extent_.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(extent_.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    auto imageFormat = vk::Format::eB8G8R8A8Unorm;

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = *surface_,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = imageFormat,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain
    };

    swapchain_ = device_->createSwapchainKHRUnique(swapChainInfo);
    swapchainImages_ = device_->getSwapchainImagesKHR(*swapchain_);

    // image views
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        swapchainImageViews_[i] = createImageViewUnique(swapchainImages_[i], imageFormat, vk::ImageAspectFlagBits::eColor);
    }

    // depth buffer
    auto depthFormat = vk::Format::eD32Sfloat;

    depthImage_ = createImageUnique(swapchainExtent, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    depthMemory_ = createDeviceMemoryUnique(device_->getImageMemoryRequirements(*depthImage_), vk::MemoryPropertyFlagBits::eDeviceLocal);
    device_->bindImageMemory(*depthImage_, *depthMemory_, 0);

    depthView_ = createImageViewUnique(*depthImage_, depthFormat, vk::ImageAspectFlagBits::eDepth);

    // framebuffers
    swapchainFramebuffers_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        std::array<vk::ImageView, 2> attachments = { *swapchainImageViews_[i], *depthView_ };
        vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = *renderPass_,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapchainExtent.width,
            .height = swapchainExtent.height,
            .layers = 1
        };
        swapchainFramebuffers_[i] = device_->createFramebufferUnique(framebufferInfo);
    }
}

void Vulkan::recreateSwapchain()
{
    device_->waitIdle();
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);

    extent_ = vk::Extent2D {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    createSwapchain(*swapchain_);
    camera_.resize((float)extent_.width / extent_.height);
}

} // namespace pl
