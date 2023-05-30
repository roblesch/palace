#include "swapchain.hpp"

namespace vk_ {

Swapchain::Swapchain(SDL_Window* window, const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physicalDevice, const vk::Device& device)
    : m_window(window)
    , m_physicalDevice(physicalDevice)
    , m_device(&device)
    , m_imageFormat(vk::Format::eB8G8R8A8Srgb)
{
    // swapchain
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(surface);
    m_extent2D = surfaceCapabilities.currentExtent;

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = m_imageFormat,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = m_extent2D,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eMailbox,
        .clipped = VK_TRUE
    };

    m_uniqueSwapchain = device.createSwapchainKHRUnique(swapChainInfo);
    m_images = m_device->getSwapchainImagesKHR(m_uniqueSwapchain.get());
    m_uniqueImageViews.resize(m_images.size());

    // image views
    for (size_t i = 0; i < m_images.size(); i++) {
        vk::ImageViewCreateInfo imageViewInfo {
            .image = m_images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = m_imageFormat,
            .subresourceRange {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1 }
        };
        m_uniqueImageViews[i] = m_device->createImageViewUnique(imageViewInfo);
    }
}

} // namespace vk_
