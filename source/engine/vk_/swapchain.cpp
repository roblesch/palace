#include "swapchain.hpp"

namespace vk_ {

vk::UniqueImageView Swapchain::createImageViewUnique(vk::Device& device, vk::Image& image, const vk::Format format)
{
    vk::ImageViewCreateInfo imageViewInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    return device.createImageViewUnique(imageViewInfo);
}

void Swapchain::create(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass)
{
    // swapchain
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = vk::Format::eB8G8R8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = {
            std::clamp(extent2D.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(extent2D.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) },
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = VK_TRUE
    };

    m_uniqueSwapchain = device.createSwapchainKHRUnique(swapChainInfo);
    m_images = device.getSwapchainImagesKHR(swapchain());

    // image views
    m_uniqueImageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        m_uniqueImageViews[i] = createImageViewUnique(device, m_images[i], vk::Format::eB8G8R8A8Srgb);
    }

    // framebuffers
    m_uniqueFramebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = &imageView(i),
            .width = extent2D.width,
            .height = extent2D.height,
            .layers = 1
        };
        m_uniqueFramebuffers[i] = device.createFramebufferUnique(framebufferInfo);
    }
}

Swapchain::Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass)
{
    create(window, surface, extent2D, physicalDevice, device, renderPass);
}

vk::Framebuffer& Swapchain::framebuffer(size_t i)
{
    return *m_uniqueFramebuffers[i];
}

vk::ImageView& Swapchain::imageView(size_t i)
{
    return *m_uniqueImageViews[i];
}

vk::SwapchainKHR& Swapchain::swapchain()
{
    return *m_uniqueSwapchain;
}

void Swapchain::recreate(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass)
{
    m_uniqueFramebuffers.clear();
    m_uniqueImageViews.clear();
    m_uniqueSwapchain.reset();
    create(window, surface, extent2D, physicalDevice, device, renderPass);
}

} // namespace vk_
