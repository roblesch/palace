#include "vktypes.hpp"

VkResult GraphicsQueue::submit(Semaphore& waitSemaphore, CommandBuffer& commandBuffer, Semaphore& signalSemaphore, Fence& fence)
{
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signalSemaphore,
    };

    return vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
}

VkResult GraphicsQueue::present(Semaphore& waitSemaphore, Swapchain& swapchain, uint32_t imageIndex)
{
    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    return vkQueuePresentKHR(graphicsQueue, &presentInfo);
}

VkResult Swapchain::acquireNextImage(Device& device, Semaphore& semaphore, uint32_t* index)
{
    return vkAcquireNextImageKHR(device, swapchain, TIMEOUT, semaphore, nullptr, index);
}
