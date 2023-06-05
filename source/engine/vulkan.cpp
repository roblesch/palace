#include "vulkan.hpp"
#include "vk_/sdl2.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace engine {

Vulkan::Vulkan(bool enableValidation)
    : m_isValidationEnabled(enableValidation)
{
    // dynamic dispatch loader
    auto vkGetInstanceProcAddr = m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // window
    m_window = sdl2::createWindow(s_windowWidth, s_windowHeight);
    m_extent2D = vk::Extent2D(s_windowWidth, s_windowHeight);

    // sdl2
    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, nullptr);
    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, extensionNames.data());

    // molten vk
    vk::InstanceCreateFlagBits flags {};
#ifdef __APPLE__
    extensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    extensionNames.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // application
    vk::ApplicationInfo appInfo {
        .pApplicationName = "viewer",
        .pEngineName = "palace",
        .apiVersion = VK_API_VERSION_1_0
    };

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};
    std::vector<const char*> validationLayers;

    // validation
    if (m_isValidationEnabled) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        debugInfo = vk_::debug::createInfo();
    }

    // instance
    vk::InstanceCreateInfo instanceInfo {
        .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data()
    };
    m_uniqueInstance = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_uniqueInstance.get());

    // surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, m_uniqueInstance.get(), &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(m_uniqueInstance.get());
    m_uniqueSurface = vk::UniqueSurfaceKHR(surface, deleter);

    // device
    m_device = vk_::Device(m_uniqueInstance.get(), m_uniqueSurface.get(), s_concurrentFrames);

    // pipeline
    m_pipeline = vk_::Pipeline(m_device.device(), m_extent2D);

    // swapchain
    m_swapchain = vk_::Swapchain(m_window, m_uniqueSurface.get(), m_extent2D,
        m_device.physicalDevice(), m_device.device(), m_pipeline.renderPass());
}

Vulkan::~Vulkan()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::recreateSwapchain()
{
    // wait for gpu idle
    m_device.waitIdle();

    // get window extends
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    m_extent2D = vk::Extent2D(w, h);

    // recreate swapchain
    m_swapchain.recreate(m_window, m_uniqueSurface.get(), m_extent2D,
        m_device.physicalDevice(), m_device.device(), m_pipeline.renderPass());
    
    // done resizing
    m_isResized = false;
}

void Vulkan::recordCommandBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    vk::RenderPass renderPass = m_pipeline.renderPass();
    vk::Framebuffer framebuffer = m_swapchain.framebuffer(imageIndex);
    vk::Pipeline pipeline = m_pipeline.pipeline();

    // begin command buffer
    vk::CommandBufferBeginInfo beginInfo {};
    commandBuffer.begin(beginInfo);

    // begin render pass
    vk::ClearValue clearColor { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };
    vk::RenderPassBeginInfo renderPassInfo {
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = m_extent2D },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    // bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // draw
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
    commandBuffer.draw(3, 1, 0, 0);

    // end render pass
    commandBuffer.endRenderPass();

    // end command buffer
    commandBuffer.end();
}

void Vulkan::drawFrame()
{
    vk::Device device = m_device.device();
    vk::Fence frameInFlight = m_device.fenceInFlight(m_currentFrame);
    vk::Semaphore imageAvailable = m_device.semaphoreImageAvailable(m_currentFrame);
    vk::Semaphore renderFinished = m_device.semaphoreRenderFinished(m_currentFrame);
    vk::SwapchainKHR swapchain = m_swapchain.swapchain();
    vk::CommandBuffer commandBuffer = m_device.commandBuffer(m_currentFrame);
    vk::Queue graphicsQueue = m_device.graphicsQueue();

    while (device.waitForFences(frameInFlight, VK_TRUE, UINT64_MAX) == vk::Result::eTimeout)
        ;

    uint32_t imageIndex;
    try {
        imageIndex = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE).value;
    } catch (...) {
        recreateSwapchain();
        return;
    }

    device.resetFences(frameInFlight);
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

    graphicsQueue.submit(submitInfo, frameInFlight);

    vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    try {
        graphicsQueue.presentKHR(presentInfo);
    } catch (...) {
        recreateSwapchain();
    }

    m_currentFrame = (m_currentFrame + 1) % s_concurrentFrames;
}

void Vulkan::run()
{
    if (!m_isInitialized) {
        return;
    }
    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_isResized = true;
                }
            }
        }
        drawFrame();
    }
    m_device.waitIdle();
}

} // namespace engine
