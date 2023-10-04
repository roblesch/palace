#include "graphics.hpp"

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

void GraphicsContext::create()
{
    window.create(application.applicationName);
    instance.create(window, debug, application);
    physicalDevice.create(instance);
    surface.create(window, instance, physicalDevice);
    graphicsQueue.create(physicalDevice);
    device.create(physicalDevice, graphicsQueue);

    swapchain.create(device, window, surface);
    viewport.create(swapchain.extent2D);
    triangleVert.create(device, "shaders/v_triangle.spv", VK_SHADER_STAGE_VERTEX_BIT);
    triangleFrag.create(device, "shaders/f_triangle.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pipelineLayout.create(device);
    pipeline.create(device, pipelineLayout, viewport, swapchain.colorFormat, { triangleVert.pipelineShaderStageCreateInfo(), triangleFrag.pipelineShaderStageCreateInfo() });

    commandPool.create(device, graphicsQueue.familyIndex);
    commandBuffer.create(device, instance, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    renderFence.create(device);
    renderSemaphore.create(device);
    presentSemaphore.create(device);
}

void GraphicsContext::destroy()
{
    vkQueueWaitIdle(graphicsQueue);
    vkDeviceWaitIdle(device);

    renderSemaphore.destroy(device);
    presentSemaphore.destroy(device);
    renderFence.destroy(device);
    commandPool.destroy(device);

    pipeline.destroy(device);
    pipelineLayout.destroy(device);
    triangleVert.destroy(device);
    triangleFrag.destroy(device);

    swapchain.destroy(device);
    device.destroy();
    surface.destroy(instance);
    instance.destroy();
    window.destroy();
}

/*
 */

void GraphicsContext::draw()
{
    renderFence.wait(device);
    renderFence.reset(device);

    uint32_t imageIndex;
    swapchain.acquireNextImage(device, presentSemaphore, &imageIndex);

    commandBuffer.reset();
    commandBuffer.begin();
    commandBuffer.writeBarrier(swapchain.imageWriteBarrier(imageIndex));
    {
        commandBuffer.beginRendering(swapchain.renderingInfo(imageIndex));
        commandBuffer.endRendering();
    }
    commandBuffer.presentBarrier(swapchain.imagePresentBarrier(imageIndex));
    commandBuffer.end();

    graphicsQueue.submit(presentSemaphore, commandBuffer, renderSemaphore, renderFence);
    graphicsQueue.present(renderSemaphore, swapchain, imageIndex);
}

/*
 */

// void GraphicsContext::createImGui()
//{
//     VkDescriptorPoolCreateInfo pool_info = {
//         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//         .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
//         .maxSets = 1,
//         .poolSizeCount = 1,
//         .pPoolSizes = new VkDescriptorPoolSize {
//             .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//             .descriptorCount = 1 }
//     };
//
//     vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_.descriptorPool);
//
//     ImGui::CreateContext();
//     ImGui_ImplSDL2_InitForVulkan(window);
//
//     ImGui_ImplVulkan_InitInfo imguiInfo {
//         .Instance = instance,
//         .PhysicalDevice = physicalDevice,
//         .Device = device,
//         .Queue = graphicsQueue,
//         .DescriptorPool = imgui_.descriptorPool,
//         .MinImageCount = surface.capabilities.minImageCount,
//         .ImageCount = surface.capabilities.minImageCount,
//         .MSAASamples = VK_SAMPLE_COUNT_4_BIT,
//         .UseDynamicRendering = true,
//         .ColorAttachmentFormat = swapchain.colorFormat
//     };
//
//     ImGui_ImplVulkan_Init(&imguiInfo, nullptr);
// }

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
