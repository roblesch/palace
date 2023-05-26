#ifndef PALACE_GRAPHICS_VULKANSWAPCHAIN_HPP
#define PALACE_GRAPHICS_VULKANSWAPCHAIN_HPP

#include "vulkaninclude.hpp"

namespace graphics::vk_ {

class Swapchain {
private:
    SDL_Window* m_window;
    const vk::Device* m_device;
    vk::Format m_imageFormat;
    vk::Extent2D m_extent;

    vk::UniqueSwapchainKHR m_uniqueSwapchain;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_uniqueImageViews;

public:
    Swapchain() = default;
    Swapchain(SDL_Window* window, const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physicalDevice, const vk::Device& device);
};

}

#endif
