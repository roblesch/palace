#include "swapchain.hpp"

namespace vk_ {

Swapchain::Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::Device& device, vk::RenderPass& renderPass)
    : m_window(window)
    , m_device(&device)
{
    // swapchain
    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = vk::Format::eB8G8R8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = extent2D,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = VK_TRUE
    };

    m_uniqueSwapchain = m_device->createSwapchainKHRUnique(swapChainInfo);
    m_images = m_device->getSwapchainImagesKHR(m_uniqueSwapchain.get());

    // image views
    m_uniqueImageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        vk::ImageViewCreateInfo imageViewInfo {
            .image = m_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eB8G8R8A8Srgb,
            .subresourceRange {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1 }
        };
        m_uniqueImageViews[i] = m_device->createImageViewUnique(imageViewInfo);
    }

    // framebuffers
    m_uniqueFramebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = &m_uniqueImageViews[i].get(),
            .width = extent2D.width,
            .height = extent2D.height,
            .layers = 1
        };
        m_uniqueFramebuffers[i] = m_device->createFramebufferUnique(framebufferInfo);
    }
}

std::vector<vk::UniqueFramebuffer>& Swapchain::getUniqueFramebuffers()
{
    return m_uniqueFramebuffers;
}

vk::SwapchainKHR& Swapchain::getSwapchain()
{
    return m_uniqueSwapchain.get();
}

} // namespace vk_
