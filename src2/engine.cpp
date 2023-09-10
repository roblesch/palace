#include "engine.hpp"

#include "log.hpp"
#include "parser.hpp"
#include <chrono>
#include <fstream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define SHADOW_PASS false
#define COLOR_PASS true

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

namespace pl {

Engine::Engine()
{
}

Engine::~Engine()
{
    ImGui_ImplVulkan_Shutdown();
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Engine::init(bool enableValidation)
{
    isValidationEnabled_ = enableValidation;

    createInstance();
    createDevice();
    createCommandBuffers();
    createMemoryHelper();
    createShadowPassResources();
    createDescriptorLayouts();
    createRenderPass();
    createPipelines();
    createStorageBuffers();
    createSwapchain();
    createGpuSync();
    initImGui();
    initCamera();

    isInitialized_ = true;
}

void Engine::loadGltfModel(const char* path)
{
    model_ = pl::createGltfModelUnique({ path, memoryHelper_.get() });
    if (!model_->complete)
        return;

    createDescriptorPool();
    createDescriptorSets();

    isSceneLoaded_ = true;
}

void Engine::run()
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
    float speed = 5.0f;
    float slow = 1.0f;

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
                    slow = 0.5f;
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
                    slow = 1.0f;
                    break;
                }
                break;
            }
        }

        if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)
            continue;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(window_);
        ImGui::NewFrame();
        // ImGui::ShowDemoWindow();
        ImGui::Render();

        drawFrame();

        Uint64 end = SDL_GetPerformanceCounter();
        float elapsedMs = (float)(end - start) / (float)SDL_GetPerformanceFrequency();

        if (SDL_GetRelativeMouseMode()) {
            SDL_GetMouseState(&mouse.x, &mouse.y);
            camera_.rotate(mouse.x - center.x, mouse.y - center.y);
            SDL_WarpMouseInWindow(window_, center.x, center.y);
        }

        keyStates = SDL_GetKeyboardState(nullptr);

        if (keyStates[SDL_SCANCODE_W] || keyStates[SDL_SCANCODE_A] || keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_D] || keyStates[SDL_SCANCODE_SPACE] || keyStates[SDL_SCANCODE_LCTRL] || keyStates[SDL_SCANCODE_LSHIFT])
            camera_.move(wasd, spacelctrl, speed * elapsedMs * slow);

        updateUniformBuffers(elapsedMs);

        printf("\33[2K%d fps\r", (int)(1.0 / elapsedMs));
    }

    device_->waitIdle();
}

void Engine::createInstance()
{
    // window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window_ = SDL_CreateWindow("viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        sWidth_, sHeight_, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    SDL_SetWindowMinimumSize(window_, 800, 600);
    extent_ = vk::Extent3D { sWidth_, sHeight_, 1 };

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
        .apiVersion = VK_API_VERSION_1_1
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

    // loader
    vk::DynamicLoader dl;
    auto getInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(getInstanceProcAddr);

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
}

void Engine::createDevice()
{
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
}

void Engine::createCommandBuffers()
{
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
}

void Engine::createMemoryHelper()
{
    pl::MemoryHelperCreateInfo memoryInfo {
        .engine = this,
        .physicalDevice = physicalDevice_,
        .device = *device_,
        .instance = *instance_
    };

    memoryHelper_ = createMemoryHelperUnique(memoryInfo);
}

void Engine::createShadowPassResources()
{
    shadowPass_.width = sShadowResolution_;
    shadowPass_.height = sShadowResolution_;

    vk::Extent3D extent { .width = sShadowResolution_, .height = sShadowResolution_, .depth = 1 };

    shadowPass_.depthImage = memoryHelper_->createImage(extent, sDepthAttachmentFormat_, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, 1, vk::SampleCountFlagBits::e1);
    shadowPass_.depthView = memoryHelper_->createImageViewUnique(shadowPass_.depthImage->image, sDepthAttachmentFormat_, vk::ImageAspectFlagBits::eDepth, 1);

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

    shadowPass_.depthSampler = device_->createSamplerUnique(samplerInfo);

    vk::AttachmentDescription depthAttachment {
        .format = sDepthAttachmentFormat_,
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

    shadowPass_.renderPass = device_->createRenderPassUnique(renderPassInfo);

    vk::FramebufferCreateInfo framebufferInfo {
        .renderPass = *shadowPass_.renderPass,
        .attachmentCount = 1,
        .pAttachments = &shadowPass_.depthView.get(),
        .width = shadowPass_.width,
        .height = shadowPass_.height,
        .layers = 1
    };

    shadowPass_.frameBuffer = device_->createFramebufferUnique(framebufferInfo);
}

void Engine::createDescriptorLayouts()
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
    descriptorLayouts_.ubo = device_->createDescriptorSetLayoutUnique(uboDescriptorLayoutInfo);

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
    descriptorLayouts_.material = device_->createDescriptorSetLayoutUnique(materialDescriptorLayoutInfo);
}

void Engine::createRenderPass()
{
    vk::AttachmentDescription colorAttachment {
        .format = sSwapchainFormat_,
        .samples = sMsaaSamples_,
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
        .format = sSwapchainFormat_,
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
        .format = sDepthAttachmentFormat_,
        .samples = sMsaaSamples_,
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

    renderPass_ = device_->createRenderPassUnique(renderPassInfo);
}

void Engine::createPipelines()
{
    // shaders
    std::vector<char> shadowShaderBytes = readSpirVFile("shaders/shadow.spv");
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    vk::UniqueShaderModule shadowShaderModule = device_->createShaderModuleUnique({ .codeSize = shadowShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shadowShaderBytes.data()) });

    vk::UniqueShaderModule vertexShaderModule = device_->createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    vk::UniqueShaderModule fragmentShaderModule = device_->createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.data()) });

    vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(pushConstants_)
    };

    // shadow pipeline layout
    vk::PipelineLayoutCreateInfo shadowPassPipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayouts_.ubo.get(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    shadowPass_.pipelineLayout = device_->createPipelineLayoutUnique(shadowPassPipelineLayoutInfo);

    // texture pipeline layout
    std::vector<vk::DescriptorSetLayout> setLayouts = { *descriptorLayouts_.ubo, *descriptorLayouts_.material };

    vk::PipelineLayoutCreateInfo texturePipelineLayoutInfo {
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    texturePipeline_.layout = device_->createPipelineLayoutUnique(texturePipelineLayoutInfo);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos = {
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
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = offsetof(pl::Vertex, color)
    };
    vk::VertexInputAttributeDescription uvDescription {
        .location = 3,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(pl::Vertex, uv)
    };

    std::array<vk::VertexInputAttributeDescription, 4> vertexAttributeDescriptions { posDescription, normalDescription, colorDescription, uvDescription };

    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingDescription,
        .vertexAttributeDescriptionCount = vertexAttributeDescriptions.size(),
        .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
    };

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport state
    vk::Viewport viewport = {
        .x = 0.0f,
        .y = (float)extent_.height,
        .width = static_cast<float>(extent_.width),
        .height = -static_cast<float>(extent_.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vk::Rect2D scissor = {
        .offset = { 0, 0 },
        .extent = { extent_.width, extent_.height }
    };
    vk::PipelineViewportStateCreateInfo viewportStateInfo = {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // rasterization state
    vk::PipelineRasterizationStateCreateInfo rasterStateInfo = {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // multisample state
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo = {
        .rasterizationSamples = sMsaaSamples_,
        .sampleShadingEnable = VK_FALSE
    };

    // color blend state
    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo = {
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // depth state
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo = {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = vk::CompareOp::eGreaterOrEqual,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // dynamic state
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .pVertexInputState = &vertexInputStateInfo,
        .pInputAssemblyState = &inputAssemblyStateInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterStateInfo,
        .pMultisampleState = &multisampleStateInfo,
        .pDepthStencilState = &depthStencilStateInfo,
        .pColorBlendState = &colorBlendStateInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = *texturePipeline_.layout,
        .renderPass = *renderPass_,
        .subpass = 0
    };

    texturePipeline_.pipeline = device_->createGraphicsPipelineUnique(nullptr, pipelineInfo).value;

    // shadow pass pipeline
    vk::PipelineShaderStageCreateInfo shadowPassStageInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shadowShaderModule,
        .pName = "main"
    };
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &shadowPassStageInfo;
    colorBlendStateInfo.attachmentCount = 0;
    rasterStateInfo.cullMode = vk::CullModeFlagBits::eNone;
    rasterStateInfo.depthBiasEnable = VK_TRUE;
    multisampleStateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    depthStencilStateInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
    dynamicStates.push_back(vk::DynamicState::eDepthBias);
    dynamicStateInfo = {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    pipelineInfo.renderPass = *shadowPass_.renderPass;
    shadowPass_.pipeline = device_->createGraphicsPipelineUnique(nullptr, pipelineInfo).value;
}

void Engine::createStorageBuffers()
{
    // uniform buffers
    uniformBuffers_.resize(sConcurrentFrames_);
    for (int i = 0; i < sConcurrentFrames_; i++) {
        uniformBuffers_[i].buffer = memoryHelper_->createBuffer(sizeof(ubo_), vk::BufferUsageFlagBits::eUniformBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }
}

void Engine::createSwapchain(vk::SwapchainKHR oldSwapchain)
{
    // multisampling
    colorImage_ = memoryHelper_->createImage(extent_, sSwapchainFormat_, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, 1, sMsaaSamples_);
    colorImageView_ = memoryHelper_->createImageViewUnique(colorImage_->image, sSwapchainFormat_, vk::ImageAspectFlagBits::eColor, 1);

    // swapchain
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice_.getSurfaceCapabilitiesKHR(*surface_);
    vk::Extent2D swapchainExtent {
        std::clamp(extent_.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(extent_.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = *surface_,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = sSwapchainFormat_,
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

    swapchain_ = device_->createSwapchainKHRUnique(swapChainInfo);
    swapchainImages_ = device_->getSwapchainImagesKHR(*swapchain_);

    // image views
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        swapchainImageViews_[i] = memoryHelper_->createImageViewUnique(swapchainImages_[i], sSwapchainFormat_, vk::ImageAspectFlagBits::eColor, 1);
    }

    // depth buffer
    depthImage_ = memoryHelper_->createImage(extent_, sDepthAttachmentFormat_, vk::ImageUsageFlagBits::eDepthStencilAttachment, 1, sMsaaSamples_);
    depthView_ = memoryHelper_->createImageViewUnique(depthImage_->image, sDepthAttachmentFormat_, vk::ImageAspectFlagBits::eDepth, 1);

    // framebuffers
    swapchainFramebuffers_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        std::array<vk::ImageView, 3> attachments = { *colorImageView_, *depthView_, *swapchainImageViews_[i] };
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

void Engine::createGpuSync()
{
    for (size_t i = 0; i < sConcurrentFrames_; i++) {
        imageAvailableSemaphores_.push_back(device_->createSemaphoreUnique({}));
        renderFinishedSemaphores_.push_back(device_->createSemaphoreUnique({}));
        inFlightFences_.push_back(device_->createFenceUnique({ .flags = vk::FenceCreateFlagBits::eSignaled }));
    }
}

void Engine::initImGui()
{
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
        .MSAASamples = VK_SAMPLE_COUNT_4_BIT
    };

    ImGui_ImplVulkan_Init(&imguiInfo, *renderPass_);
    auto cmd = beginOneTimeCommandBuffer();
    ImGui_ImplVulkan_CreateFontsTexture(*cmd);
    endOneTimeCommandBuffer(*cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Engine::createDescriptorPool()
{
    uint32_t uboCount = 2;
    uint32_t samplerCount = 2 + 2 * static_cast<uint32_t>(model_->materials.size());

    vk::DescriptorPoolSize uboSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = uboCount
    };
    vk::DescriptorPoolSize samplerSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = samplerCount
    };
    std::array<vk::DescriptorPoolSize, 2> pipelinePoolSizes { uboSize, samplerSize };

    vk::DescriptorPoolCreateInfo pipelinePoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = uboCount + samplerCount,
        .poolSizeCount = static_cast<uint32_t>(pipelinePoolSizes.size()),
        .pPoolSizes = pipelinePoolSizes.data()
    };

    descriptorPool_ = device_->createDescriptorPoolUnique(pipelinePoolInfo);
}

void Engine::createDescriptorSets()
{
    // ubo descriptors
    for (int i = 0; i < sConcurrentFrames_; i++) {
        vk::DescriptorSetAllocateInfo uboDescriptorSetInfo {
            .descriptorPool = *descriptorPool_,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorLayouts_.ubo.get()
        };
        uniformBuffers_[i].descriptorSet = std::move(device_->allocateDescriptorSetsUnique(uboDescriptorSetInfo)[0]);
        vk::DescriptorBufferInfo uboBufferInfo {
            .buffer = uniformBuffers_[i].buffer->buffer,
            .offset = 0,
            .range = sizeof(ubo_)
        };
        vk::WriteDescriptorSet sceneUboWriteDescriptor {
            .dstSet = *uniformBuffers_[i].descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboBufferInfo
        };
        vk::DescriptorImageInfo shadowMapSamplerInfo {
            .sampler = *shadowPass_.depthSampler,
            .imageView = *shadowPass_.depthView,
            .imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal
        };
        vk::WriteDescriptorSet shadowMapWriteDescriptor {
            .dstSet = *uniformBuffers_[i].descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &shadowMapSamplerInfo
        };
        std::array<vk::WriteDescriptorSet, 2> uboWriteDescriptors = { sceneUboWriteDescriptor, shadowMapWriteDescriptor };
        device_->updateDescriptorSets(static_cast<uint32_t>(uboWriteDescriptors.size()), uboWriteDescriptors.data(), 0, nullptr);
    }

    // material descriptor sets
    for (auto& _material : model_->materials) {
        vk::DescriptorSetAllocateInfo descriptorSetInfo {
            .descriptorPool = *descriptorPool_,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorLayouts_.material.get()
        };
        _material->descriptorSet = std::move(device_->allocateDescriptorSetsUnique(descriptorSetInfo)[0]);
        vk::DescriptorImageInfo textureSamplerInfo {
            .sampler = _material->baseColor->sampler.get(),
            .imageView = _material->baseColor->view.get(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        vk::WriteDescriptorSet textureWriteDescriptor {
            .dstSet = _material->descriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &textureSamplerInfo
        };
        vk::DescriptorImageInfo normalSamplerInfo;
        if (_material->useNormalTexture > 0.5f) {
            normalSamplerInfo = {
                .sampler = _material->normal->sampler.get(),
                .imageView = _material->normal->view.get(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
        } else {
            normalSamplerInfo = {
                .sampler = _material->baseColor->sampler.get(),
                .imageView = _material->baseColor->view.get(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
        }
        vk::WriteDescriptorSet normalWriteDescriptor {
            .dstSet = _material->descriptorSet.get(),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &normalSamplerInfo
        };
        std::array<vk::WriteDescriptorSet, 2> writeDescriptors = { textureWriteDescriptor, normalWriteDescriptor };
        device_->updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
    }
}

void Engine::initCamera()
{
    camera_.lookAt({ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    camera_.resize((float)extent_.width / (float)extent_.height);
}

vk::UniqueCommandBuffer Engine::beginOneTimeCommandBuffer()
{
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = *commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    vk::UniqueCommandBuffer commandBuffer { std::move(device_->allocateCommandBuffersUnique(bufferInfo)[0]) };

    commandBuffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    return commandBuffer;
}

void Engine::endOneTimeCommandBuffer(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    graphicsQueue_.submit(submitInfo);
    graphicsQueue_.waitIdle();
}

void Engine::recreateSwapchain()
{
    device_->waitIdle();
    int width, height;
    SDL_GetWindowSize(window_, &width, &height);

    extent_ = vk::Extent3D { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    createSwapchain(*swapchain_);
    camera_.resize((float)extent_.width / (float)extent_.height);
}

void Engine::updateUniformBuffers(float dt)
{
    // ubo_.lightPos = glm::rotate(ubo_.lightPos, dt*0.2f, glm::vec3(0.0f, 1.0f, 0.0f));

    ubo_.cameraView = camera_.view;
    ubo_.cameraProj = camera_.proj;

    ubo_.lightView = glm::lookAt(glm::vec3(ubo_.lightPos), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo_.lightProj = glm::perspective(45.0f, 1.0f, 1.0f, 1000.0f);

    memoryHelper_->uploadToBufferDirect(uniformBuffers_[currentFrame_].buffer, &ubo_);
}

void Engine::drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node)
{
    if (node->mesh != nullptr && !node->mesh->primitives.empty()) {
        pushConstants_.meshTransform = node->globalMatrix;
        for (const auto& _primitive : node->mesh->primitives) {
            if (_primitive->indexCount > 0) {
                if (_primitive->material->baseColor) {
                    std::array<vk::DescriptorSet, 2> descriptorSets {
                        uniformBuffers_[currentFrame_].descriptorSet.get(),
                        _primitive->material->descriptorSet.get()
                    };
                    pushConstants_.useNormalTexture = _primitive->material->useNormalTexture;
                    commandBuffer.pushConstants(*texturePipeline_.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pushConstants_);
                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *texturePipeline_.layout, 0, 2, descriptorSets.data(), 0, nullptr);
                }
                commandBuffer.drawIndexed(_primitive->indexCount, 1, _primitive->firstIndex, 0, 0);
            }
        }
    }
    for (const auto& _node : node->children) {
        drawNode(commandBuffer, _node);
    }
}

void Engine::drawNodeShadow(vk::CommandBuffer& commandBuffer, pl::Node* node)
{
    if (node->mesh != nullptr && !node->mesh->primitives.empty()) {
        pushConstants_.meshTransform = node->globalMatrix;
        pushConstants_.useNormalTexture = 0.0f;
        for (const auto& _primitive : node->mesh->primitives) {
            if (_primitive->indexCount > 0) {
                commandBuffer.pushConstants(*shadowPass_.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pushConstants_);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *shadowPass_.pipelineLayout, 0, 1, &uniformBuffers_[currentFrame_].descriptorSet.get(), 0, nullptr);
                commandBuffer.drawIndexed(_primitive->indexCount, 1, _primitive->firstIndex, 0, 0);
            }
        }
    }
    for (const auto& _node : node->children) {
        drawNodeShadow(commandBuffer, _node);
    }
}

void Engine::drawFrame()
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
    vk::CommandBufferBeginInfo beginInfo {};
    commandBuffer.begin(beginInfo);

    vk::ClearValue clearColor { .color = { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f } } };
    vk::ClearValue clearDepthValue { .depthStencil = vk::ClearDepthStencilValue { 0.0f } };
    std::array<vk::ClearValue, 2> clearValues { clearColor, clearDepthValue };

    /*
        Shadow pass
    */
    if (SHADOW_PASS) {
        vk::RenderPassBeginInfo renderPassInfo {
            .renderPass = *shadowPass_.renderPass,
            .framebuffer = *shadowPass_.frameBuffer,
            .renderArea = {
                .offset = { 0, 0 },
                .extent = { shadowPass_.width, shadowPass_.height },
            },
            .clearValueCount = 1,
            .pClearValues = &clearDepthValue
        };

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        vk::Viewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(shadowPass_.width),
            .height = static_cast<float>(shadowPass_.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor {
            .offset = { 0, 0 },
            .extent = { shadowPass_.width, shadowPass_.height }
        };
        commandBuffer.setScissor(0, 1, &scissor);
        commandBuffer.setDepthBias(1.25f, 0.0f, 1.75f);
        commandBuffer.bindVertexBuffers(0, vk::Buffer(model_->vertexBuffer->buffer), { 0 });
        commandBuffer.bindIndexBuffer(vk::Buffer(model_->indexBuffer->buffer), 0, vk::IndexType::eUint32);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *shadowPass_.pipeline);

        for (const auto& _node : model_->defaultScene->nodes) {
            drawNodeShadow(commandBuffer, _node);
        }

        commandBuffer.endRenderPass();
    }

    /*
        Color pass
    */
    if (COLOR_PASS) {
        vk::RenderPassBeginInfo renderPassInfo {
            .renderPass = *renderPass_,
            .framebuffer = *swapchainFramebuffers_[imageIndex],
            .renderArea = {
                .offset = { 0, 0 },
                .extent = { extent_.width, extent_.height } },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data()
        };

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        {
            vk::Viewport viewport {
                .x = 0.0f,
                .y = (float)extent_.height,
                .width = static_cast<float>(extent_.width),
                .height = -static_cast<float>(extent_.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            };
            commandBuffer.setViewport(0, 1, &viewport);

            vk::Rect2D scissor {
                .offset = { 0, 0 },
                .extent = { extent_.width, extent_.height }
            };
            commandBuffer.setScissor(0, 1, &scissor);

            commandBuffer.bindVertexBuffers(0, vk::Buffer(model_->vertexBuffer->buffer), { 0 });
            commandBuffer.bindIndexBuffer(vk::Buffer(model_->indexBuffer->buffer), 0, vk::IndexType::eUint32);
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *texturePipeline_.pipeline);

            for (const auto& _node : model_->defaultScene->nodes) {
                drawNode(commandBuffer, _node);
            }

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }
        commandBuffer.endRenderPass();
    }
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

bool Engine::running()
{
    return isRunning_;
}

void Engine::processInput()
{

}

void Engine::updateState()
{

}

void Engine::renderFrame()
{

}

}

using namespace pl;

Parser* args;
Engine* engine;

int main(const int argc, const char* argv[])
{
    args = new Parser(argc, argv);
    engine = new Engine();

    engine->init();
    engine->loadGltfModel(args->gltf_path());
    engine->run();

    while (engine->running())
    {
        engine->processInput();
        engine->updateState();
        engine->renderFrame();
    }

    return 0;
}
