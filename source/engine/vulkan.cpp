#include "vulkan.hpp"

#include "vk_/log.hpp"
#include "vk_/primitive.hpp"
#include "vk_/sdl2.hpp"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

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
    m_extent2D = vk::Extent2D {
        .width = s_windowWidth,
        .height = s_windowHeight
    };

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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_uniqueInstance);

    // surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, *m_uniqueInstance, &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(*m_uniqueInstance);
    m_uniqueSurface = vk::UniqueSurfaceKHR(surface, deleter);

    // device
    m_device = vk_::Device(*m_uniqueInstance, *m_uniqueSurface, s_concurrentFrames);

    // pipeline
    m_pipeline = vk_::Pipeline(m_device.device(), m_extent2D, s_concurrentFrames);

    // swapchain
    m_swapchain = vk_::Swapchain(m_window, *m_uniqueSurface, m_extent2D,
        m_device.physicalDevice(), m_device.device(), m_pipeline.renderPass());

    m_isInitialized = true;
}

Vulkan::~Vulkan()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::bindVertexBuffer(const std::vector<vk_::Vertex>& vertices, const std::vector<uint16_t>& indices)
{
    m_vertexCount = vertices.size();
    m_indexCount = indices.size();
    m_buffer = vk_::Buffer(m_device.physicalDevice(), m_device.device(), m_device.commandPool(), m_device.graphicsQueue(), vertices, indices, s_concurrentFrames);
    m_isVerticesBound = true;
}

void Vulkan::loadTextureImage(const char* path)
{
    if (!m_isVerticesBound) {
        vk_::LOG_ERROR("Failed to load texture: no vertices bound");
        return;
    }
    m_texture = vk_::Texture(path, m_device.physicalDevice(), m_device.device(), m_device.commandPool(), m_device.graphicsQueue());
    m_pipeline.setDescriptorSets(m_device.device(), m_buffer.uniformBuffers(), m_texture.sampler(), m_texture.imageView(), s_concurrentFrames);
    m_isTextureLoaded = true;
}

void Vulkan::recreateSwapchain()
{
    m_device.waitIdle();
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    m_extent2D = vk::Extent2D {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    m_swapchain.recreate(m_window, *m_uniqueSurface, m_extent2D, m_device.physicalDevice(), m_device.device(), m_pipeline.renderPass());
}

void Vulkan::recordCommandBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    vk::CommandBufferBeginInfo commandBufferInfo {};
    vk::ClearValue clearColor { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };
    vk::RenderPassBeginInfo renderPassInfo {
        .renderPass = m_pipeline.renderPass(),
        .framebuffer = m_swapchain.framebuffer(imageIndex),
        .renderArea = {
            .offset = { 0, 0 },
            .extent = m_extent2D },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    commandBuffer.begin(commandBufferInfo);
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.pipeline());

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

        commandBuffer.bindVertexBuffers(0, m_buffer.vertexBuffer(), { 0 });
        commandBuffer.bindIndexBuffer(m_buffer.indexBuffer(), 0, vk::IndexType::eUint16);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.pipelineLayout(), 0, 1, &m_pipeline.descriptorSet(m_currentFrame), 0, nullptr);
        commandBuffer.drawIndexed(m_indexCount, 1, 0, 0, 0);
    }
    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void Vulkan::drawFrame()
{
    auto device = m_device.device();
    auto inFlight = m_device.inFlightFence(m_currentFrame);
    auto imageAvailable = m_device.imageAvailableSemaphore(m_currentFrame);
    auto renderFinished = m_device.renderFinishedSemaphore(m_currentFrame);
    auto swapchain = m_swapchain.swapchain();
    auto commandBuffer = m_device.commandBuffer(m_currentFrame);
    auto graphicsQueue = m_device.graphicsQueue();

    auto result = device.waitForFences(inFlight, true, UINT64_MAX);

    uint32_t imageIndex;

    try {
        std::tie(result, imageIndex) = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE);
        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
            m_device.recreateImageAvailableSemaphore(m_currentFrame);
            recreateSwapchain();
            return;
        }
    } catch (vk::OutOfDateKHRError&) {
        recreateSwapchain();
        return;
    }

    device.resetFences(inFlight);

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

    vk_::UniformBufferObject ubo {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), m_extent2D.width / (float)m_extent2D.height, 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1;

    m_buffer.updateUniformBuffer(m_currentFrame, ubo);
}

void Vulkan::run()
{
    if (!m_isInitialized) {
        vk_::LOG_ERROR("Failed to run: Vulkan not initialized.");
        return;
    }
    if (!m_isTextureLoaded) {
        vk_::LOG_ERROR("Failed to run: no texture loaded.");
        return;
    }
    if (!m_isVerticesBound) {
        vk_::LOG_ERROR("Failed to run: no vertices bound.");
        return;
    }

    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
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

        modelViewProj();
        drawFrame();
    }

    m_device.waitIdle();
}

}
