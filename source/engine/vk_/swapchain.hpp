#ifndef PALACE_ENGINE_VK_SWAPCHAIN_HPP
#define PALACE_ENGINE_VK_SWAPCHAIN_HPP

#include "include.hpp"

namespace vk_ {

class Swapchain {
private:
    SDL_Window* m_window {};
    vk::PhysicalDevice m_physicalDevice;
    const vk::Device* m_device {};
    vk::Format m_imageFormat {};
    vk::Extent2D m_extent;

    vk::UniqueSwapchainKHR m_uniqueSwapchain;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_uniqueImageViews;

public:
    Swapchain() = default;
    Swapchain(SDL_Window* window, const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physicalDevice, const vk::Device& device);
};

} // namespace vk_

#endif
