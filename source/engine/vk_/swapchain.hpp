#pragma once

#include "include.hpp"

namespace vk_ {

class Swapchain {
private:
    vk::UniqueSwapchainKHR m_swapchain;

    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
    std::vector<vk::UniqueFramebuffer> m_framebuffers;

    vk::UniqueImage m_depthImage;
    vk::UniqueDeviceMemory m_depthMemory;
    vk::UniqueImageView m_depthView;

    void create(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass, vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);

public:
    static void transitionImageLayout(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout);
    static vk::UniqueImage createImageUnique(vk::Device& device, vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage);
    static vk::UniqueImageView createImageViewUnique(vk::Device& device, vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask);

    Swapchain() = default;
    Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass);

    vk::Framebuffer& framebuffer(size_t frame);
    vk::ImageView& imageView(size_t frame);
    vk::SwapchainKHR& swapchain();

    void recreate(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass);
};

}
