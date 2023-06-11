#include "swapchain.hpp"

#include "buffer.hpp"
#include "device.hpp"

namespace vk_ {

void Swapchain::transitionImageLayout(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout)
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

    vk::UniqueCommandBuffer commandBuffer = Device::beginSingleUseCommandBuffer(device, commandPool);
    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, barrier);
    Device::endSingleUseCommandBuffer(*commandBuffer, graphicsQueue);
}

vk::UniqueImage Swapchain::createImageUnique(vk::Device& device, vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage)
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

    return device.createImageUnique(imageInfo);
}

vk::UniqueImageView Swapchain::createImageViewUnique(vk::Device& device, vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask)
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

    return device.createImageViewUnique(imageViewInfo);
}

void Swapchain::create(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass, vk::SwapchainKHR oldSwapchain)
{
    // swapchain
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    vk::Extent2D swapchainExtent {
        std::clamp(extent2D.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(extent2D.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    auto imageFormat = vk::Format::eB8G8R8A8Srgb;

    vk::SwapchainCreateInfoKHR swapChainInfo {
        .surface = surface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = imageFormat,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
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

    m_swapchain = device.createSwapchainKHRUnique(swapChainInfo);
    m_images = device.getSwapchainImagesKHR(swapchain());

    // image views
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        m_imageViews[i] = createImageViewUnique(device, m_images[i], imageFormat, vk::ImageAspectFlagBits::eColor);
    }

    // depth buffer
    auto depthFormat = vk::Format::eD32Sfloat;

    m_depthImage = createImageUnique(device, swapchainExtent, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
    m_depthMemory = Buffer::createDeviceMemoryUnique(physicalDevice, device, device.getImageMemoryRequirements(*m_depthImage), vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.bindImageMemory(*m_depthImage, *m_depthMemory, 0);

    m_depthView = createImageViewUnique(device, *m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

    // framebuffers
    m_framebuffers.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        std::array<vk::ImageView, 2> attachments = { imageView(i), *m_depthView };
        vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapchainExtent.width,
            .height = swapchainExtent.height,
            .layers = 1
        };
        m_framebuffers[i] = device.createFramebufferUnique(framebufferInfo);
    }
}

Swapchain::Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass)
{
    create(window, surface, extent2D, physicalDevice, device, renderPass);
}

vk::Framebuffer& Swapchain::framebuffer(size_t frame)
{
    return *m_framebuffers[frame];
}

vk::ImageView& Swapchain::imageView(size_t frame)
{
    return *m_imageViews[frame];
}

vk::SwapchainKHR& Swapchain::swapchain()
{
    return *m_swapchain;
}

void Swapchain::recreate(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass)
{
    create(window, surface, extent2D, physicalDevice, device, renderPass, *m_swapchain);
}

}
